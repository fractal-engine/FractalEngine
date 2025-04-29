$input v_skyColor, v_screenPos, v_viewDir

// Uniforms for sky rendering
uniform vec4 u_parameters;     // x: sun size, y: bloom, z: exposure, w: time
uniform vec4 u_sunDirection;   // Sun direction (xyz), w unused
uniform vec4 u_sunLuminance;   // Sun color/intensity (xyz), w unused

#include "../common/common.sh"

// Simple hash function for noise
float hash(vec2 p)
{
    p = frac(p * 0.3183099 + vec2(0.71, 0.113));
    return frac(43758.5453 * p.x * p.y);
}

// 2D Value noise
float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = frac(p);
    float a = hash(i);
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

void main()
{
    float3 viewDir = normalize(v_viewDir);
    float3 lightDir = normalize(u_sunDirection.xyz);
    
    // Distance to sun
    float dist = 2.0 * (1.0 - dot(viewDir, lightDir));
    float size2 = u_parameters.x * u_parameters.x;

    float sunGlow = exp(-dist / u_parameters.y / size2) + step(dist, size2);
    float sunIntensity = min(sunGlow * sunGlow, 1.0);

    // Horizon blending
    float horizon = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);

    // Base sky color transitions
    float sunAmount = clamp(dot(viewDir, lightDir), 0.0, 1.0);
    float3 baseSky = lerp(float3(0.9, 0.5, 0.2), float3(0.4, 0.6, 0.95), pow(sunAmount, 1.5));

    // Procedural cloud generation (moving with time)
    float cloudNoise = noise(v_screenPos * 4.0 + u_parameters.ww); // u_time.w
    float cloudMask = smoothstep(0.5, 0.6, cloudNoise);

    // Blend clouds over sky
    float3 color = lerp(baseSky, float3(1.0, 1.0, 1.0), cloudMask * 0.5);
    
    // Add sun intensity
    color += sunIntensity * u_sunLuminance.xyz;

    // Apply gamma correction if needed
    color = toGamma(color);

    // Output
    gl_FragColor = float4(color * horizon, 1.0);
}
