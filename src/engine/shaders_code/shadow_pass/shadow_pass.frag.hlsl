cbuffer CurrentCameraUBO : register(b0, space3) {
    int   currentCameraIndex;
    float currentFarRange;
};

float main(float3 viewPosWS : TEXCOORD0) : SV_Depth
{
    return length(viewPosWS) / currentFarRange;
}