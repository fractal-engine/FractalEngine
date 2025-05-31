$input v_out_uv, v_out_worldPos, v_out_shadowCoord, v_out_viewVec, v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom

#include <bgfx_shader.sh>
#include "../common/common.sh"

// --- Samplers ---
SAMPLER2D(s_waterNorm, 6);
SAMPLER2DSHADOW(s_shadowMap, 4);

// --- Uniforms ---
uniform vec4 u_time;
uniform vec4 u_cameraPos;
uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;
uniform vec4 u_skyAmbient;
uniform vec4 u_waterColor;

#define PI 3.14159265359

// --- PBR GGX functions (fully inlined) ---

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
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (vec3_splat(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    // --- Animated UV flow ---
    vec2 flow1 = vec2(0.4, 0.2);
    vec2 flow2 = vec2(-0.3, 0.6);
    float t = u_time.x;

    vec2 uv1 = fract(v_out_uv * 6.0 + flow1 * t * 0.1);
    vec2 uv2 = fract(v_out_uv * 7.0 + flow2 * t * 0.1);

    // --- Normal Map ---
    vec3 n1 = texture2D(s_waterNorm, uv1).rgb * 2.0 - 1.0;
    vec3 n2 = texture2D(s_waterNorm, uv2).rgb * 2.0 - 1.0;
    vec3 normalTex = normalize(n1 + n2);

    // --- Transform normal to world space ---
    mat3 tbn = mat3(v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom);
    vec3 N = normalize(mul(tbn, normalTex));
    vec3 V = normalize(u_cameraPos.xyz - v_out_worldPos);
    vec3 L = normalize(u_sunDirection.xyz);
    vec3 H = normalize(V + L);

    // --- Fresnel ---
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    // --- Specular GGX ---
    float roughness = 0.05;
    float NdotH = max(dot(N, H), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(NdotH, vec3_splat(0.04));
    vec3 spec = (D * G * F) / (4.0 * max(NdotV * NdotL, 0.001));

    // --- Lighting ---
    float shadow = 1.0; // optional: use PCF(s_shadowMap, ...) if needed
    vec3 ambient = u_skyAmbient.rgb * u_waterColor.rgb;
    vec3 light   = (spec + u_waterColor.rgb / PI) * u_sunLuminance.rgb * NdotL * shadow;

    // --- Sparkle / Glint ---
    float glossPower = 32.0; // or 64.0 — stay within GPU-safe limits
    float glint = pow(NdotL, glossPower) * fresnel;

    vec3 sparkle = vec3(glint, glint, glint);

    vec3 finalColor = ambient + light + sparkle;

    gl_FragColor = vec4(toGamma(finalColor), 1.0);
}
