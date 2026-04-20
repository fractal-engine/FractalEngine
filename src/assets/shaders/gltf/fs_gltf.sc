$input v_position
#include "common.sh"

uniform vec4 u_meshColor;

void main()
{
    // Extract the 3D position
    vec3 pos = v_position.xyz;

    // Auto-calculate flat normals based on screen-space derivatives
    vec3 dpdx = dFdx(pos);
    vec3 dpdy = dFdy(pos);
    vec3 normal = normalize(cross(dpdx, dpdy));
    
    // Simulate a simple directional light (like Blender's default solid view)
    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.5));
    float ndotl = max(dot(normal, -lightDir), 0.0);
    
    // Ambient gray (0.3) + Diffuse gray (0.5)
    // vec3 finalColor = vec3_splat(0.3) + (vec3_splat(0.5) * ndotl); use this when removing baseColor
    vec3 baseColor = u_meshColor.rgb; // REMOVE THIS
    vec3 finalColor = (baseColor * 0.3) + (baseColor * 0.5 * ndotl);
    
    gl_FragColor = vec4(finalColor, 1.0);
}
