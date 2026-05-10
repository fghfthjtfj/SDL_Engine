float2 main(float4 sv_pos : SV_Position) : SV_Target0
{
    float d = sv_pos.z;
    return float2(d, d * d);
}
