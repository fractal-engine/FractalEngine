$input v_color0

#include <bgfx_shader.sh>

void main()
{
    // Output vertex color with alpha blending
    gl_FragColor = v_color0;
}