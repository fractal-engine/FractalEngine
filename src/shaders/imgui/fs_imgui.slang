// fs_imgui.slang - ImGui fragment shader
#version 450 core

Texture2D s_texture : register(t0);
SamplerState s_sampler : register(s1);

struct FS_INPUT
{
    float4 position : SV_POSITION;
    float2 texcoord0 : TEXCOORD0;
    float4 color0 : COLOR0;
};

float4 main(FS_INPUT input) : SV_TARGET
{
    float4 texColor = s_texture.Sample(s_sampler, input.texcoord0);
    return texColor * input.color0;
}