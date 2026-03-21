$input v_position
#include "common.sh"

// --- DEBUG VISUALIZATION SHADER ---
/* Outputs positions as RGB color */
void main()
{
    gl_FragColor = vec4(abs(v_position.xyz) * 2.5, 1.0);
}
