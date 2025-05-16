$input a_position
$output v_viewDir, v_screenPos, v_skyColor

#include "../common/common.sh"

void main()
{
    vec4 pos = mul(u_modelViewProj, vec4(a_position, 1.0));
    // Basic projection setup
    gl_Position = pos;

    // This will point from camera center outwards
    v_viewDir = a_position;

    // convert clip-space to [0,1] UV:
    v_screenPos = pos.xy / pos.w * 0.5 + 0.5;

    // Initial base sky color (e.g. horizon blue), tweak as needed
    v_skyColor = vec3(0.2, 0.4, 0.8);
}

