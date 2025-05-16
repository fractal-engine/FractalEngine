$input a_position
$output v_viewDir

#include "../common/common.sh"

// Required uniforms
uniform mat4 u_projInv;
uniform mat4 u_viewInv;

void main()
{
    float2 ndc = a_position.xy;
// a_position.xy is expected to be in NDC space [-1, 1].
// This is the position of the vertex on the screen quad.

float4 clipPos = float4(ndc, 1.0, 1.0);
// Construct clip-space position (z = 1.0 for far plane, w = 1.0 for homogeneous divide).

float4 viewPos = mul(u_projInv, clipPos);
// Transform from clip space to view space using inverse projection matrix.

viewPos /= viewPos.w;
// Perspective divide to get actual view-space position (from homogeneous coordinates).

float4 worldPos = mul(u_viewInv, viewPos);
// Transform from view space to world space using inverse view matrix.

v_viewDir = normalize(worldPos.xyz);
// Output the normalized world-space direction for use in the fragment shader.

gl_Position = float4(ndc, 0.0, 1.0);
// Output position of the fullscreen quad in clip space.

}
