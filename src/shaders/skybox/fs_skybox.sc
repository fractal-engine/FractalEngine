$input v_viewDir

uniform vec4 u_parameters;     // x: sun size, y: bloom, z: exposure, w: time
uniform vec4 u_sunDirection;   // xyz = direction to sun
uniform vec4 u_sunLuminance;   // rgb = sun color

#include "../common/common.sh"

#define PI 3.14159265359

// Hash, noise, fbm functions (unchanged)
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

void main() {
    float3 viewDir = normalize(v_viewDir);
    float3 sunDir = normalize(u_sunDirection.xyz);

    // --------------------------------------------
    // SUN DISK & GLOW

    float dotSun = dot(viewDir, sunDir);
    float dist = 2.0 * (1.0 - dotSun); // Distance to the sun - angular distance approximation
    float sunSize = u_parameters.x; // 0.008
    float bloom = u_parameters.y;   // 3.0

    // Make sun disk size relative to its apparent size, not fixed angular size

    float sunDisk = smoothstep(sunSize * 0.05, 0.0, dist - sunSize * sunSize * 0.5);
    float sunGlow = exp(-dist / (bloom * sunSize * sunSize)) + sunDisk; // Adjusted bloom sensitivity
    float sunIntensity = saturate(sunGlow);

    // variables for sun disk and glow to change color based on time
    float sunDiskFactor = sunDisk; 
    float sunGlowFactor = sunGlow; 

    // --------------------------------------------
    // SPHERICAL COORDINATES (Equirectangular for noise)

    float longitude = atan2(viewDir.x, viewDir.z);
    float latitude  = asin(viewDir.y);
    float2 uv = float2(longitude / (2.0 * PI) + 0.5, latitude / PI + 0.5);

    // --------------------------------------------
    // DAY/NIGHT & SUN ELEVATION FACTORS
    // sunH ranges from -1 to 1

    float sunH = sunDir.y;

    // dayNightFactor: 0 for night (sun well below horizon), 1 for day (sun well above horizon)

    float dayNightFactor = smoothstep(-0.15, 0.2, sunH);

    // sunriseSunsetFactor: peaks when sun is near horizon
    // Strongest when sunH is between 0.0 and 0.2, fades out otherwise

    float sunriseSunsetFactor = smoothstep(0.0, 0.2, sunH) * (1.0 - smoothstep(0.1, 0.3, sunH));
    sunriseSunsetFactor = saturate(sunriseSunsetFactor * 3.0); // Amplify the peak

    // --------------------------------------------
    // SKY COLORS DEFINITIONS

    // Night
    float3 nightZenithCol   = float3(0.01, 0.02, 0.05);
    float3 nightHorizonCol  = float3(0.03, 0.04, 0.12); // Slightly brighter/desaturated for horizon
    // Day
    float3 dayZenithCol     = float3(0.20, 0.45, 0.85); // Brighter blue
    float3 dayHorizonCol    = float3(0.50, 0.70, 0.95); // Paler blue towards horizon
    // Sunrise/Sunset
    float3 sunriseZenithCol = float3(0.4, 0.25, 0.1);  // Orange/reddish zenith
    float3 sunriseHorizonCol= float3(0.9, 0.4, 0.15); // Brighter orange/red horizon

    // --------------------------------------------
    // INTERPOLATE SKY COLORS BASED ON TIME OF DAY
    // Start with night-to-day transition

    float3 currentZenithColor = mix(nightZenithCol, dayZenithCol, dayNightFactor);
    float3 currentHorizonColor = mix(nightHorizonCol, dayHorizonCol, dayNightFactor);

    // Add sunrise/sunset hues
    currentZenithColor = mix(currentZenithColor, sunriseZenithCol, sunriseSunsetFactor);
    currentHorizonColor = mix(currentHorizonColor, sunriseHorizonCol, sunriseSunsetFactor);

    // --------------------------------------------
    // VERTICAL SKY GRADIENT (Pixel's elevation)
    // viewElevationFactor: 0 for horizon/below, 1 for zenith. Power helps shape the gradient.

    float viewElevationFactor = pow(saturate(viewDir.y * 0.5 + 0.5), 0.6);
    float3 skyGradientColor = mix(currentHorizonColor, currentZenithColor, viewElevationFactor);

    // --------------------------------------------
    // DEFINE SUN DISK'S VISUAL COLOR
  
    float3 sunDiskVisualColorDay    = float3(1.0, 0.98, 0.9);  // Bright, slightly off-white for midday sun disk
    float3 sunDiskVisualColorSunset = float3(1.0, 0.6, 0.2);  // Orange/Red for sunset sun disk
    float3 sunDiskVisualColorNight  = float3(0.8, 0.4, 0.1);  // Dimmer, reddish if sun is just below horizon but disk might still "conceptually" contribute a color


    // Interpolate the sun disk's visual color using the SAME time of day factors:

    float3 currentSunDiskVisualColor = mix(sunDiskVisualColorNight, sunDiskVisualColorDay, dayNightFactor);
    currentSunDiskVisualColor = mix(currentSunDiskVisualColor, sunDiskVisualColorSunset, sunriseSunsetFactor);
    
    // Modulate by u_sunLuminance to respect overall light intensity

    currentSunDiskVisualColor *= (u_sunLuminance.rgb * 0.7 + 0.3); // need to adjust multipliers as needed

    // --------------------------------------------
    // CLOUDS (animated & fading with dayNightFactor)

    float2 cloudUV = uv + float2(u_parameters.w * 0.002, 0.0); // Time from u_parameters.w
    float cloudNoise = fbm(cloudUV * 10.0);
    float cloudMask = smoothstep(0.5, 0.7, cloudNoise);

    // Reduce cloud contribution based on dayNightFactor and also slightly by sunrise factor

    float cloudVisibility = dayNightFactor * (1.0 - sunriseSunsetFactor * 0.5);
    float3 cloudColor = float3(1.0, 1.0, 1.0); // Base cloud color
    // Tint clouds slightly by sun color during sunrise/sunset

    float3 sunLightForClouds = mix(float3(1.0,1.0,1.0), u_sunLuminance.rgb, sunriseSunsetFactor * 0.8);
    cloudColor *= sunLightForClouds;

    float3 blendedSky = mix(skyGradientColor, cloudColor, cloudMask * cloudVisibility * 0.3); // Cloud opacity

    // --------------------------------------------
    // ADD SUN LIGHT TO SKY
    
    float3 skyColor = blendedSky;

    // --------------------------------------------
    // STARS — Procedural and visible only when dark

    float starVisibility = 1.0 - dayNightFactor; // Inverse of dayNightFactor
    starVisibility *= (1.0 - sunriseSunsetFactor); // Fade stars during sunrise/sunset too
    float starPattern = pow(noise(uv * 512.0), 100.0); // Sparse tiny specks
    float3 starColorEffect = float3(starPattern, starPattern, starPattern) * starVisibility;
    skyColor += starColorEffect;

    // --------------------------------------------

    float pureGlowFactor = saturate(sunGlowFactor - sunDiskFactor); // Glow outside the hard disk
    skyColor += pureGlowFactor * u_sunLuminance.rgb;

    // Add sun disk (colored by its own modulated visual color) to the sky

    skyColor += sunDiskFactor * currentSunDiskVisualColor;

    // --------------------------------------------
    // FINAL ADJUSTMENTS

    // Exposure: Brighter during day, darker at night

    skyColor *= mix(0.8, 1.8, dayNightFactor);
    float horizonMask = saturate(viewDir.y * 0.5 + 0.5); // fade to black below horizon
    gl_FragColor = float4(toGamma(skyColor) * horizonMask, 1.0);
}