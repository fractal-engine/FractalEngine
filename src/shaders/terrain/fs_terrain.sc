/* The following shader uses functions from the bgfx shader library. Specifically, it uses the bgfx_shader.sh and fs_sms_shadow.sh files.
 * Copyright 2013-2014 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

$input v_out_uv, v_out_worldPos, v_out_shadowCoord, v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom
#include <bgfx_shader.sh>
#include "../common/common.sh"

// --- Texture Samplers ---
SAMPLER2D(s_diffuse,  0);
SAMPLER2D(s_orm,      1);
SAMPLER2D(s_normal,   2);
SAMPLER2DSHADOW(s_shadowMap, 4);

// --- Uniforms ---
uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;
uniform vec4 u_cameraPos;
uniform vec4 u_skyAmbient;

#define PI 3.14159265359

// --- Shadow Filtering (PCF) ---
float PCF(sampler2DShadow shadowSampler, vec4 shadowCoord, float bias, vec2 texelSize) {
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    if (any(lessThan(projCoords.xy, vec2(0.0, 0.0))) || any(greaterThan(projCoords.xy, vec2(1.0, 1.0))) || projCoords.z > 1.0)
        return 1.0;

    float result = 0.0;
    for (float y = -1.5; y <= 1.5; y += 1.0)
        for (float x = -1.5; x <= 1.5; x += 1.0)
            result += shadow2D(shadowSampler, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias));
    return result / 16.0;
}

// --- GGX Lighting ---
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float Ndot, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return Ndot / (Ndot * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0001), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0001), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (vec3_splat(1.0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// --- Main Fragment Shader ---
void main() {
    vec2 uv = v_out_uv * 32.0; // UV scaling for texture detail

    vec3 albedo     = texture2D(s_diffuse, uv).rgb;
    vec3 orm        = texture2D(s_orm, uv).rgb;
    vec3 normalMap  = texture2D(s_normal, uv).rgb * 2.0 - 1.0;

    float ao        = orm.r;
    float roughness = clamp(orm.g, 0.04, 1.0);
    float metalness = orm.b;

    float3 T = normalize(v_out_worldTangent);
    float3 B = normalize(v_out_worldBitangent);
    float3 N = normalize(v_out_worldNormalGeom);

    vec3 N_world = normalize(mul(mat3(T, B, N), normalMap));

    float3 V = normalize(u_cameraPos.xyz - v_out_worldPos);
    vec3 L = normalize(u_sunDirection.xyz);
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3_splat(0.04), albedo, metalness);

    float NdotL = max(dot(N_world, L), 0.0);
    float NdotV = max(dot(N_world, V), 0.0);
    float LdotH = max(dot(L, H), 0.0);

    float D = DistributionGGX(N_world, H, roughness);
    float G = GeometrySmith(N_world, V, L, roughness);
    vec3  F = FresnelSchlick(LdotH, F0);

    vec3 specular = (D * G * F) / (4.0 * NdotL * NdotV + 0.0001);
    vec3 kS = F;
    vec3 kD = (vec3_splat(1.0) - kS) * (1.0 - metalness);

    float shadow = PCF(s_shadowMap, v_out_shadowCoord, 0.005, vec2_splat(1.0 / 2048.0));

    vec3 Lo = (kD * albedo / PI + specular) * u_sunLuminance.rgb * NdotL * shadow;
    vec3 ambient = u_skyAmbient.rgb * albedo * ao;
    vec3 color = toGamma(Lo + ambient);

    gl_FragColor = vec4(color, 1.0);
}
