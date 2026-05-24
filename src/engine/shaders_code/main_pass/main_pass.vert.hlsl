struct VSInput
{
    float3 a_pos     : POSITION;
    float2 a_uv      : TEXCOORD0;
    float3 a_normal  : NORMAL;
    float3 a_tangent : TANGENT;
    uint instanceID  : SV_InstanceID;
};

struct VSOutput
{
    float4 position         : SV_Position;
    float2 v_uv             : TEXCOORD0;
    float3 v_worldPos       : TEXCOORD1;
    float3 v_worldNormal    : TEXCOORD2;
    float3 v_worldTangent   : TEXCOORD3;
    float3 v_worldBitangent : TEXCOORD4;
};

// GLSL std430 buffer → HLSL StructuredBuffer
StructuredBuffer<float4x4> ModelMatrixBlock     : register(t0, space0);
StructuredBuffer<int>      PositionIndexBuffer  : register(t1, space0);

// GLSL std140 buffer → HLSL cbuffer
struct CameraData
{
    float4x4 view;
    float4x4 proj;
};
StructuredBuffer<CameraData> Camera : register(t2, space0);

VSOutput main(VSInput input)
{
    VSOutput output;

    float4x4 view = Camera[0].view;
    float4x4 proj = Camera[0].proj;

    float4x4 modelMatrix = ModelMatrixBlock[PositionIndexBuffer[input.instanceID]];
    float4 worldPos = mul(modelMatrix, float4(input.a_pos, 1.0));

    // В HLSL mul(M, v) — столбцовое умножение
    float3x3 normalMatrix = (float3x3)modelMatrix;
    float3 worldNormal    = normalize(mul(normalMatrix, input.a_normal));
    float3 worldTangent   = normalize(mul(normalMatrix, input.a_tangent));
    float3 worldBitangent = normalize(cross(worldNormal, worldTangent));

    output.position         = mul(proj, mul(view, worldPos));
    output.v_worldPos       = worldPos.xyz;
    output.v_worldNormal    = worldNormal;
    output.v_uv             = input.a_uv;
    output.v_worldTangent   = worldTangent;
    output.v_worldBitangent = worldBitangent;

    return output;
}