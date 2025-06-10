$input v_viewDir
#include "../common/common.sh"

// --- Uniforms ---
uniform vec4 u_parameters;         // xyz = sun angular radius, bloom, exposure; w = time
uniform vec4 u_sunDirection;       // xyz = sun direction vector
uniform vec4 u_sunLuminance;       // rgb = sun color (already includes intensity for this shader)
uniform vec4 u_scatterParams;      // x = mie anisotropy (g), yzw = unused
uniform vec4 u_betaR;              // rgb = total Rayleigh scattering coefficients
uniform vec4 u_betaM;              // rgb = total Mie scattering coefficients
uniform vec4 u_artisticInfluence;  // x = artistic sky blend factor, yzw = unused (basically the old color function)

// --- Constants ---
#define PI 3.14159265359
#define SUN_VIEW_ZENITH_EPSILON 0.005 // Small epsilon to prevent division by zero for zenith angles

// --- Scattering Phase Functions ---
// Calculates the Rayleigh phase function
float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

// Calculates the Henyey-Greenstein phase function for Mie scattering

float hgPhase(float cosTheta, float g) {
    float g2 = g * g;                                  // Mie asymmetry parameter squared
    float num = 1.0 - g2;                              // Numerator of the HG phase function
    float den_base = 1.0 + g2 - 2.0 * g * cosTheta;    // Base of the denominator term
    float den = pow(max(den_base, 1e-4), 1.5);         // Denominator, clamped to prevent issues
    return (1.0 / (4.0 * PI)) * (num / den);           // Complete HG phase function
}

// --- Noise Functions (for Clouds/Stars) ---

// Simple hash function for procedural noise
float hash(vec2 p) {
    p = frac(p * 0.3183099 + vec2(0.71, 0.113)); // Apply transformations and take fractional part
    return frac(43758.5453 * p.x * p.y);          // Combine components and take fractional part
}

// 2D procedural noise function using hash
float noise(vec2 p) {
    vec2 i = floor(p);                             // Integer part of p
    vec2 f = frac(p);                              // Fractional part of p
    float a = hash(i);                             // Noise value at grid point (0,0)
    float b = hash(i + vec2(1.0, 0.0));            // Noise value at grid point (1,0)
    float c = hash(i + vec2(0.0, 1.0));            // Noise value at grid point (0,1)
    float d = hash(i + vec2(1.0, 1.0));            // Noise value at grid point (1,1)
    vec2 u = f * f * (3.0 - 2.0 * f);              // Smooth interpolation weights (smoothstep)
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y); // Bilinear interpolation of noise values
}

// Fractional Brownian Motion (fBm) for multi-octave noise

float fbm(vec2 p) {
    float result = 0.0;                            // Initialize result
    float amp = 0.5;                               // Initial amplitude
    for (int i = 0; i < 4; ++i) {                  // Loop for multiple octaves
        result += noise(p) * amp;                  // Add noise at current octave, scaled by amplitude
        p *= 2.0;                                  // Increase frequency for next octave
        amp *= 0.5;                                // Decrease amplitude for next octave
    }
    return result;
}

// --- Main Fragment Shader ---
void main() {
    // --- Initial Setup: View and Sun Directions ---
    float3 viewDir = normalize(v_viewDir);                     
    float3 sunDir = normalize(u_sunDirection.xyz);             
    float3 initialSunColorAndIntensity = u_sunLuminance.rgb;   

    // --- Time of Day Factors ---
    float sunElevationY = sunDir.y;                                                     
    float dayNightFactor = smoothstep(-0.15, 0.2, sunElevationY);                        
    float sunriseSunsetFactor = smoothstep(0.0, 0.2, sunElevationY) * (1.0 - smoothstep(0.1, 0.3, sunElevationY)); 
    sunriseSunsetFactor = saturate(sunriseSunsetFactor * 3.0);                           

    // --- Angle Cosines for Scattering Calculations ---
    float cosTheta = dot(viewDir, sunDir);                                 // Cosine of the angle between view and sun directions
    float cosSunZenith = max(sunElevationY, SUN_VIEW_ZENITH_EPSILON);      // Cosine of sun's zenith angle (clamped)
    float cosViewZenith = max(viewDir.y, SUN_VIEW_ZENITH_EPSILON);         // Cosine of view's zenith angle (clamped)

    // --- Atmospheric Parameters ---
    float3 totalRayleighOpticalDepth = u_betaR.rgb;                        // Total Rayleigh scattering optical depth
    float3 totalMieOpticalDepth = u_betaM.rgb;                             // Total Mie scattering optical depth
    float mieAnisotropy = u_scatterParams.x;                               // Mie scattering anisotropy 'g' parameter

    float3 totalExtinctionOpticalDepth = totalRayleighOpticalDepth + totalMieOpticalDepth; 
    totalExtinctionOpticalDepth = max(totalExtinctionOpticalDepth, float3(1e-8, 1e-8, 1e-8)); // Clamp to prevent division by zero later

    // --- Sun Light Transmittance ---
    float sunPathAirMass = 1.0 / cosSunZenith;                                        
    float3 opticalDepthSunToScatteringPoint = totalExtinctionOpticalDepth * sunPathAirMass; 
    float3 sunLightTransmittance = exp(-opticalDepthSunToScatteringPoint);              // Transmittance of sunlight to scattering point (Beer's Law)

    // --- Phase Function Values ---
    float rayleighPhaseVal = rayleighPhase(cosTheta);                      // Rayleigh phase function value for current angle
    float miePhaseVal = hgPhase(cosTheta, mieAnisotropy);                  // Mie (Henyey-Greenstein) phase function value

    // --- Scattering Terms ---
    float3 rayleighScatteringTerm = totalRayleighOpticalDepth * rayleighPhaseVal; // Combined Rayleigh scattering term
    float3 mieScatteringTerm = totalMieOpticalDepth * miePhaseVal;                // Combined Mie scattering term

    // --- View Path In-Scattering Factor ---
    float viewPathAirMass = 1.0 / cosViewZenith;                                            
    float3 opticalDepthViewPath = totalExtinctionOpticalDepth * viewPathAirMass;            
    float3 viewPathInScatterFactor = (float3(1.0, 1.0, 1.0) - exp(-opticalDepthViewPath)) / totalExtinctionOpticalDepth; 

    // --- Physical Sky Color Calculation ---
    // Combines sun light, transmittance, scattering, and view path integration
    float3 physicalSkyColor = initialSunColorAndIntensity * sunLightTransmittance * (rayleighScatteringTerm + mieScatteringTerm) * viewPathInScatterFactor;

    // --- Artistic Sky Color Influence ---
    float3 artisticSkyInfluenceColor = float3(1.0, 1.0, 1.0);              // Default artistic influence (no change)
    if (u_artisticInfluence.x > 0.0) {                                     // Check if artistic influence is active
        // Define artistic colors for different times of day and view angles
        float3 nightZenithCol = float3(0.01, 0.02, 0.05);
        float3 nightHorizonCol = float3(0.03, 0.04, 0.12);
        float3 dayZenithCol = float3(0.20, 0.45, 0.85);
        float3 dayHorizonCol = float3(0.50, 0.70, 0.95);
        float3 sunriseZenithCol = float3(0.4, 0.25, 0.1);
        float3 sunriseHorizonCol = float3(0.9, 0.4, 0.15);

        // Interpolate zenith and horizon colors based on time of day
        float3 currentZenithColor = mix(nightZenithCol, dayZenithCol, dayNightFactor);
        float3 currentHorizonColor = mix(nightHorizonCol, dayHorizonCol, dayNightFactor);
        // Further mix with sunrise/sunset colors
        currentZenithColor = mix(currentZenithColor, sunriseZenithCol, sunriseSunsetFactor);
        currentHorizonColor = mix(currentHorizonColor, sunriseHorizonCol, sunriseSunsetFactor);

        // Interpolate between horizon and zenith color based on view elevation
        float viewElevationFactor = pow(saturate(viewDir.y * 0.5 + 0.5), 0.6); // Factor based on view direction's Y component
        artisticSkyInfluenceColor = mix(currentHorizonColor, currentZenithColor, viewElevationFactor);
    }

    // --- Combine Physical and Artistic Sky ---
    float3 baseSkyColor = physicalSkyColor + artisticSkyInfluenceColor * u_artisticInfluence.x; // Add artistic influence to physical sky
    baseSkyColor = baseSkyColor * 10;                                            // Boost ambient color for artistic effect

    // --- Cloud Layer Setup (call functions)---
    float longitude = atan2(viewDir.x, viewDir.z);                         // Calculate longitude from view direction
    float latitude = asin(viewDir.y);                                      // Calculate latitude from view direction
    float2 uv = float2(longitude / (2.0 * PI) + 0.5, latitude / PI + 0.5); // Convert to spherical UV coordinates

    float3 skyWithClouds = baseSkyColor;                                   // Initialize sky color with clouds
    float2 cloudAnimUV = uv + float2(u_parameters.w * 0.002, 0.0);         
    float cloudNoiseVal = fbm(cloudAnimUV * 10.0);                         
    float cloudCoverageMask = smoothstep(0.5, 0.7, cloudNoiseVal);         
    float cloudOverallVisibility = dayNightFactor * (1.0 - sunriseSunsetFactor * 0.5); // Clouds less visible during strong sunrise/sunset

    // --- Cloud Rendering ---
    if (cloudOverallVisibility > 0.0 && cloudCoverageMask > 0.0) {         // Check if clouds should be rendered
        float3 cloudBaseColor = float3(0.95, 0.95, 1.0);                   // Base color for clouds

        // Influence of sun transmittance on cloud color, more pronounced during sunrise/sunset
        float3 cloudSunInfluence = mix(float3(1.0, 1.0, 1.0), sunLightTransmittance, sunriseSunsetFactor * 0.8 + 0.2);
        float3 litCloudColor = cloudBaseColor * initialSunColorAndIntensity * cloudSunInfluence * 0.5;                      // Calculate lit cloud color

        // Mix base sky color with lit cloud color based on coverage and visibility
        skyWithClouds = mix(skyWithClouds, litCloudColor, cloudCoverageMask * cloudOverallVisibility * 0.4);
    }

    // --- Star Layer ---

    float3 finalSkyColorResult = skyWithClouds;                             // Start with sky color including clouds
    float starOverallVisibility = (1.0 - dayNightFactor) * (1.0 - sunriseSunsetFactor); // Stars visible mainly at night, not during sunrise/sunset
    if (starOverallVisibility > 0.0) {                                     // Check if stars should be rendered
        // Generate multi-layered noise for stars
        float starNoise = pow(noise(uv * 512.0), 100.0);                   
        starNoise += pow(noise(uv * 128.0 + vec2(50.0, 50.0)), 150.0) * 0.3; 
        starNoise += pow(noise(uv * 64.0 + vec2(100.0, 100.0)), 120.0) * 0.15; 
        float3 starColor = vec3(1.0, 1.0, 1.0);                            // Base star color
        finalSkyColorResult += starColor * starNoise * starOverallVisibility * 0.8; 
    }

    // --- Tonemapping and Gamma Correction ---
    // Apply exponential tonemapping using exposure from u_parameters.z
    finalSkyColorResult = vec3(1.0, 1.0, 1.0) - exp(-finalSkyColorResult * u_parameters.z);
    finalSkyColorResult = toGamma(finalSkyColorResult);                     // Convert to sRGB color space for display

    // --- Output Final Color ---
    gl_FragColor = vec4(finalSkyColorResult, 1.0);                        
}