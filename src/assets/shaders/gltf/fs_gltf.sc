$input v_position
#include "common.sh"

void main()
{
    gl_FragColor = vec4(abs(v_position.xyz), 1.0);
}
