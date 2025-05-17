$input v_position, v_texcoord0

#include "../common/common.sh"
#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse,  0);
SAMPLER2D(s_orm,      1);
SAMPLER2D(s_normal,   2);
SAMPLER2DSHADOW(s_shadowMap, 4); // Use depth comparison sampler for shadow map

uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;
uniform vec4 u_cameraPos;
uniform vec4 u_skyAmbient;

uniform mat4 u_lightMatrix; // Light-space matrix (light VP)

#define PI 3.14159265359

// GGX normal distribution
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = max(roughness, 0.05);
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Geometry term approximation (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.001);
    float NdotL = max(dot(N, L), 0.001);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel approximation using Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (vec3(1.0, 1.0, 1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}
void main()
{
    vec2 uv = v_texcoord0;

    // --- Texture Sampling ---
    vec3 albedo     = bgfxTexture2D(s_diffuse, uv).rgb;
    vec3 orm        = bgfxTexture2D(s_orm, uv).rgb;
    vec3 normalMap  = bgfxTexture2D(s_normal, uv).rgb * 2.0 - 1.0;

    float ao        = orm.r;
    float roughness = max(orm.g, 0.05);  // Prevent divide by zero
    float metalness = orm.b;

    // --- Lighting Setup ---
    vec3 N = normalize(normalMap);
    vec3 V = normalize(u_cameraPos.xyz - v_position.xyz); 
    vec3 L = normalize(u_sunDirection.xyz);
    vec3 H = normalize(V + L);

    // Fresnel reflectance at normal incidence
    vec3 F0 = mix(vec3(0.04, 0.04, 0.04), albedo, metalness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    float NdotL = max(dot(N, L), 0.001);
    float NdotV = max(dot(N, V), 0.001);
    float denom = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = (D * G * F) / denom;

    // --- Shadow Mapping ---
    vec3 worldPos = v_position.xyz;
    vec4 shadowCoord = mul(u_lightMatrix, vec4(v_position.x, v_position.y, v_position.z, 1.0));

    //  Normalize coordinates
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xyz = shadowCoord.xyz * 0.5 + 0.5;  // Convert NDC [-1,1] to [0,1]
    shadowCoord.z -= 0.005;  // Optional bias

    // Sample shadow map using projected coordinates
    float shadowFactor = bgfxShadow2DProj(s_shadowMap, shadowCoord);


    // --- Final Lighting ---
    vec3 kS = F;
    vec3 kD = (vec3(1.0, 1.0, 1.0) - kS) * (1.0 - metalness);


    vec3 Lo = (kD * albedo / PI + specular) * u_sunLuminance.rgb * NdotL * ao * shadowFactor;

    // Ambient + specular ambient
    vec3 ambientSky = u_skyAmbient.rgb * albedo * ao;
    vec3 ambientSpec = F * u_skyAmbient.rgb * ao * 0.1;

    vec3 finalColor = ambientSky + Lo + ambientSpec;

    gl_FragColor = vec4(finalColor, 1.0);


}
