$input a_position
$output v_worldPos

#include <bgfx_shader.sh>

void main()
{
    // Pass world position to fragment shader
    v_worldPos = a_position;
    
    // Transform to clip space
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}