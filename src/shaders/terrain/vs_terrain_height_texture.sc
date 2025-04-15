$input a_position, a_texcoord0
$output v_position, v_texcoord0

#include "../common/common.sh"

SAMPLER2D(s_heightTexture, 0); // Uniform created in C++

void main()
{
    v_texcoord0 = a_texcoord0;

    // Sample height value from texture
    float height = texture2DLod(s_heightTexture, a_texcoord0, 0.0).r;

    // Apply scaling to exaggerate height (adjust as needed)
    vec3 pos = a_position;
    pos.y = height * 30.0; // Scale height between 0 and 30 units

    v_position = pos;

    // Project to clip space
    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
}
