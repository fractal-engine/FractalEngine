$input a_position, a_texcoord0
$output v_shadowUv, v_depth

#include "../common/common.sh"

SAMPLER2D(s_heightTexture, 0);
uniform vec4 u_terrainScale;

void main() {

    vec3 pos = a_position;


    // Terrain height displacement from heightmap
    float height = (texture2DLod(s_heightTexture, a_texcoord0, 0.0).r - 0.5) * 100.0;
    pos.y = height;

    vec4 localPos = vec4(pos, 1.0);

    // Compute clip-space position for shadow render pass
    vec4 clipPos = mul(u_modelViewProj, localPos);
    gl_Position = clipPos;

    // Pass screen-space UV for sampling in fragment shader
    v_shadowUv = clipPos.xy / clipPos.w * 0.5 + 0.5;

    // Linear depth for packing
    v_depth = clipPos.z / clipPos.w;
}
