#version 450 core

// Projection matrix for the orthographic UI projection
cbuffer ProjectionMatrix {
    float4x4 u_projection;
};

struct VS_INPUT {
    float2 a_position : POSITION;
    float2 a_texcoord0 : TEXCOORD0;
    float4 a_color0 : COLOR0;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texcoord0 : TEXCOORD0;
    float4 color0 : COLOR0;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.position = mul(u_projection, float4(input.a_position, 0.0, 1.0));
    output.texcoord0 = input.a_texcoord0;
    output.color0 = input.a_color0;
    return output;
}