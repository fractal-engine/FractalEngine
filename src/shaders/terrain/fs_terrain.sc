$input v_position, v_texcoord0, v_worldPos, v_view, v_shadowCoord

#include "../common/common.sh"
#include <bgfx_shader.sh>

// ---------------------------------------------
// Samplers
// ---------------------------------------------
SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_orm,     1);
SAMPLER2D(s_normal,  3);
SAMPLER2D(s_shadowMap, 4); 

// ---------------------------------------------
// Uniforms
// ---------------------------------------------
uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;
uniform vec4 u_cameraPos;
uniform vec4 u_skyAmbient;
uniform mat4 u_lightMatrix[1];

#define PI 3.14159265359

// ---------------------------------------------
// Shadow Sampling (Manual PCF)
// ---------------------------------------------
float getShadow(sampler2D shadowMap, vec4 shadowCoord)
{
    vec3 projCoord = shadowCoord.xyz / shadowCoord.w;

    // Early out: outside shadow map bounds
    if (projCoord.x < 0.0 || projCoord.x > 1.0 ||
        projCoord.y < 0.0 || projCoord.y > 1.0 ||
        projCoord.z < 0.0 || projCoord.z > 1.0)
    {
        return 1.0;
    }

    float result = 0.0;
    float bias = 0.005;
    vec2 texelSize = vec2_splat(1.0 / 2048.0); // match shadow map resolution

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleUV = projCoord.xy + offset;

            vec4 packedDepth = texture2D(shadowMap, sampleUV);
            float sampledDepth = unpackRgbaToFloat(packedDepth);  
            float currentDepth = projCoord.z - bias;

            if (currentDepth <= sampledDepth)
                result += 1.0;
        }
    }

    return result / 9.0;
}

// ---------------------------------------------
// GGX PBR Lighting
// ---------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = max(roughness, 0.05);
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

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

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (vec3_splat(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// ---------------------------------------------
// Main Fragment Shader
// ---------------------------------------------
void main()
{
    vec2 uv = v_texcoord0;

    // --- Sample Material Maps ---
    vec3 albedo     = bgfxTexture2D(s_diffuse, uv).rgb;
    vec3 orm        = bgfxTexture2D(s_orm, uv).rgb;
    vec3 normalMap  = bgfxTexture2D(s_normal, uv).rgb * 2.0 - 1.0;

    float ao        = orm.r;
    float roughness = max(orm.g, 0.05);
    float metalness = orm.b;

    // --- Lighting Vectors ---
    vec3 N = normalize(normalMap);
    vec3 V = normalize(u_cameraPos.xyz - v_position.xyz);
    vec3 L = normalize(u_sunDirection.xyz);
    vec3 H = normalize(V + L);

    // --- BRDF Calculation ---
    vec3 F0 = mix(vec3_splat(0.04), albedo, metalness);
    vec3 F  = FresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    float NdotL = max(dot(N, L), 0.001);
    float NdotV = max(dot(N, V), 0.001);
    float denom = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = (D * G * F) / denom;

    vec3 kS = F;
    vec3 kD = (vec3_splat(1.0) - kS) * (1.0 - metalness);

    // --- Shadow Factor ---
    float shadowFactor = getShadow(s_shadowMap, v_shadowCoord);

    // --- Final Lighting ---
    vec3 Lo = (kD * albedo / PI + specular) * u_sunLuminance.rgb * NdotL * ao * shadowFactor;
    vec3 ambientSky = u_skyAmbient.rgb * albedo * ao;
    vec3 ambientSpec = F * u_skyAmbient.rgb * ao * 0.1;

    vec3 finalColor = ambientSky + Lo + ambientSpec;

    // Gamma Correct
    gl_FragColor = vec4(toGamma(finalColor), 1.0);
}
