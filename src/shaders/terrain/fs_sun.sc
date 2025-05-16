$input v_position
$output o_color0

#include "../common/common.sh"

void main()
{
    float height = clamp(v_position.y / 30.0, 0.0, 1.0);
    float3 color = mix(float3(0.0, 0.4, 1.0), float3(0.2, 0.8, 0.1), height);
    color = mix(color, float3(1.0, 1.0, 1.0), smoothstep(0.7, 1.0, height));

    gl_FragColor = float4(color, 1.0); 
}
