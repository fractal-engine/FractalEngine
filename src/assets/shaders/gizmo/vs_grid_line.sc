$input a_position, a_color0
$output v_color0

#include <bgfx_shader.sh>

void main()
{
    // Transform position to clip space
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    
    // Pass color through
    v_color0 = a_color0;
}