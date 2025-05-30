$input v_viewDir

#include "../common/common.sh" 

// Uniform parameters:
// u_parameters: x = sun angular radius (radians, ~0.0046 for Earth sun), 
//               y = procedural bloom intensity multiplier,
//               z = exposure,
//               w = time for cloud animation

uniform vec4 u_parameters;

// Sun direction, normalized (xyz)

uniform vec4 u_sunDirection;

// Sun color & intensity before atmospheric scattering (rgb)

uniform vec4 u_sunLuminance;

// Scattering parameters:
// x = g (Mie phase anisotropy, e.g. 0.76)

uniform vec4 u_scatterParams;

// Optical depths:
// u_betaR = Rayleigh total optical depth (xyz)
// u_betaM = Mie total optical depth (xyz)

uniform vec4 u_betaR;
uniform vec4 u_betaM;

// Artistic sky blending:
// x = blending factor for artistic sky colors (0 = physical only, 1 = full artistic add)

uniform vec4 u_artisticInfluence; 
// y, z, w unused currently

#define PI 3.14159265359
#define SUN_VIEW_ZENITH_EPSILON 0.005 // Horizon stability epsilon for zenith angle calculations

// ------ Scattering phase functions ------

// Rayleigh phase function for scattering angle cosTheta

float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

// Henyey-Greenstein phase function (Mie scattering)
// g = anisotropy parameter

float hgPhase(float cosTheta, float g) {
    float g2 = g * g;
    float num = 1.0 - g2;
    // Avoid issues with pow(0, 1.5) by clamping denominator base
    float den_base = 1.0 + g2 - 2.0 * g * cosTheta;
    float den = pow(max(den_base, 1e-4), 1.5);
    return (1.0 / (4.0 * PI)) * (num / den);
}

// ------ Noise and fractional Brownian motion (fbm) for clouds ------

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

    // Normalize view and sun directions

    float3 viewDir = normalize(v_viewDir);
    float3 sunDir = normalize(u_sunDirection.xyz);
    float3 initialSunColorAndIntensity = u_sunLuminance.rgb;

    // --- Time-of-day factors ---

    float sunElevationY = sunDir.y;
    float dayNightFactor = smoothstep(-0.15, 0.2, sunElevationY);
    float sunriseSunsetFactor = smoothstep(0.0, 0.2, sunElevationY) * (1.0 - smoothstep(0.1, 0.3, sunElevationY));
    sunriseSunsetFactor = saturate(sunriseSunsetFactor * 3.0);

    // ------ Physical Sky Calculation ------

    // Angles between view and sun directions
    float cosTheta = dot(viewDir, sunDir);
    float cosSunZenith = max(sunElevationY, SUN_VIEW_ZENITH_EPSILON); // sun zenith stability
    float cosViewZenith = max(viewDir.y, SUN_VIEW_ZENITH_EPSILON);    // view zenith stability

    // Optical depths for scattering

    float3 totalRayleighOpticalDepth = u_betaR.rgb;
    float3 totalMieOpticalDepth = u_betaM.rgb;
    float mieAnisotropy = u_scatterParams.x;

    float3 totalExtinctionOpticalDepth = totalRayleighOpticalDepth + totalMieOpticalDepth;
    // Avoid division by zero or negative extinction by clamping minimum
    totalExtinctionOpticalDepth = max(totalExtinctionOpticalDepth, float3(1e-8, 1e-8, 1e-8));

    // Calculate air mass (optical path length) for sun and view

    float sunPathAirMass = 1.0 / cosSunZenith;
    float3 opticalDepthSunToScatteringPoint = totalExtinctionOpticalDepth * sunPathAirMass;
    float3 sunLightTransmittance = exp(-opticalDepthSunToScatteringPoint);

    // Phase functions for Rayleigh and Mie scattering

    float rayleighPhaseVal = rayleighPhase(cosTheta);
    float miePhaseVal = hgPhase(cosTheta, mieAnisotropy);

    // Scattering terms weighted by optical depth and phase function

    float3 rayleighScatteringTerm = totalRayleighOpticalDepth * rayleighPhaseVal;
    float3 mieScatteringTerm = totalMieOpticalDepth * miePhaseVal;

    // Air mass and optical depth along view path

    float viewPathAirMass = 1.0 / cosViewZenith;
    float3 opticalDepthViewPath = totalExtinctionOpticalDepth * viewPathAirMass;

    // Factor for scattering contribution from view path

    float3 viewPathInScatterFactor = (float3(1.0, 1.0, 1.0) - exp(-opticalDepthViewPath)) / totalExtinctionOpticalDepth;

    // Final physical sky color combining scattering effects

    float3 physicalSkyColor = initialSunColorAndIntensity * sunLightTransmittance *
                              (rayleighScatteringTerm + mieScatteringTerm) * viewPathInScatterFactor;

    // ------ Artistic Sky Influence (adds “fake” colors) ------

    float3 artisticSkyInfluenceColor = float3(0.0, 0.0, 0.0);
    if (u_artisticInfluence.x > 0.0) {
        // Define base colors for night, day, sunrise for zenith and horizon
        float3 nightZenithCol   = float3(0.01, 0.02, 0.05);
        float3 nightHorizonCol  = float3(0.03, 0.04, 0.12);
        float3 dayZenithCol     = float3(0.20, 0.45, 0.85);
        float3 dayHorizonCol    = float3(0.50, 0.70, 0.95);
        float3 sunriseZenithCol = float3(0.4, 0.25, 0.1);
        float3 sunriseHorizonCol= float3(0.9, 0.4, 0.15);

        // Interpolate colors by time of day and sunrise/sunset factors
        float3 currentZenithColor = mix(nightZenithCol, dayZenithCol, dayNightFactor);
        float3 currentHorizonColor = mix(nightHorizonCol, dayHorizonCol, dayNightFactor);
        currentZenithColor = mix(currentZenithColor, sunriseZenithCol, sunriseSunsetFactor);
        currentHorizonColor = mix(currentHorizonColor, sunriseHorizonCol, sunriseSunsetFactor);

        // Elevation factor from view direction (0 at horizon, 1 at zenith)
        float viewElevationFactor = pow(saturate(viewDir.y * 0.5 + 0.5), 0.6);
        artisticSkyInfluenceColor = mix(currentHorizonColor, currentZenithColor, viewElevationFactor);
    }

    // Combine physical and artistic sky colors with blending factor
    float3 baseSkyColor = physicalSkyColor + artisticSkyInfluenceColor * u_artisticInfluence.x;


    // ------ Spherical coordinates for cloud and star sampling ------

    float longitude = atan2(viewDir.x, viewDir.z);
    float latitude  = asin(viewDir.y);
    float2 uv = float2(longitude / (2.0 * PI) + 0.5, latitude / PI + 0.5);

    // --- Clouds ---

    float3 skyWithClouds = baseSkyColor;
    float2 cloudAnimUV = uv + float2(u_parameters.w * 0.002, 0.0);
    float cloudNoiseVal = fbm(cloudAnimUV * 10.0);
    float cloudCoverageMask = smoothstep(0.5, 0.7, cloudNoiseVal);
    float cloudOverallVisibility = dayNightFactor * (1.0 - sunriseSunsetFactor * 0.5);

    if (cloudOverallVisibility > 0.0 && cloudCoverageMask > 0.0) {
        float3 cloudBaseColor = float3(0.95, 0.95, 1.0);
        // Clouds lit by sun transmittance, simplified lighting
        float3 cloudSunInfluence = mix(float3(1.0,1.0,1.0), sunLightTransmittance, sunriseSunsetFactor * 0.8 + 0.2);
        float3 litCloudColor = cloudBaseColor * initialSunColorAndIntensity * cloudSunInfluence * 0.5; 
        
        // Blend clouds over sky base color
        skyWithClouds = mix(skyWithClouds, litCloudColor, cloudCoverageMask * cloudOverallVisibility * 0.4);
    }
    float3 finalSkyColorResult = skyWithClouds;

    // --- Stars ---

    float starOverallVisibility = (1.0 - dayNightFactor) * (1.0 - sunriseSunsetFactor);
    if (starOverallVisibility > 0.0) {
        float starNoise = pow(noise(uv * 512.0), 100.0);
        starNoise += pow(noise(uv * 128.0 + vec2(50.0, 50.0)), 150.0) * 0.3;
        starNoise += pow(noise(uv * 64.0 + vec2(100.0, 100.0)), 120.0) * 0.15;

        float3 starColor = vec3(1.0, 1.0, 1.0);
        finalSkyColorResult += starColor * starNoise * starOverallVisibility * 0.8;
    }

    // --- Gamma correction and exposure ---
    finalSkyColorResult = toGamma(finalSkyColorResult * u_parameters.z);

    // Output the final color

    gl_FragColor = vec4(finalSkyColorResult, 1.0);
}
