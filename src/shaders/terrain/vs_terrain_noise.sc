$input a_position, a_texcoord0
$output v_texcoord0, v_position

#include "../common/common.sh"
#include "../includes/noise_shader_terrain.sc" 

void main()
{
    v_texcoord0 = a_texcoord0;

    vec3 pos = a_position;
    
    // Use noise instead of texture for terrain height
    float height = noise(a_texcoord0 * 10.0) * 50.0;  // Scale and amplify noise
    pos.y = height;

    v_position = pos;

    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
}
