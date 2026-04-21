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
    float4 position          : SV_Position;
    float2 v_uv              : TEXCOORD0;
    float3 v_worldPos        : TEXCOORD1;
    float3 v_worldNormal     : TEXCOORD2;
    float3 v_worldTangent    : TEXCOORD3;
    float3 v_worldBitangent  : TEXCOORD4;
    float4 v_lightSpacePos0  : TEXCOORD5;
    float4 v_lightSpacePos1  : TEXCOORD6;
    float4 v_lightSpacePos2  : TEXCOORD7;
    float4 v_lightSpacePos3  : TEXCOORD8;
    float4 v_lightSpacePos4  : TEXCOORD9;
    float4 v_lightSpacePos5  : TEXCOORD10;
};

// GLSL std430 buffer → HLSL StructuredBuffer
StructuredBuffer<float4x4> ModelMatrixBlock : register(t0, space0);
StructuredBuffer<int>      PositionIndexBuffer : register(t1, space0);

// GLSL std140 buffer → HLSL cbuffer
struct CameraData
{
    float4x4 view;
    float4x4 proj;
};
StructuredBuffer<CameraData> Camera : register(t2, space0);



struct LightCamera
{
    float4x4 view;
    float4x4 proj;
};

StructuredBuffer<LightCamera> LightCameras : register(t3, space0);

VSOutput main(VSInput input)
{
    VSOutput output;

    float4x4 view = Camera[0].view;
    float4x4 proj = Camera[0].proj;

    float4x4 modelMatrix = ModelMatrixBlock[PositionIndexBuffer[input.instanceID]];
    float4 worldPos = mul(modelMatrix, float4(input.a_pos, 1.0));

    // В HLSL mul(M, v) — столбцовое умножение, transpose для нормальной матрицы
    float3x3 normalMatrix = (float3x3)modelMatrix;
    float3 worldNormal    = normalize(mul(normalMatrix, input.a_normal));
    float3 worldTangent   = normalize(mul(normalMatrix, input.a_tangent));
    float3 worldBitangent = normalize(cross(worldNormal, worldTangent));

    output.position = mul(proj, mul(view, worldPos));

    output.v_uv           = input.a_uv;
    output.v_worldPos     = worldPos.xyz;
    output.v_worldNormal  = worldNormal;
    output.v_worldTangent = worldTangent;
    output.v_worldBitangent = worldBitangent;

    // HLSL не поддерживает массивы в выходных структурах с динамическим индексом,
    // поэтому разворачиваем цикл вручную
    output.v_lightSpacePos0 = mul(LightCameras[0].proj, mul(LightCameras[0].view, worldPos));
    output.v_lightSpacePos1 = mul(LightCameras[1].proj, mul(LightCameras[1].view, worldPos));
    output.v_lightSpacePos2 = mul(LightCameras[2].proj, mul(LightCameras[2].view, worldPos));
    output.v_lightSpacePos3 = mul(LightCameras[3].proj, mul(LightCameras[3].view, worldPos));
    output.v_lightSpacePos4 = mul(LightCameras[4].proj, mul(LightCameras[4].view, worldPos));
    output.v_lightSpacePos5 = mul(LightCameras[5].proj, mul(LightCameras[5].view, worldPos));

    return output;
}