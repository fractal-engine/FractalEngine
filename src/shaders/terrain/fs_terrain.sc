/* The following shader uses functions from the bgfx shader library. Specifically, it uses the bgfx_shader.sh and fs_sms_shadow.sh files.
 * Copyright 2013-2014 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

$input v_out_uv, v_out_worldPos, v_out_shadowCoord, v_out_viewVec, v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom, v_out_oasisMask

#include <bgfx_shader.sh>
#include "../common/common.sh"

// --- Texture Samplers ---
SAMPLER2D(s_diffuse,      0);   // Terrain base color
SAMPLER2D(s_orm,          1);   // Occlusion, Roughness, Metalness
SAMPLER2D(s_normal,       2);   // Terrain normal map
SAMPLER2D(s_waterNormal,  5);   // Water surface normal map (for reflections)
SAMPLER2DSHADOW(s_shadowMap, 4); // Shadow map from directional light

// --- Uniforms ---
uniform vec4 u_sunDirection;    // xyz = sun direction, w = unused
uniform vec4 u_sunLuminance;    // xyz = sun color, w = intensity
uniform vec4 u_cameraPos;       // Camera position in world space
uniform vec4 u_skyAmbient;      // Ambient lighting from sky
uniform vec4 u_time;            // x = time in seconds (used for water animation)

#define PI 3.14159265359

// --- PCF Shadow Filtering ---
float PCF(sampler2DShadow shadowSampler, vec4 shadowCoord, float bias, vec2 texelSize) {
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;

    // Check if shadow projection is outside valid range
    if (any(lessThan(projCoords.xy, vec2_splat(0.0))) ||
        any(greaterThan(projCoords.xy, vec2_splat(1.0))) ||
        projCoords.z > 1.0)
        return 1.0;

    float result = 0.0;
    // 4x4 kernel of PCF samples
    for (float y = -1.5; y <= 1.5; y += 1.0)
        for (float x = -1.5; x <= 1.5; x += 1.0)
            result += shadow2D(shadowSampler, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias));

    return result / 16.0;
}

// --- GGX Microfacet Distribution and Geometry Functions ---
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// --- Schlick-GGX Approximation  ---
float GeometrySchlickGGX(float Ndot, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return Ndot / (Ndot * (1.0 - k) + k);
}

// --- Smith-GGX Formulation ---
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0001), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0001), roughness);
}

// --- Fresnel Schlick Approximation ---
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (vec3_splat(1.0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


// --- Main Fragment Shader ---
void main() {
    // --- UV Coordinates ---
    vec2 uv = v_out_uv * 150.0; // Terrain UV tiling scale
    vec2 flowDir = vec2(0.3, 0.7); // Water flow direction
    vec2 worldXZ = v_out_worldPos.xz;
    float time = u_time.x;

    // --- Dual-layer Animated Water UVs ---
    vec2 flowDir1 = vec2(0.3, 0.7);
    vec2 flowDir2 = vec2(-0.6, 0.2);
    vec2 uv1 = worldXZ * 0.15 + flowDir1 * time * 0.1;
    vec2 uv2 = worldXZ * 0.18 + flowDir2 * time * 0.1;

    // --- Terrain Textures ---
    vec3 albedo     = texture2D(s_diffuse, uv).rgb;
    vec3 orm        = texture2D(s_orm, uv).rgb;
    vec3 normalMap  = texture2D(s_normal, uv).rgb * 2.0 - 1.0;

    // --- Water Normals (Blended flow layers) ---
    vec3 n1 = texture2D(s_waterNormal, uv1).rgb * 2.0 - 1.0;
    vec3 n2 = texture2D(s_waterNormal, uv2).rgb * 2.0 - 1.0;
    vec3 waterNormal = normalize(n1 + n2); // Combine both flows

    // --- Water Parameters ---
    vec3 waterColor   = vec3(0.06, 0.13, 0.15);
    float waterMetal  = 0.0;
    float waterRough  = 0.05;
    float waterAO     = 1.0;

    // --- Unpack Terrain ORM ---
    float ao         = orm.r;
    float roughness  = clamp(orm.g, 0.04, 1.0);
    float metalness  = orm.b;

    // --- Construct TBN and Normals ---
    float3 T = normalize(v_out_worldTangent);
    float3 B = normalize(v_out_worldBitangent);
    float3 N = normalize(v_out_worldNormalGeom);
    vec3 N_terrain = normalize(mul(mat3(T, B, N), normalMap));
    vec3 N_water   = normalize(mul(mat3(T, B, N), waterNormal));

    // --- View & Light Vectors ---
    vec3 V = normalize(u_cameraPos.xyz - v_out_worldPos); // View vector
    vec3 L = normalize(u_sunDirection.xyz);               // Light direction
    vec3 H = normalize(V + L);                            // Half vector

    // --- Terrain PBR Lighting ---
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

    // --- Water PBR Lighting ---
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

    // --- Shadow Mapping ---
    float shadow = PCF(s_shadowMap, v_out_shadowCoord, 0.005, vec2_splat(1.0 / 2048.0));

    // --- Fake AO (based on slope) ---
    float normalAO = clamp(dot(N_terrain, vec3(0.0, 1.0, 0.0)), 0.2, 1.0);
    float aoCombined = ao * normalAO;

    // --- Terrain Lighting Components ---
    vec3 Lo_terrain = (kD_T * albedo / PI + specularT) * u_sunLuminance.rgb * NdotL_T * shadow;
    vec3 ambientT = u_skyAmbient.rgb * albedo * aoCombined;

    // --- Water Lighting Components ---
    vec3 Lo_water = (kD_W * waterColor / PI + specularW) * u_sunLuminance.rgb * NdotL_W * shadow;
    vec3 ambientW = u_skyAmbient.rgb * waterColor * waterAO;

    // --- Fresnel and Optical Effects for Water ---
    float fresnel = pow(1.0 - max(dot(N_water, V), 0.0), 3.0) * 0.5;
    vec3 skyTint = mix(vec3(0.1, 0.2, 0.3), u_skyAmbient.rgb, fresnel * 0.8);
    vec2 refractOffset = N_water.xz * 0.03 * fresnel;
    vec3 terrainRefract = texture2D(s_diffuse, uv + refractOffset).rgb * 0.6;
    float scatter = smoothstep(0.0, 0.2, v_out_oasisMask);
    vec3 scatterTint = mix(vec3(0.0, 0.4, 0.6), vec3(0.05, 0.6, 0.8), scatter);

    // --- Final Water Composition ---
    vec3 waterFinal = mix(terrainRefract, Lo_water + skyTint + ambientW, fresnel);
    waterFinal = mix(waterFinal, scatterTint, 0.3); // Inject color variation

    // --- (Optional) Screen Space Reflection Placeholder ---
    // vec2 screenUV = gl_FragCoord.xy / u_resolution.xy;
    // vec3 reflDir = reflect(-V, N_water);
    // vec2 reflOffset = reflDir.xy * 0.05;
    // vec3 screenRefl = texture2D(u_sceneColor, screenUV + reflOffset).rgb;
    // skyTint = mix(skyTint, screenRefl, 0.4);

    // --- Final Blending Between Terrain and Water ---
    float mask = v_out_oasisMask;
    vec3 finalColor = mix(Lo_terrain + ambientT, waterFinal, mask);

    // --- Output Final Color ---
    gl_FragColor = vec4(toGamma(finalColor), 1.0);
}


