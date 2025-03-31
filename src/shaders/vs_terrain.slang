// vs_terrain.hlsl (or .slang compiled to vs_terrain.bin)
cbuffer DummyUniform { float4 dummy; };

struct VS_INPUT {
    float3 a_position : POSITION;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    // Pass through the vertex position (assume an identity transform)
    output.position = float4(input.a_position, 1.0);
    return output;
}