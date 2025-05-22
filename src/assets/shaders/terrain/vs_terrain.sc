$input a_position, a_texcoord0
$output v_texcoord0, v_position

#include "../common/common.sh"

#ifndef S_HEIGHT_TEXTURE
#define S_HEIGHT_TEXTURE
SAMPLER2D(s_heightTexture, 0);

#endif

void main() {
    v_texcoord0 = a_texcoord0;

    vec3 pos = a_position;
    float height = texture2DLod(s_heightTexture, a_texcoord0, 0.0).r * 50.0;
    pos.y = height;

    v_position = vec4(pos, 1.0); 

    gl_Position = mul(u_modelViewProj, v_position);

}
