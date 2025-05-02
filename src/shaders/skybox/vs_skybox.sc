$input a_position
$output v_viewDir

#include "../common/common.sh"

void main()
{
    // Project cube vertex
    gl_Position = vec4(a_position, 1.0);

    // Use position as a directional vector
    v_viewDir = a_position;
}
