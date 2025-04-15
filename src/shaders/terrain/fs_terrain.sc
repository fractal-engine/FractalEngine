$input v_position, v_texcoord0

#include "../common/common.sh"

void main()
{
    // Normalize height assuming height range from 0 to 30.0 (same as VS)
    float height = clamp(v_position.y / 30.0, 0.0, 1.0);

    // Color gradient: low = blue, mid = green, high = white
    vec3 color = mix(vec3(0.0, 0.4, 1.0), vec3(0.2, 0.8, 0.1), height); // blue to green
    color = mix(color, float3(1.0, 1.0, 1.0), smoothstep(0.7, 1.0, height));      // blend to white at top

    gl_FragColor = vec4(color, 1.0);
}
