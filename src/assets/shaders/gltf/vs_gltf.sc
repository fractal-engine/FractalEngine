$input a_position
$output v_position

#include "common.sh"

void main()
{
    // Pass world-space position to fragment shader 
    v_position = mul(u_model[0], vec4(a_position, 1.0));
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}