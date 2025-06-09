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
SAMPLER2D(s_grassDiffuse, 9);   // Grass texture set
SAMPLER2D(s_grassORM,     10);
SAMPLER2D(s_grassNormal,  11);

// --- Uniforms ---
uniform vec4 u_sunDirection;        // xyz = sun direction, w = unused
uniform vec4 u_sunLuminance;        // xyz = sun color, w = intensity
uniform vec4 u_cameraPos;           // Camera position in world space
uniform vec4 u_skyAmbient;          // Ambient lighting from sky
uniform vec4 u_slopeBlendParams;    // x: minSlopeDot, y: maxSlopeDot, z,w: unused

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
    vec2 uv = v_out_uv * 150.0;


    // --- Slope Calculation for Grass Blending ---

    vec3 worldUp = vec3(0.0, 1.0, 0.0);
    float dotNormalUp = dot(normalize(v_out_worldNormalGeom), worldUp);
    float grassBlendFactor = smoothstep(u_slopeBlendParams.x, u_slopeBlendParams.y, dotNormalUp);

    // --- Sample Terrain Textures ---
    vec3 albedo_terrain_only     = texture2D(s_diffuse, uv).rgb;
    vec3 orm_terrain_only        = texture2D(s_orm, uv).rgb;
    vec3 normalMap_terrain_only  = texture2D(s_normal, uv).rgb * 2.0 - 1.0;

    // --- Unpack Terrain ORM ---
    float ao_terrain_only         = orm_terrain_only.r;
    float roughness_terrain_only  = clamp(orm_terrain_only.g, 0.04, 1.0);
    float metalness_terrain_only  = orm_terrain_only.b;

    // --- Sample Grass Textures ---
    vec3 albedo_grass_only       = texture2D(s_grassDiffuse, uv).rgb * 7;
    vec3 orm_grass_only          = texture2D(s_grassORM, uv).rgb;
    vec3 normalMap_grass_only    = texture2D(s_grassNormal, uv).rgb * 2.0 - 1.0;

    
    // --- Unpack Grass ORM ---
    float ao_grass_only           = orm_grass_only.r;
    float roughness_grass_only    = clamp(orm_grass_only.g, 0.04, 1.0);
    float metalness_grass_only    = orm_grass_only.b;


    // --- Blend Material Properties (Using grassBlendFactor) ---

    vec3 albedo     = mix(albedo_terrain_only, albedo_grass_only, grassBlendFactor);
    float ao        = mix(ao_terrain_only, ao_grass_only, grassBlendFactor);
    float roughness = mix(roughness_terrain_only, roughness_grass_only, grassBlendFactor);
    float metalness = mix(metalness_terrain_only, metalness_grass_only, grassBlendFactor);

    // --- Construct TBN matrix ---

    float3 T_geom = normalize(v_out_worldTangent);
    float3 B_geom = normalize(v_out_worldBitangent);
    float3 N_geom_surf = normalize(v_out_worldNormalGeom); // Geometric surface normal
    mat3 tbn = mat3(T_geom, B_geom, N_geom_surf);

    // --- Blend Normals ---

    // 1. Transform terrain-only tangent-space normal to world space
    vec3 N_world_TERRAIN = normalize(mul(tbn, normalMap_terrain_only));
    // 2. Transform grass-only tangent-space normal to world space
    vec3 N_world_GRASS   = normalize(mul(tbn, normalMap_grass_only));
    // 3. Blend the two world-space normals. This is the final normal for lighting.
    vec3 N_final_blended = normalize(mix(N_world_TERRAIN, N_world_GRASS, grassBlendFactor));


    // --- Calculate Half Vector H from View & Light Vectors ---
    vec3 V = normalize(u_cameraPos.xyz - v_out_worldPos);
    vec3 L_dir = normalize(u_sunDirection.xyz); 
    vec3 H = normalize(V + L_dir);                          // when N aligns with H, we get strongest specular reflection

    // --- PBR Lighting (Using BLENDED properties and N_final_blended) ---

    vec3 F0_blended = mix(vec3_splat(0.04), albedo, metalness); // Use blended albedo and metalness

    float NdotL_blended = max(dot(N_final_blended, L_dir), 0.0); // Use blended Normal
    float NdotV_blended = max(dot(N_final_blended, V), 0.0);     // Use blended Normal
  

    float D_blended = DistributionGGX(N_final_blended, H, roughness);      // Use blended N and roughness
    float G_blended = GeometrySmith(N_final_blended, V, L_dir, roughness); // Use blended N and roughness
    vec3  F_blended = FresnelSchlick(max(dot(V, H), 0.0), F0_blended);   // Use blended F0

    vec3 specular_numerator = D_blended * G_blended * F_blended;
    float specular_denominator = 4.0 * NdotV_blended * NdotL_blended + 0.0001;
    vec3 specular_blended = specular_numerator / specular_denominator;
    
    vec3 kS_blended = F_blended;
    vec3 kD_blended = (vec3_splat(1.0) - kS_blended) * (1.0 - metalness); // Use blended metalness

    // --- Shadow Mapping ---
    // **Use NdotL_blended for bias calculation**
    float shadow_bias = max(0.0025 * (1.0 - NdotL_blended), 0.0001);
    float shadow = PCF(s_shadowMap, v_out_shadowCoord, shadow_bias, vec2_splat(1.0 / 2048.0));

    // --- Fake AO (based on slope of N_final_blended and blended 'ao' map value) ---
    float normalAO_val = clamp(dot(N_final_blended, vec3(0.0, 1.0, 0.0)), 0.2, 1.0); // Use blended N
    float aoCombined = ao * normalAO_val; // Use blended 'ao' from map

    // --- Lighting Components (using blended properties) ---
    vec3 sunlight = u_sunLuminance.rgb * u_sunLuminance.w;
    vec3 directLighting = (kD_blended * albedo / PI + specular_blended) * sunlight * NdotL_blended * shadow;
    vec3 ambientLighting = u_skyAmbient.rgb * albedo * aoCombined;

    // --- Final Color  ---
    vec3 litColor = directLighting + ambientLighting;

    // --- Exposure control + Reinhard ToneMapping  ---
    float sunElevation = u_sunDirection.y;
    float minExposureDawnDusk = 0.0007;     // Manually adjust these values
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

