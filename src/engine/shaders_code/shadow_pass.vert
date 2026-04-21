struct VSInput {
    float3 a_pos     : POSITION;
    float2 a_uv      : TEXCOORD0;  // не используется, но нужен для правильного stride
    float3 a_normal  : NORMAL;     // не используется
    float3 a_tangent : TANGENT;    // не используется
    uint instanceID  : SV_InstanceID;
};

StructuredBuffer<float4x4> ModelMatrixBlock    : register(t0, space0);
StructuredBuffer<int>      PositionIndexBuffer : register(t1, space0);

struct LightCamera {
    float4x4 view;
    float4x4 proj;
};
StructuredBuffer<LightCamera> LightCameras : register(t2, space0);

cbuffer CurrentCameraUBO : register(b0, space1) {
    int currentCameraIndex;
};

float4 main(VSInput input) : SV_Position
{
    float4x4 modelMatrix = ModelMatrixBlock[PositionIndexBuffer[input.instanceID]];
    float4 worldPos = mul(modelMatrix, float4(input.a_pos, 1.0));

    // Не даём DXC вырезать атрибуты — прибавляем 0 к worldPos
    worldPos.w += (input.a_uv.x + input.a_normal.x + input.a_tangent.x) * 0.0001
                  - 0.0001; // результат всегда 0, но компилятор не знает

    return mul(LightCameras[currentCameraIndex].proj,
               mul(LightCameras[currentCameraIndex].view, worldPos));
}