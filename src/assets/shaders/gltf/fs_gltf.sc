$input v_position, v_normal, v_texcoord0

#include "common.sh"

uniform vec4 u_meshColor;      // base_color fallback
uniform vec4 u_AlbedoMap;   // x=1 if texture bound, 0 otherwise

SAMPLER2D(s_albedo, 0);

void main()
{
    vec3 normal = normalize(v_normal);

    // Sample albedo texture if available, otherwise use flat color
    vec3 base_color;
    if (u_AlbedoMap.x > 0.5) {
        base_color = texture2D(s_albedo, v_texcoord0).rgb;
    } else {
        base_color = u_meshColor.rgb;
    }

    // Directional light
    vec3 light_dir = normalize(vec3(-0.5, -1.0, -0.5));
    float ndotl = max(dot(normal, -light_dir), 0.0);

    vec3 ambient  = base_color * 0.3;
    vec3 diffuse  = base_color * 0.7 * ndotl;

    gl_FragColor = vec4(ambient + diffuse, u_meshColor.a);
}