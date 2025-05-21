$input a_position, a_texcoord0

#include "../common/common.sh"

SAMPLER2D(s_heightTexture, 0);

void main()
{
    vec3 pos = a_position;

    // Terrain height displacement from heightmap
    float height = (texture2DLod(s_heightTexture, a_texcoord0, 0.0).r - 0.5) * 100.0;
    pos.y = height;

    vec4 localPos = vec4(pos, 1.0);

    // Compute clip-space position
    gl_Position = mul(u_modelViewProj, localPos);

    // No varying outputs — we write depth only
}
