$input a_position, a_texcoord0
$output v_texcoord0, v_position, v_worldPos, v_view, v_shadowCoord

#include "../common/common.sh"

uniform mat4 u_lightMatrix[1];
uniform vec4 u_terrainScale;

#ifndef S_HEIGHT_TEXTURE
#define S_HEIGHT_TEXTURE
SAMPLER2D(s_heightTexture, 0);
#endif

void main() {
    v_texcoord0 = a_texcoord0;

    // Apply terrain displacement using heightmap
    vec3 pos = a_position;

    // Center height: maps [0, 1] to [-50, +50]
    float height = (texture2DLod(s_heightTexture, a_texcoord0, 0.0).r - 0.5) * 100.0;
    pos.y = height;

    vec4 localPosition = vec4(pos, 1.0);
    v_position = localPosition; // optional use - clip/view space

    // World space position
    vec4 worldPos = mul(u_model[0], localPosition);
    v_worldPos = worldPos.xyz;

    // View vector
    vec4 viewPos = mul(u_view, worldPos);
    v_view = -viewPos.xyz;

    // Shadow projection
    v_shadowCoord = mul(u_lightMatrix[0], worldPos);

    // Final projected position
    gl_Position = mul(u_modelViewProj, localPosition);
}
