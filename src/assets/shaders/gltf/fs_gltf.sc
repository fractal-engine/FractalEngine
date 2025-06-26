$input v_position, v_uv

#include "common.sh"

SAMPLER2D(s_diffuse, 0); // Must match bgfx uniform

void main()
{
    vec4 color = texture2D(s_diffuse, v_uv);
    gl_FragColor = color;
}
