$input a_position
$output v_position

#include "common.sh"

void main()
{
    v_position = vec4(a_position, 1.0);
    gl_Position = mul(u_modelViewProj, v_position);
}
