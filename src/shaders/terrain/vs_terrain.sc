$input a_position, a_texcoord0
$output v_texcoord0, v_position, v_worldPos, v_view

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

    vec4 localPosition = vec4(pos, 1.0);
    v_position = localPosition;

    // World space position
    vec4 worldPos = mul(u_model[0], localPosition);
    v_worldPos = worldPos.xyz;

    // Compute view-space position
    vec4 viewPos = mul(u_view, worldPos);

    // v_view = vector from camera to fragment, in view space
    v_view = -viewPos.xyz;


    // Final projected position
    gl_Position = mul(u_modelViewProj, localPosition);
}
