$input a_position
$output v_viewDir

#include "../common/common.sh"

// Required uniforms
uniform mat4 u_projInv;
uniform mat4 u_viewInv;

void main()
{
    // Convert NDC screen-space quad to clip space
    float4 clipPos = float4(a_position.xy, 1.0, 1.0);

    // Inverse projection to view space
    float4 viewPos = mul(u_projInv, clipPos);
    viewPos /= viewPos.w;

    // Inverse view matrix to get world-space direction
    float4 worldPos = mul(u_viewInv, viewPos);

    // Output normalized direction
    v_viewDir = normalize(worldPos.xyz);

    // Fullscreen quad position
    gl_Position = float4(a_position.xy, 0.0, 1.0);
}
