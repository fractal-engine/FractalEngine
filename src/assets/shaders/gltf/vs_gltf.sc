$input a_position, a_normal, a_tangent, a_texcoord0
$output v_uv, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent, v_shadowCoord

#include "common.sh"
#include <bgfx_shader.sh>

// --- Uniforms ---

uniform mat4 u_lightMatrix[1];

void main()
{
    // Pass through UV coordinates
    v_uv = a_texcoord0;

    // --- Transform TBN to World Space ---
    // Normals and tangents are directions, so we use the 3x3 model matrix
    // to transform them, avoiding translation.
    mat3 modelRot = mat3(u_model[0][0].xyz, u_model[0][1].xyz, u_model[0][2].xyz);
    
    v_worldNormal   = normalize(mul(modelRot, a_normal));
    v_worldTangent  = normalize(mul(modelRot, a_tangent.xyz));
    
    // Calculate the bitangent using the cross product.

    vec3 bitangent  = cross(a_normal, a_tangent.xyz) * a_tangent.w;
    v_worldBitangent = normalize(mul(modelRot, bitangent));

    // --- Transform Position to World Space ---
    vec4 worldPos4 = mul(u_model[0], vec4(a_position, 1.0));
    v_worldPos = worldPos4.xyz;

    // --- Shadow Projection ---
    // Project the world position into the light's view space for shadow mapping.
    v_shadowCoord = mul(u_lightMatrix[0], worldPos4);

    // --- Final Screen Position ---
    gl_Position = mul(u_viewProj, worldPos4);
}