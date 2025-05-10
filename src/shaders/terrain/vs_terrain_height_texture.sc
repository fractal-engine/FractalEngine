$input a_position, a_texcoord0
$output v_position, v_texcoord0

#include "../common/common.sh"


SAMPLER2D(s_heightTexture, 0);

void main()
{
    v_texcoord0 = a_texcoord0;

    // Sample height value from texture
    float height = texture2DLod(s_heightTexture, a_texcoord0, 0.0).r;

    // Apply scaling to exaggerate height (adjust as needed)
    vec3 pos = a_position;
    height = height * 12.0; // Exaggerate height (increase the canyon effect)
    float canyonEffect = exp(-10.0 * (a_texcoord0.x - 0.5) * (a_texcoord0.x - 0.5)); // Strong canyon in the center
    height *= canyonEffect;  // Apply canyon depth effect
    pos.y = height; // Set the new height to the Y position

    v_position = pos;

    // Project to clip space
    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
}
