/* The following shader uses functions from the bgfx shader library. Specifically, it uses the bgfx_shader.sh and fs_sms_shadow.sh files.
 * Copyright 2013-2014 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

$input v_out_uv, v_out_worldPos, v_out_shadowCoord, v_out_viewVec, v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom, v_out_oasisMask

#include <bgfx_shader.sh>
#include "../common/common.sh"

// --- Texture Samplers ---
SAMPLER2D(s_diffuse,  0);          // Terrain diffuse texture
SAMPLER2D(s_orm,      1);          // Terrain ORM texture (Occlusion, Roughness, Metalness)
SAMPLER2D(s_normal,   2);          // Terrain normal map
SAMPLER2D(s_waterNormal, 5);       // Water normal map (used for specular reflections)
SAMPLER2DSHADOW(s_shadowMap, 4);   // Shadow map sampler

// --- Uniforms ---
uniform vec4 u_sunDirection;    // Directional light (sun) direction
uniform vec4 u_sunLuminance;    // Sun light color/intensity
uniform vec4 u_cameraPos;       // Camera world position
uniform vec4 u_skyAmbient;      // Ambient light color
uniform vec4 u_time;            // Animation time



#define PI 3.14159265359

// --- PCF Shadow Filtering ---
// Percentage-Closer Filtering for soft shadows using shadow map samples
float PCF(sampler2DShadow shadowSampler, vec4 shadowCoord, float bias, vec2 texelSize) {
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;

    // Early exit if outside shadow map projection or beyond far plane
    if (any(lessThan(projCoords.xy, vec2(0.0, 0.0))) || 
        any(greaterThan(projCoords.xy, vec2(1.0, 1.0))) || 
        projCoords.z > 1.0)
        return 1.0;

    float result = 0.0;
    // Sample 4x4 grid around current shadow coord for smooth shadow edges
    for (float y = -1.5; y <= 1.5; y += 1.0)
        for (float x = -1.5; x <= 1.5; x += 1.0)
            result += shadow2D(shadowSampler, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias));

    return result / 16.0;
}

// --- GGX Microfacet Distribution and Geometry Functions ---
// Normal distribution function (NDF) for GGX microfacet model
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// Schlick's approximation for geometry term (single direction)
float GeometrySchlickGGX(float Ndot, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return Ndot / (Ndot * (1.0 - k) + k);
}

// Smith's geometry term combining view and light directions
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0001), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0001), roughness);
}

// Fresnel term using Schlick's approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (vec3_splat(1.0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// --- Main Fragment Shader ---
void main() {
    // --- UV Coordinates ---
    vec2 uv = v_out_uv * 150.0;                                 // Terrain tiling
    vec2 flowDir = vec2(0.3, 0.7);                              // arbitrary flow direction
    vec2 uvWater = v_out_uv * 25.0 + flowDir * u_time.x * 0.2;  // Animated water UVs
    vec2 uvMask = v_out_uv * 2.0;                               // Oasis mask scale
    float time = u_time.x;                                      // Animation time for water

    // --- Terrain Textures ---
    vec3 albedo     = texture2D(s_diffuse, uv).rgb;
    vec3 orm        = texture2D(s_orm, uv).rgb;
    vec3 normalMap  = texture2D(s_normal, uv).rgb * 2.0 - 1.0;

    // --- Water ---
    vec3 waterColor    = vec3(0.02, 0.3, 0.4);
    vec3 waterNormal   = texture2D(s_waterNormal, uvWater).rgb * 2.0 - 1.0;
    float waterMetal   = 0.0;
    float waterRough   = 0.05;
    float waterAO      = 1.0;

    // --- Unpack Terrain ORM ---
    float ao        = orm.r;
    float roughness = clamp(orm.g, 0.04, 1.0);
    float metalness = orm.b;

    // --- TBN ---
    float3 T = normalize(v_out_worldTangent);
    float3 B = normalize(v_out_worldBitangent);
    float3 N = normalize(v_out_worldNormalGeom);
    vec3 N_terrain = normalize(mul(mat3(T, B, N), normalMap));
    vec3 N_water   = normalize(mul(mat3(T, B, N), waterNormal));

    // --- View & Light Vectors ---
    vec3 V = normalize(u_cameraPos.xyz - v_out_worldPos);
    vec3 L = normalize(u_sunDirection.xyz);
    vec3 H = normalize(V + L);

    // --- Terrain PBR ---
    vec3 F0_terrain = mix(vec3_splat(0.04), albedo, metalness);
    float NdotL_T = max(dot(N_terrain, L), 0.0);
    float NdotV_T = max(dot(N_terrain, V), 0.0);
    float LdotH_T = max(dot(L, H), 0.0);
    float D_T = DistributionGGX(N_terrain, H, roughness);
    float G_T = GeometrySmith(N_terrain, V, L, roughness);
    vec3  F_T = FresnelSchlick(LdotH_T, F0_terrain);
    vec3  specularT = (D_T * G_T * F_T) / (4.0 * NdotL_T * NdotV_T + 0.0001);
    vec3  kS_T = F_T;
    vec3  kD_T = (vec3_splat(1.0) - kS_T) * (1.0 - metalness);

    // --- Water PBR ---
    vec3 F0_water = vec3_splat(0.04);
    float NdotL_W = max(dot(N_water, L), 0.0);
    float NdotV_W = max(dot(N_water, V), 0.0);
    float LdotH_W = max(dot(L, H), 0.0);
    float D_W = DistributionGGX(N_water, H, waterRough);
    float G_W = GeometrySmith(N_water, V, L, waterRough);
    vec3  F_W = FresnelSchlick(LdotH_W, F0_water);
    vec3  specularW = (D_W * G_W * F_W) / (4.0 * NdotL_W * NdotV_W + 0.0001);
    vec3  kS_W = F_W;
    vec3  kD_W = (vec3_splat(1.0) - kS_W) * (1.0 - waterMetal);

    // --- Shadow ---
    float shadow = PCF(s_shadowMap, v_out_shadowCoord, 0.005, vec2_splat(1.0 / 2048.0));

    // --- Lighting ---
    vec3 Lo_terrain = (kD_T * albedo / PI + specularT) * u_sunLuminance.rgb * NdotL_T * shadow;
    vec3 ambientT = u_skyAmbient.rgb * albedo * ao;

    vec3 Lo_water = (kD_W * waterColor / PI + specularW) * u_sunLuminance.rgb * NdotL_W * shadow;
    vec3 ambientW = u_skyAmbient.rgb * waterColor * waterAO;

    // --- Fresnel and Scattering Additions for Water ---
    float fresnel = pow(1.0 - max(dot(N_water, V), 0.0), 3.0) * 0.5;
    vec3 skyReflection = mix(vec3(0.0, 0.1, 0.3), u_skyAmbient.rgb, fresnel);
    float scatter = smoothstep(0.0, 0.2, 1.0 - clamp(texture2D(s_waterNormal, uvMask).r, 0.0, 1.0));
    vec3 shallowColor = mix(waterColor, vec3(0.0, 0.6, 0.8), scatter);
    vec3 waterFinal = mix(shallowColor, Lo_water + ambientW + skyReflection, fresnel);

    // --- Blend ---
    float mask = v_out_oasisMask;
    vec3 finalColor = mix(Lo_terrain + ambientT, waterFinal, mask);

    // --- Output ---
    gl_FragColor = vec4(toGamma(finalColor), 1.0);
}
