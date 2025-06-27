$input v_position, v_uv

#include "common.sh"

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_orm, 1);
SAMPLER2D(s_normal, 2);

void main()
{
    vec4 albedo = texture2D(s_diffuse, v_uv);
    vec3 orm    = texture2D(s_orm,     v_uv).rgb;
    vec3 normal = texture2D(s_normal,  v_uv).rgb;

    // For debug: visualize ORM as color
    // gl_FragColor = vec4(orm, 1.0);

    // For now just show base color
    gl_FragColor = albedo;
}
