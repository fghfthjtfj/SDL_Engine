#ifndef SHADOW_HLSL
#define SHADOW_HLSL

static const float MIN_VARIANCE    = 1e-5;
static const float BLEED_REDUCTION = 0.2;

float chebyshev(float2 moments, float t)
{
    float mu = moments.x;
    if (t <= mu) return 1.0;
    float sigma2 = max(moments.y - mu * mu, MIN_VARIANCE);
    float d      = t - mu;
    return sigma2 / (sigma2 + d * d);
}

float reduceBleeding(float p, float amount)
{
    return saturate((p - amount) / (1.0 - amount));
}

int getCubeFace(float3 dir)
{
    float3 a = abs(dir);
    if (a.x >= a.y && a.x >= a.z) return dir.x > 0.0 ? 0 : 1;
    if (a.y >= a.x && a.y >= a.z) return dir.y > 0.0 ? 2 : 3;
    return dir.z > 0.0 ? 4 : 5;
}

// Один слот теневого атласа. Текстура и sampler — параметры.
float computeShadowVSM(
    Texture2DArray<float2> momentsArray,
    SamplerState           momentsSampler,
    float4                 lightSpacePos,
    int                    slot)
{
    if (lightSpacePos.w <= 0.0) return 1.0;

    float3 ndc = lightSpacePos.xyz / lightSpacePos.w;
    float2 uv  = float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
    float  z   = ndc.z;

    // Сэмплируем до условий — иначе ломаются derivative-ы в квадре
    float2 moments = momentsArray.SampleLevel(momentsSampler,
                         float3(saturate(uv), float(slot)), 0);

    if (any(isnan(ndc)) || any(isinf(ndc)) ||
        uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0 ||
        z < 0.0 || z > 1.0)
        return 1.0;

    float visibility = chebyshev(moments, z);
    return reduceBleeding(visibility, BLEED_REDUCTION);
}

#endif