$input v_viewDir

// Uniforms from CPU
uniform vec4 u_parameters;     // x: sun size, y: bloom, z: exposure, w: time
uniform vec4 u_sunDirection;   // xyz = direction to sun
uniform vec4 u_sunLuminance;   // rgb = sun color

#include "../common/common.sh"

// Hash for procedural noise
float hash(vec2 p) {
    p = frac(p * 0.3183099 + vec2(0.71, 0.113));
    return frac(43758.5453 * p.x * p.y);
}

// 2D value noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = frac(p);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Fractal Brownian Motion (layered noise)
float fbm(vec2 p) {
    float f = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; ++i) {
        f += noise(p) * amp;
        p *= 2.0;
        amp *= 0.5;
    }
    return f;
}

void main()
{
    float3 viewDir = normalize(v_viewDir);
    float3 lightDir = normalize(u_sunDirection.xyz);

    float sunDist = 2.0 * (1.0 - dot(viewDir, lightDir));
    float size2 = u_parameters.x * u_parameters.x;

    float sunGlow = exp(-sunDist / u_parameters.y / size2) + step(sunDist, size2);
    float sunIntensity = min(sunGlow * sunGlow, 1.0);

    float horizon = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);
    float sunAmount = clamp(dot(viewDir, lightDir), 0.0, 1.0);
    float dayFactor = (sin(u_parameters.w) + 1.0) * 0.5;

    // Elevation-based horizon gradient
    float elevation = clamp(viewDir.y, -1.0, 1.0);
    float verticalBlend = smoothstep(-0.2, 0.5, elevation);

    // Sky colors
    float3 nightColor   = float3(0.05, 0.05, 0.2);
    float3 sunriseColor = float3(0.9, 0.5, 0.2);
    float3 dayColor     = float3(0.4, 0.6, 0.95);

    float3 skyBlend = mix(sunriseColor, dayColor, pow(sunAmount, 1.5));
    float3 timeSky  = mix(nightColor, skyBlend, dayFactor);
    float3 baseSky  = mix(nightColor, timeSky, verticalBlend);

    // Fractal cloud UV and mask
    float2 cloudUV = viewDir.xy * 5.0 + float2(u_parameters.w * 0.1, u_parameters.w * 0.1);
    float cloudNoise = fbm(cloudUV);
    float cloudMask = smoothstep(0.5, 0.7, cloudNoise);

    float3 color = mix(baseSky, float3(1.0, 1.0, 1.0), cloudMask * 0.5);

    // Apply sun
    color += sunIntensity * u_sunLuminance.xyz;

    // Global brightness by time of day
    color *= mix(0.1, 1.0, dayFactor);  // darker at night

    color = toGamma(color);
    gl_FragColor = vec4(color * horizon, 1.0);
}
