#ifndef SHADOW_PCF_HLSL
#define SHADOW_PCF_HLSL

// ─── Configuration ──────────────────────────────────────────────────────────

static const float SHADOW_BIAS   = 0.0005;  // дополнение к slope-scaled bias из rasterizer
static const int   PCF_TAPS      = 16;
static const float PCF_RADIUS_TX = 1.5;     // радиус кернела в текселях shadow map

// 16-точечный Poisson disk, нормирован в [-1, 1]
static const float2 PoissonDisk16[16] = {
    float2( 0.94558609, -0.76890725), float2(-0.09418410, -0.92938870),
    float2( 0.34495938,  0.29387760), float2(-0.91588581,  0.45771432),
    float2(-0.81544232, -0.87912464), float2(-0.38277543,  0.27676845),
    float2( 0.97484398,  0.75648379), float2( 0.44323325, -0.97511554),
    float2( 0.53742981, -0.47373420), float2(-0.26496911, -0.41893023),
    float2( 0.79197514,  0.19090188), float2(-0.24188840,  0.99706507),
    float2(-0.81409955,  0.91437590), float2( 0.19984126,  0.78641367),
    float2( 0.14383161, -0.14100790), float2(-0.41435000,  0.50098030)
};

// Interleaved Gradient Noise — детерминированный per-pixel шум для поворота кернела
float ign(float2 pixel)
{
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}

// ─── Helpers ────────────────────────────────────────────────────────────────

int getCubeFace(float3 dir)
{
    float3 a = abs(dir);
    if (a.x >= a.y && a.x >= a.z) return dir.x > 0.0 ? 0 : 1;
    if (a.y >= a.x && a.y >= a.z) return dir.y > 0.0 ? 2 : 3;
    return dir.z > 0.0 ? 4 : 5;
}

// ─── PCF ────────────────────────────────────────────────────────────────────
float2 computeReceiverPlaneDepthBias(float3 shadowUVZ)
{
    // Производные UV-координат теневой карты и depth-а по экранным осям
    float3 dx = ddx(shadowUVZ);
    float3 dy = ddy(shadowUVZ);

    // Решаем систему: при смещении на (du, dv) в shadow space, насколько меняется z
    float2 bias;
    float det = dx.x * dy.y - dx.y * dy.x;
    if (abs(det) < 1e-7) return float2(0.0, 0.0);
    bias.x = (dy.y * dx.z - dx.y * dy.z) / det;
    bias.y = (dx.x * dy.z - dy.x * dx.z) / det;
    return bias;
}

float computeShadowPCF(
    Texture2DArray<float>  depthArray,
    SamplerComparisonState shadowSampler,
    float4                 lightSpacePos,
    int                    slot,
    float2                 pixelCoord)
{
    if (lightSpacePos.w <= 0.0) return 1.0;

    float3 ndc = lightSpacePos.xyz / lightSpacePos.w;
    float2 uv  = float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
    float  z   = ndc.z;

    if (any(isnan(ndc)) || any(isinf(ndc)) ||
        uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0 ||
        z > 1.0)
        return 1.0;

    // Per-pixel bias на основе градиента поверхности в shadow space
    float2 rpdBias = computeReceiverPlaneDepthBias(float3(uv, z));

    // Клампим чтобы не ушло в бесконечность на разрывах геометрии
    float biasMagnitude = length(rpdBias);
    if (biasMagnitude > 0.01) rpdBias *= 0.01 / biasMagnitude;

    // Минимальный фоновый bias на случай когда ddx/ddy нестабильны (силуэты)
    static const float MIN_BIAS = 0.0001;

    float w, h, layers;
    depthArray.GetDimensions(w, h, layers);
    float2 texelSize = 1.0 / float2(w, h);

    float angle = ign(pixelCoord) * 6.28318530718;
    float c = cos(angle), si = sin(angle);
    float2x2 rot = float2x2(c, -si, si, c);

    float sum = 0.0;
    [unroll]
    for (int i = 0; i < PCF_TAPS; ++i)
    {
        float2 offset = mul(rot, PoissonDisk16[i]) * texelSize * PCF_RADIUS_TX;

        // Корректируем z для этого тапа исходя из наклона поверхности
        float zBiased = z + dot(offset, rpdBias) - MIN_BIAS;

        sum += depthArray.SampleCmpLevelZero(
            shadowSampler,
            float3(saturate(uv + offset), float(slot)),
            zBiased
        );
    }
    return sum / float(PCF_TAPS);
}
#endif