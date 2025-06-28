$input v_uv, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent, v_shadowCoord

#include "common.sh"
#include <bgfx_shader.sh>

// --- Texture Samplers ---
SAMPLER2D(s_diffuse,     0); // Base Color (Albedo)
SAMPLER2D(s_orm,         1); // Occlusion, Roughness, Metalness
SAMPLER2D(s_normal,      2); // Normal Map
SAMPLER2DSHADOW(s_shadowMap, 4); // Shadow map from directional light

// --- Uniforms ---

uniform vec4 u_sunDirection;        // xyz = sun direction, w = unused
uniform vec4 u_sunLuminance;        // xyz = sun color, w = intensity
uniform vec4 u_cameraPos;           // Camera position in world space
uniform vec4 u_skyAmbient;          // Ambient lighting from skybox/environment

#define PI 3.14159265359

// --- Helper functions (Copied from fs_terrain.sc) ---

float PCF(sampler2DShadow shadowSampler, vec4 shadowCoord, float bias, vec2 texelSize) {
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    if (any(lessThan(projCoords.xy, vec2_splat(0.0))) || any(greaterThan(projCoords.xy, vec2_splat(1.0))) || projCoords.z > 1.0)
        return 1.0;
    float result = 0.0;
    for (float y = -1.5; y <= 1.5; y += 1.0)
        for (float x = -1.5; x <= 1.5; x += 1.0)
            result += shadow2D(shadowSampler, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias));
    return result / 16.0;
}

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
    // --- Sample Material Properties ---
    vec3 albedo     = texture2D(s_diffuse, v_uv).rgb;
    vec3 orm        = texture2D(s_orm,     v_uv).rgb;
    vec3 normalMap  = texture2D(s_normal,  v_uv).rgb * 2.0 - 1.0;

    // --- Unpack ORM ---
    float ao        = orm.r;
    float roughness = clamp(orm.g, 0.04, 1.0); // Clamp to avoid pure black specular
    float metalness = orm.b;

    // --- Construct TBN matrix from world-space varyings ---
    mat3 tbn = mat3(normalize(v_worldTangent), normalize(v_worldBitangent), normalize(v_worldNormal));

    // --- Final Normal for Lighting ---
    // Transform normal from tangent-space (from map) to world-space
    vec3 N = normalize(mul(tbn, normalMap));

    // --- Lighting Vectors ---
    vec3 V = normalize(u_cameraPos.xyz - v_worldPos); // View vector
    vec3 L = normalize(u_sunDirection.xyz);           // Light vector
    vec3 H = normalize(V + L);                        // Halfway vector

    // --- PBR Calculations ---
    vec3 F0 = mix(vec3_splat(0.04), albedo, metalness); // Base reflectivity

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0001); // Avoid division by zero

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = FresnelSchlick(max(dot(V, H), 0.0), F0);

    vec3 specular_numerator = D * G * F;
    float specular_denominator = 4.0 * NdotV * NdotL + 0.0001; // Avoid division by zero
    vec3 specular = specular_numerator / specular_denominator;
    
    vec3 kS = F;
    vec3 kD = (vec3_splat(1.0) - kS) * (1.0 - metalness);

    // --- Shadow Mapping ---
    float shadow_bias = max(0.0025 * (1.0 - NdotL), 0.0001);
    float shadow = PCF(s_shadowMap, v_shadowCoord, shadow_bias, vec2_splat(1.0 / 2048.0)); // Assuming 2k shadow map

    // --- Final Lighting Equation ---
    vec3 sunlight = u_sunLuminance.rgb * u_sunLuminance.w;
    vec3 directLighting = (kD * albedo / PI + specular) * sunlight * NdotL * shadow;
    vec3 ambientLighting = u_skyAmbient.rgb * albedo * ao;

    vec3 litColor = directLighting + ambientLighting;

    // --- Tonemapping & Gamma Correction ---
    // Using the same exposure/tonemapping as the terrain for consistency
    float sunElevation = u_sunDirection.y;
    float minExposureDawnDusk = 0.0007;
    float maxExposureMidday   = 0.07;
    float dawnElevationThreshold   = 0.0;
    float middayElevationThreshold = 0.6;
    float exposureInterpFactor = smoothstep(dawnElevationThreshold, middayElevationThreshold, sunElevation);
    float dynamicExposure = mix(minExposureDawnDusk, maxExposureMidday, exposureInterpFactor);

    vec3 exposedColor = litColor * dynamicExposure;
    vec3 tonemappedColor = exposedColor / (exposedColor + vec3_splat(1.0));

    // --- Output Final Color ---
    gl_FragColor = vec4(toGamma(tonemappedColor), 1.0);
}