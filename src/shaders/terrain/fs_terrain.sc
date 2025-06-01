/* The following shader uses functions from the bgfx shader library. Specifically, it uses the bgfx_shader.sh and fs_sms_shadow.sh files.
 * Copyright 2013-2014 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

$input v_out_uv, v_out_worldPos, v_out_shadowCoord, v_out_viewVec, v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom
#include <bgfx_shader.sh>
#include "../common/common.sh"

// --- Texture Samplers ---
SAMPLER2D(s_diffuse,      0);   // Terrain base color
SAMPLER2D(s_orm,          1);   // Occlusion, Roughness, Metalness
SAMPLER2D(s_normal,       2);   // Terrain normal map
SAMPLER2DSHADOW(s_shadowMap, 4); // Shadow map from directional light

// --- Uniforms ---
uniform vec4 u_sunDirection;    // xyz = sun direction, w = unused
uniform vec4 u_sunLuminance;    // xyz = sun color, w = intensity
uniform vec4 u_cameraPos;       // Camera position in world space
uniform vec4 u_skyAmbient;      // Ambient lighting from sky


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
    vec2 worldXZ = v_out_worldPos.xz;


    // --- Terrain Textures ---

    vec3 albedo     = texture2D(s_diffuse, uv).rgb;
    vec3 orm        = texture2D(s_orm, uv).rgb;
    vec3 normalMap  = texture2D(s_normal, uv).rgb * 2.0 - 1.0;


    // --- Unpack Terrain ORM ---

    float ao         = orm.r;
    float roughness  = clamp(orm.g, 0.04, 1.0);
    float metalness  = orm.b;

    // --- Construct TBN and Normals ---

    float3 T = normalize(v_out_worldTangent);
    float3 B = normalize(v_out_worldBitangent);
    float3 N = normalize(v_out_worldNormalGeom);
    vec3 N_terrain = normalize(mul(mat3(T, B, N), normalMap));


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
    vec3 F_T = FresnelSchlick(max(dot(V, H), 0.0), F0_terrain);
    vec3 specularT = (D_T * G_T * F_T) / (4.0 * NdotL_T * NdotV_T + 0.0001);
    vec3 kS_T = F_T;
    vec3 kD_T = (vec3_splat(1.0) - kS_T) * (1.0 - metalness);

    // --- Shadow Mapping ---

    float NdotL = max(dot(N_terrain, L), 0.0);
    float bias = max(0.0025 * (1.0 - NdotL), 0.0001);
    float shadow = PCF(s_shadowMap, v_out_shadowCoord, bias, vec2_splat(1.0 / 2048.0)); // use larger texel size

    // --- Fake AO (based on slope) ---

    float normalAO = clamp(dot(N_terrain, vec3(0.0, 1.0, 0.0)), 0.2, 1.0);
    float aoCombined = ao * normalAO;

    // --- Terrain Lighting Components ---

    vec3 sunlight = u_sunLuminance.rgb * u_sunLuminance.w;
    vec3 Lo_terrain = (kD_T * albedo / PI + specularT) * sunlight * NdotL_T * shadow;
    vec3 ambientT = u_skyAmbient.rgb * albedo * aoCombined;

    // --- Final Blending ---

    vec3 finalColor = Lo_terrain + ambientT;

    // --- Exposure control + Reinhard ToneMapping ---

    float sunElevation = u_sunDirection.y; 

    // Define exposure range
    float minExposureDawnDusk = 0.0007; 
    float maxExposureMidday   = 0.07;  

    // Define sun elevation thresholds for the transition.
    float dawnElevationThreshold   = 0.0;  // Sun at/near horizon
    float middayElevationThreshold = 0.6;  // Sun elevation at which maxExposureMidday is reached
                                           

    // Calculate an interpolation factor based on sun elevation.
  
    float exposureInterpFactor = smoothstep(dawnElevationThreshold, middayElevationThreshold, sunElevation);

    // Interpolate the exposure value
    float dynamicExposure = mix(minExposureDawnDusk, maxExposureMidday, exposureInterpFactor);

    // --- Tonemapping (before Gamma) ---
    vec3 exposedColor = finalColor * dynamicExposure;
    vec3 tonemappedColor = exposedColor / (exposedColor + vec3_splat(1.0)); // Reinhard

    // --- Output Final Color ---
    gl_FragColor = vec4(toGamma(tonemappedColor), 1.0);
}


