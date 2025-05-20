$input v_shadowUv, v_depth

#include "../common/common.sh"
#include <bgfx_shader.sh>

void main() {
    // Convert to [0,1] range
    float depth = v_depth * 0.5 + 0.5;

    // Pack depth into RGBA for shadow map
    gl_FragColor = packFloatToRgba(depth);
}

