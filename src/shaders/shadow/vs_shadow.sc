$input a_position
#include "../common/common.sh"

void main() {
    vec3 pos = a_position;
    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
}
