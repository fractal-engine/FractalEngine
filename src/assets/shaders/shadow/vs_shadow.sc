$input a_position, a_texcoord0
$output v_out_uv

#include "../common/common.sh"
#include <bgfx_shader.sh>

SAMPLER2D(s_heightTexture, 3);
uniform vec4 u_terrainParams;

#define u_terrainHeightScale u_terrainParams.x

float getHeight(vec2 uv) {
    return (bgfxTexture2DLod(s_heightTexture, uv, 0.0).r * 2.0 - 1.0) * u_terrainHeightScale;
}

void main() {
    float height = getHeight(a_texcoord0);
    vec3 displaced = vec3(a_position.x, height, a_position.z);
    gl_Position = mul(u_viewProj, mul(u_model[0], vec4(displaced, 1.0)));

    v_out_uv = a_texcoord0;
}
