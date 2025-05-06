$input v_viewDir

uniform vec4 u_parameters;     // x: sun size, y: bloom, z: exposure, w: time
uniform vec4 u_sunDirection;   // xyz = direction to sun
uniform vec4 u_sunLuminance;   // rgb = sun color

#include "../common/common.sh"
#define PI 3.1415926535897932384626433832795

float hash(vec2 p) {
    p = frac(p * 0.3183099 + vec2(0.71, 0.113));
    return frac(43758.5453 * p.x * p.y);
}

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

    float dotSun = dot(viewDir, lightDir);
    float dist = 2.0 * (1.0 - dotSun);
    float size2 = u_parameters.x * u_parameters.x;

    float sunDisk = smoothstep(0.0025, 0.0, dist - size2 * 0.5);
    float sunGlow = exp(-dist / (u_parameters.y * size2)) + sunDisk;
    float sunIntensity = saturate(sunGlow);

    float horizon = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);
    float sunAmount = clamp(dotSun, 0.0, 1.0);
    float dayFactor = (sin(u_parameters.w) + 1.0) * 0.5;

    float elevation = clamp(viewDir.y, -1.0, 1.0);
    float verticalBlend = smoothstep(-0.2, 0.5, elevation);

    float3 nightColor   = float3(0.02, 0.02, 0.08);
    float3 sunriseColor = float3(1.0, 0.4, 0.1);
    float3 dayColor     = float3(0.4, 0.7, 1.0);

    float3 skyBlend = mix(sunriseColor, dayColor, pow(sunAmount, 1.5));
    float3 timeSky  = mix(nightColor, skyBlend, dayFactor);
    float3 baseSky  = mix(nightColor, timeSky, verticalBlend);

    //UV seamless fix for clouds
    // Convert direction vector to spherical coordinates
    float longitude = atan2(viewDir.x, viewDir.z); // [-PI, PI]
    float latitude  = asin(viewDir.y);             // [-PI/2, PI/2]

    // Map to [0, 1] UV space
    float2 cloudUV = float2(longitude / (2.0 * PI) + 0.5, latitude / PI + 0.5);

    // Scroll clouds in X over time
    cloudUV.x += u_parameters.w * 0.002;

    // Increase frequency to avoid large uniform blobs
    float cloudNoise = fbm(cloudUV * 10.0);

    // Final cloud mask
    float cloudMask = smoothstep(0.5, 0.7, cloudNoise);


    float3 color = mix(baseSky, float3(1.0, 1.0, 1.0), cloudMask * 0.4);
    color += sunIntensity * u_sunLuminance.xyz;
    color *= mix(2.2, 1.0, dayFactor); // Ambient brightening based on time

    color = toGamma(color);
    gl_FragColor = vec4(color * horizon, 1.0);
}