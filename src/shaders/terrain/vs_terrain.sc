$input a_position, a_texcoord0
$output v_texcoord0, v_position

#include "../common/common.sh"

SAMPLER2D(s_heightTexture, 0);

void main() {
    v_texcoord0 = a_texcoord0;

    // Base position
    vec3 pos = a_position;

    // Sample height from texture and scale it
    float height = texture2DLod(s_heightTexture, a_texcoord0, 0.0).r * 50.0;
    pos.y = height;

    v_position = pos; // Pass to fragment for slope calc

    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
}
