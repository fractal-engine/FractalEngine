$input v_position, v_normal, v_texcoord0

#include "common.sh"

uniform vec4 u_meshColor;   // base_color fallback
uniform vec4 u_AlbedoMap;   // x=1 if texture bound, 0 otherwise

SAMPLER2D(s_albedo, 0);

void main()
{
    // Normalize the world-space normal
    vec3 normal = normalize(v_normal);

    // Transform normal to VIEW SPACE. 
    // This is the core trick for MatCaps and Blender's Solid view. 
    // The lighting will now "stick" to the camera.
    vec3 viewNormal = normalize(mul(u_view, vec4(normal, 0.0)).xyz);

    // Resolve base color
    vec3 base_color;
    if (u_AlbedoMap.x > 0.5) {
        base_color = texture2D(s_albedo, v_texcoord0).rgb;
    } else {
        base_color = u_meshColor.rgb;
    }

    // Procedural Blender MatCap Lighting
    // Light 1: Key Light (Top-Left-Front) - Bright & Warm
    vec3 keyDir = normalize(vec3(-0.6, 0.8, 0.4));
    float keyIntensity = max(dot(viewNormal, keyDir), 0.0);

    // Light 2: Fill Light (Bottom-Right-Front) - Soft & slightly cool
    vec3 fillDir = normalize(vec3(0.5, -0.4, 0.4));
    float fillIntensity = max(dot(viewNormal, fillDir), 0.0);

    // Light 3: Back/Rim Light (Top-Back) - Gives edges subtle definition
    vec3 rimDir = normalize(vec3(0.0, 0.6, -0.8));
    float rimIntensity = max(dot(viewNormal, rimDir), 0.0);

    // Combine the lights
    vec3 ambient = vec3_splat(0.15); // Base minimum light so nothing is pitch black
    
    vec3 diffuse = (keyIntensity * vec3(0.85, 0.85, 0.85)) + 
                   (fillIntensity * vec3(0.25, 0.28, 0.35)) + 
                   (rimIntensity  * vec3(0.15, 0.15, 0.15));

    vec3 final_color = base_color * (ambient + diffuse);

    gl_FragColor = vec4(final_color, u_meshColor.a);
}