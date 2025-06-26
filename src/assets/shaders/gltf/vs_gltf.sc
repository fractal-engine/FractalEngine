$input a_position, a_texcoord0
$output v_position, v_uv

#include "common.sh"

void main()
{
    v_position = vec4(a_position, 1.0);
    v_uv = a_texcoord0;

    gl_Position = mul(u_modelViewProj, v_position);
}
