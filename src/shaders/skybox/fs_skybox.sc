$input v_viewDir

uniform vec4 u_parameters;       // x: sun size, y: bloom, z: exposure, w: time
uniform vec4 u_sunDirection;     // xyz = direction to sun
uniform vec4 u_sunLuminance;     // rgb = sun color
uniform vec4 u_scatterParams;    // x: g (Mie phase)
uniform vec4 u_betaR;            // xyz: Rayleigh coefficients
uniform vec4 u_betaM;            // xyz: Mie coefficients

#include "../common/common.sh"

#define PI 3.14159265359

// ------ scattering helpers ------

float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

float hgPhase(float cosTheta, float g) {
    return (1.0 / (4.0 * PI)) * ((1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5));
}

// ------ noise and fbm ------

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
    float result = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; ++i) {
        result += noise(p) * amp;
        p *= 2.0;
        amp *= 0.5;
    }
    return result;
}

// ------ main shading ------

void main() {
    float3 viewDir = normalize(v_viewDir);
    float3 sunDir = normalize(u_sunDirection.xyz);

    // ------ sun disk & glow ------

    float dotSun = dot(viewDir, sunDir);
    float dist = 2.0 * (1.0 - dotSun);
    float sunSize = u_parameters.x; 
    float bloom = u_parameters.y;   

    float sunDisk = smoothstep(sunSize * 0.05, 0.0, dist - sunSize * sunSize * 0.5);
    float glowFalloff = exp(-pow(dist / (sunSize * bloom * 0.5), 2.0));
    float sunGlow = glowFalloff * 4.0 + sunDisk;
    float sunIntensity = saturate(sunGlow);

    float sunDiskFactor = sunDisk; 
    float sunGlowFactor = sunGlow; 

    // ------ spherical coordinates (for procedural sampling) ------

    float longitude = atan2(viewDir.x, viewDir.z);
    float latitude  = asin(viewDir.y);
    float2 uv = float2(longitude / (2.0 * PI) + 0.5, latitude / PI + 0.5);

    // ------ time-of-day factors ------

    float sunH = sunDir.y;
    float dayNightFactor = smoothstep(-0.15, 0.2, sunH);
    float sunriseSunsetFactor = smoothstep(0.0, 0.2, sunH) * (1.0 - smoothstep(0.1, 0.3, sunH));
    sunriseSunsetFactor = saturate(sunriseSunsetFactor * 3.0);

    // ------ base sky colors ------

    float3 nightZenithCol   = float3(0.01, 0.02, 0.05);
    float3 nightHorizonCol  = float3(0.03, 0.04, 0.12);
    float3 dayZenithCol     = float3(0.20, 0.45, 0.85);
    float3 dayHorizonCol    = float3(0.50, 0.70, 0.95);
    float3 sunriseZenithCol = float3(0.4, 0.25, 0.1);
    float3 sunriseHorizonCol= float3(0.9, 0.4, 0.15);

    float3 currentZenithColor = mix(nightZenithCol, dayZenithCol, dayNightFactor);
    float3 currentHorizonColor = mix(nightHorizonCol, dayHorizonCol, dayNightFactor);

    currentZenithColor = mix(currentZenithColor, sunriseZenithCol, sunriseSunsetFactor);
    currentHorizonColor = mix(currentHorizonColor, sunriseHorizonCol, sunriseSunsetFactor);

    // ------ vertical sky gradient ------

    float viewElevationFactor = pow(saturate(viewDir.y * 0.5 + 0.5), 0.6);
    float3 skyGradientColor = mix(currentHorizonColor, currentZenithColor, viewElevationFactor);
    

    // ------ sun disk color ------

    float3 sunDiskVisualColorDay    = float3(1.0, 0.98, 0.9);
    float3 sunDiskVisualColorSunset = float3(1.0, 0.6, 0.2);
    float3 sunDiskVisualColorNight  = float3(0.8, 0.4, 0.1);

    float3 currentSunDiskVisualColor = mix(sunDiskVisualColorNight, sunDiskVisualColorDay, dayNightFactor);
    currentSunDiskVisualColor = mix(currentSunDiskVisualColor, sunDiskVisualColorSunset, sunriseSunsetFactor);
    currentSunDiskVisualColor *= (u_sunLuminance.rgb * 0.7 + 0.3);

    // ------ clouds ------

    float2 cloudUV = uv + float2(u_parameters.w * 0.002, 0.0);
    float cloudNoise = fbm(cloudUV * 10.0);
    float cloudMask = smoothstep(0.5, 0.7, cloudNoise);
    float cloudVisibility = dayNightFactor * (1.0 - sunriseSunsetFactor * 0.5);
    float3 cloudColor = float3(1.0, 1.0, 1.0);
    float3 sunLightForClouds = mix(float3(1.0, 1.0, 1.0), u_sunLuminance.rgb, sunriseSunsetFactor * 0.8);
    cloudColor *= sunLightForClouds;

    float3 blendedSky = mix(skyGradientColor, cloudColor, cloudMask * cloudVisibility * 0.3);

    // ------ sky scattering effects ------

    float3 skyColor = blendedSky;
    float cosTheta = dot(viewDir, sunDir);

    float rayleigh = rayleighPhase(cosTheta);
    float mie = hgPhase(cosTheta, u_scatterParams.x); // g from uniform

    float3 betaR = u_betaR.rgb;
    float3 betaM = u_betaM.rgb;

    float3 scattering = rayleigh * betaR + mie * betaM;
    scattering *= u_sunLuminance.rgb * 20.0;
    skyGradientColor += scattering * 1.0; 
    skyColor += scattering;

    // ------ stars ------

    float starVisibility = 1.0 - dayNightFactor;
    starVisibility *= (1.0 - sunriseSunsetFactor);
    float starPattern = pow(noise(uv * 512.0), 100.0);
    float3 starColorEffect = float3(starPattern, starPattern, starPattern) * starVisibility;
    skyColor += starColorEffect;

    // ------ sun addition ------

    skyColor += currentSunDiskVisualColor * glowFalloff * 4.0;

    // ------ final tone mapping & output ------

    skyColor *= mix(0.8, 1.8, dayNightFactor);
    float horizonMask = saturate(viewDir.y * 0.5 + 0.5); // Fade to black under horizon
    gl_FragColor = float4(toGamma(skyColor) * horizonMask, 1.0);
   // gl_FragColor = float4(toGamma(scattering * 100.0), 1.0); // Debug

}
