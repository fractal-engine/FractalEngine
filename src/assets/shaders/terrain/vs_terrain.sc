$input a_position, a_texcoord0
$output v_out_uv, v_out_worldPos, v_out_shadowCoord, v_out_viewVec, v_out_worldTangent, v_out_worldBitangent, v_out_worldNormalGeom

#include "../common/common.sh"
#include <bgfx_shader.sh>

// --- Uniforms ---
SAMPLER2D(s_heightTexture, 3);
uniform vec4 u_terrainParams;         // x: height scale, z: world step x, w: world step z
uniform vec4 u_heightmapTexelSize;    // texel size in UV space
uniform mat4 u_lightMatrix[1];

#define u_terrainHeightScale u_terrainParams.x
#define u_worldStepX         u_terrainParams.z
#define u_worldStepZ         u_terrainParams.w

// --- Heightmap Sampling ---
float getScaledHeight(vec2 uv) {
    return (texture2DLod(s_heightTexture, uv, 0.0).r * 2.0 - 1.0) * u_terrainHeightScale;
}

void main() {
    // UV passthrough
    v_out_uv = a_texcoord0;

    // Center position
    float h_center = getScaledHeight(a_texcoord0);
    vec3 p_center = vec3(a_position.x, h_center, a_position.z);

    // Neighbor in +U (x) direction
    vec2 uv_u = a_texcoord0 + vec2(u_heightmapTexelSize.x, 0.0);
    float h_u = getScaledHeight(uv_u);
    vec3 p_u = vec3(a_position.x + u_worldStepX, h_u, a_position.z);

    // Neighbor in +V (z) direction
    vec2 uv_v = a_texcoord0 + vec2(0.0, u_heightmapTexelSize.y);
    float h_v = getScaledHeight(uv_v);
    vec3 p_v = vec3(a_position.x, h_v, a_position.z + u_worldStepZ);

    // Local TBN from neighbors
    vec3 tangent   = normalize(p_u - p_center);
    vec3 bitangent = normalize(p_v - p_center);
    vec3 normal    = normalize(cross(tangent, bitangent));
    bitangent      = normalize(cross(normal, tangent));

    // Transform TBN to world space
    vec4 col0 = u_model[0][0];
    vec4 col1 = u_model[0][1];
    vec4 col2 = u_model[0][2];

    mat3 modelRot = mat3(col0.xyz, col1.xyz, col2.xyz);

    // Use mul() instead of operator *
    v_out_worldTangent    = normalize(mul(modelRot, tangent));
    v_out_worldBitangent  = normalize(mul(modelRot, bitangent));
    v_out_worldNormalGeom = normalize(mul(modelRot, normal));


    // World position
    vec4 worldPos4 = mul(u_model[0], vec4(p_center, 1.0));
    v_out_worldPos = worldPos4.xyz;

    // View vector (camera space, pointing to camera)
    v_out_viewVec = -mul(u_view, worldPos4).xyz;

    // Shadow projection
    v_out_shadowCoord = mul(u_lightMatrix[0], worldPos4);

    gl_Position = mul(u_viewProj, worldPos4);
}
