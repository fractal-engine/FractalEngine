$input a_position, a_texcoord0
$output v_out_uv, v_out_worldPos, v_out_shadowCoord, v_out_viewVec, v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom

#include "../common/common.sh"
#include <bgfx_shader.sh>

void main() {

    // Output UV
    v_out_uv = a_texcoord0;

    // Transform position to world space
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    v_out_worldPos = worldPos.xyz;

    // Flat plane: upward-facing normal
    vec3 N = vec3(0.0, 1.0, 0.0);
    vec3 T = vec3(1.0, 0.0, 0.0);
    vec3 B = vec3(0.0, 0.0, 1.0);

    mat3 modelRot = mat3(u_model[0][0].xyz, u_model[0][1].xyz, u_model[0][2].xyz);

    v_out_worldNormalGeom  = normalize(mul(modelRot, N));
    v_out_worldTangent     = normalize(mul(modelRot, T));
    v_out_worldBitangent   = normalize(mul(modelRot, B));
    v_out_viewVec          = -mul(u_view, worldPos).xyz;

    gl_Position = mul(u_viewProj, worldPos);
}