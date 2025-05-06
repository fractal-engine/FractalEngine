$input a_position, a_texcoord0
$output v_texcoord0, v_position

#include "../common/common.sh"
#include "../includes/noise_shader_terrain.sc" 

SAMPLER2D(s_heightTexture, 0);

void main()
{
    v_texcoord0 = a_texcoord0;

    vec3 pos = a_position;
    
    // Use noise instead of texture for terrain height
    float height = texture2DLod(s_heightTexture, a_texcoord0, 0.0).r * 50.0;

    // Central canyon: deep trench, flat sides
    float dist = abs(a_texcoord0.x - 0.5);
    float canyonEffect = 1.0 - smoothstep(0.1, 0.4, dist); // deep in center, flat sides

    height *= canyonEffect;

    // Create horizontal "steps" in terrain
    height = floor(height * 5.0) / 5.0;

    pos.y = height;


    v_position = pos;

    // Project to clip space
    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
}
