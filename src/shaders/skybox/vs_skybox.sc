$input a_position
$output v_viewDir, v_screenPos, v_skyColor

#include "../common/common.sh"

void main()
{
    // Transform cube vertex from object to clip space
    vec4 pos = mul(u_modelViewProj, vec4(a_position, 1.0));
    gl_Position = pos;

    // Calculate direction vector from camera to vertex in world space
    v_viewDir = a_position;

    // Map NDC coordinates [-1..1] to screen UVs [0..1]
    v_screenPos = pos.xy / pos.w * 0.5 + 0.5;

    // Give a base color, this can be overwritten by the fragment shader
    v_skyColor = vec3(0.2, 0.4, 0.8);
}
