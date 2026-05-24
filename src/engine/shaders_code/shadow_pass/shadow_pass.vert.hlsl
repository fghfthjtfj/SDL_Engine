struct VSInput {
    float3 a_pos     : POSITION;
    float2 a_uv      : TEXCOORD0;
    float3 a_normal  : NORMAL;
    float3 a_tangent : TANGENT;
    uint   instanceID : SV_InstanceID;
};


StructuredBuffer<float4x4> ModelMatrixBlock    : register(t0, space0);
StructuredBuffer<int>      PositionIndexBuffer : register(t1, space0);

struct LightCamera {
    float4x4 view;
    float4x4 proj;
};
StructuredBuffer<LightCamera> LightCameras : register(t2, space0);

cbuffer CurrentCameraUBO : register(b0, space1) {
    int   currentCameraIndex;
    float currentFarRange;
};

struct VSOutput {
    float4 sv_pos    : SV_Position;
    float3 viewPosWS : TEXCOORD0;   // view-space позиция, интерполируется линейно
};


VSOutput main(VSInput input)
{
    VSOutput o;
    float4x4 modelMatrix = ModelMatrixBlock[PositionIndexBuffer[input.instanceID]];
    float4   worldPos    = mul(modelMatrix, float4(input.a_pos, 1.0));

    float4x4 view    = LightCameras[currentCameraIndex].view;
    float4   viewPos = mul(view, worldPos);

    o.sv_pos    = mul(LightCameras[currentCameraIndex].proj, viewPos);
    o.viewPosWS = viewPos.xyz;

    // удержание входов — подмешиваем в интерполянт, который не участвует в трансформе
    o.viewPosWS += float3(input.a_uv.x, input.a_normal.x, input.a_tangent.x) * 0.0001 - 0.0001;

    return o;
}