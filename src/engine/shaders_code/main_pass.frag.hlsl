// ─── Bindings ─────────────────────────────────────────────────────────────────

[[vk::combinedImageSampler]]
Texture2DArray          u_shadowMapFlat  : register(t0, space2);
[[vk::combinedImageSampler]]
SamplerComparisonState  u_shadowSampler  : register(s0, space2);

[[vk::combinedImageSampler]]
Texture2DArray          u_albedo         : register(t1, space2);
[[vk::combinedImageSampler]]
SamplerState            u_albedoSampler  : register(s1, space2);

[[vk::combinedImageSampler]]
Texture2DArray          u_normal         : register(t2, space2);
[[vk::combinedImageSampler]]
SamplerState            u_normalSampler  : register(s2, space2);

struct Light {
    float4 position_radius;
    float4 direction_angle;
    float4 color_power;
    int4   light_info;
};

StructuredBuffer<Light> LightBlock : register(t3, space2);

struct TextureData {
    uint4 data;
};

cbuffer TextureUVLBlock : register(b0, space3) {
    TextureData textures[4];
};

// ─── Input ───────────────────────────────────────────────────────────────────

struct PSInput
{
    [[vk::location(0)]]  float2 v_uv             : TEXCOORD0;
    [[vk::location(1)]]  float3 v_worldPos       : TEXCOORD1;
    [[vk::location(2)]]  float3 v_worldNormal    : TEXCOORD2;
    [[vk::location(3)]]  float3 v_worldTangent   : TEXCOORD3;
    [[vk::location(4)]]  float3 v_worldBitangent : TEXCOORD4;
    [[vk::location(5)]]  float4 v_lsp0           : TEXCOORD5;
    [[vk::location(6)]]  float4 v_lsp1           : TEXCOORD6;
    [[vk::location(7)]]  float4 v_lsp2           : TEXCOORD7;
    [[vk::location(8)]]  float4 v_lsp3           : TEXCOORD8;
    [[vk::location(9)]]  float4 v_lsp4           : TEXCOORD9;
    [[vk::location(10)]] float4 v_lsp5           : TEXCOORD10;
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

float2 unpackUnorm2x16(uint p)
{
    return float2(
        float(p & 0xFFFFu) / 65535.0,
        float(p >> 16u)    / 65535.0
    );
}

// HLSL не поддерживает динамическую индексацию выходов/входов — разворачиваем вручную
float4 getLightSpacePos(PSInput input, int i)
{
    switch (i)
    {
        case 0:  return input.v_lsp0;
        case 1:  return input.v_lsp1;
        case 2:  return input.v_lsp2;
        case 3:  return input.v_lsp3;
        case 4:  return input.v_lsp4;
        default: return input.v_lsp5;
    }
}

float4 sampleAtlas(Texture2DArray atlas, SamplerState samp, uint slot, float2 uv)
{
    float2 offset = unpackUnorm2x16(textures[slot].data.x);
    float2 scale  = unpackUnorm2x16(textures[slot].data.y);
    uint   layer  = textures[slot].data.z;
    return atlas.Sample(samp, float3(uv * scale + offset, float(layer)));
}

float3 computeNormal(PSInput input)
{
    // GLSL: mat3(T,B,N) — колонки; HLSL: float3x3(T,B,N) — строки
    // GLSL: TBN * n == HLSL: mul(n, TBN) — результат идентичен
    float3x3 TBN = float3x3(
        normalize(input.v_worldTangent),
        normalize(input.v_worldBitangent),
        normalize(input.v_worldNormal)
    );
    float3 n = sampleAtlas(u_normal, u_normalSampler, 1, input.v_uv).rgb * 2.0 - 1.0;
    n.x = -n.x;
    return normalize(mul(n, TBN));
}

// ─── Lighting ────────────────────────────────────────────────────────────────

static const float AMBIENT   = 0.75;
static const float EDGE_SOFT = 0.30;

float computeSpotLight(float3 fragPos, float3 normal, Light light)
{
    float3 lightPos   = light.position_radius.xyz;
    float  nearRadius = light.position_radius.w;
    float3 lightDir   = normalize(light.direction_angle.xyz);
    float  tanHalf    = abs(light.direction_angle.w);
    float  power      = light.color_power.a;

    float3 v = fragPos - lightPos;
    float  L = dot(v, lightDir);
    if (L < 0.0) return 0.0;

    float coneRadius  = nearRadius + L * tanHalf;
    float distToAxis  = length(v - lightDir * L);
    float edge1       = coneRadius;
    float edge0       = coneRadius * (1.0 - EDGE_SOFT);
    float radiusAtten = smoothstep(edge1, edge0, distToAxis);
    if (radiusAtten <= 0.0) return 0.0;

    float  dist        = length(v);
    float  attenuation = 1.0 / (1.0 + 0.22 * dist + 0.2 * dist * dist);
    float3 lDir        = normalize(lightPos - fragPos);
    float  NdotL       = dot(normal, lDir);

    const float wrap = 0.4;
    float diffuse    = max((NdotL + wrap) / (1.0 + wrap), 0.0);

    return diffuse * attenuation * radiusAtten * power;
}

float computeSphereLight(float3 fragPos, float3 normal, Light light)
{
    float3 C     = light.position_radius.xyz;
    float  R     = light.position_radius.w;
    float  power = light.color_power.a;

    float3 toC        = C - fragPos;
    float  dist       = length(toC);
    float  d0         = max(dist - R, 0.0);
    float  attenuation = 1.0 / (1.0 + 0.22 * d0 + 0.2 * d0 * d0);

    if (dist <= R) return attenuation * power;

    float3 L        = toC / dist;
    float  sinAlpha = clamp(R / dist, 0.0, 1.0);
    float  cosAlpha = sqrt(max(1.0 - sinAlpha * sinAlpha, 0.0));
    float  NL       = dot(normalize(normal), L);

    float3 D;
    if (NL >= cosAlpha) {
        D = normalize(normal);
    } else {
        float3 T    = normalize(normal) - L * NL;
        float  tLen = length(T);
        D = tLen < 1e-6 ? L : normalize(L * cosAlpha + (T / tLen) * sinAlpha);
    }

    float dotL = max(dot(normalize(normal), D), 0.0) * 0.5 + 0.5;
    return dotL * attenuation * power;
}

// ─── Shadows ─────────────────────────────────────────────────────────────────

float computeShadowSpot(PSInput input, int cameraOffset)
{
    float4 lsp = getLightSpacePos(input, cameraOffset);
    float3 ndc = lsp.xyz / lsp.w;
    float2 uv  = float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
    float  z   = ndc.z;

    if (any(isnan(ndc)) || any(isinf(ndc)) ||
        uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0 ||
        z < -0.01  || z > 1.01)
        return 1.0;

    return u_shadowMapFlat.SampleCmpLevelZero(u_shadowSampler,
               float3(uv, float(cameraOffset)), z);
}

int getCubeFace(float3 dir)
{
    float3 a = abs(dir);
    if (a.x >= a.y && a.x >= a.z) return dir.x > 0.0 ? 0 : 1;
    if (a.y >= a.x && a.y >= a.z) return dir.y > 0.0 ? 2 : 3;
    return dir.z > 0.0 ? 4 : 5;
}

float computeShadowSphere(PSInput input, float3 fragPos, float3 lightPos, int cameraOffset)
{
    float3 dir  = fragPos - lightPos;
    int    face = getCubeFace(dir);

    float4 lsp = getLightSpacePos(input, cameraOffset + face);
    if (lsp.w <= 0.0) return 1.0;

    float3 ndc = lsp.xyz / lsp.w;
    float2 uv  = float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
    float  z   = clamp(ndc.z - 0.0001, 0.0, 1.0);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return 1.0;

    return u_shadowMapFlat.SampleCmpLevelZero(u_shadowSampler,
               float3(uv, float(cameraOffset + face)), z);
}

// ─── Main ────────────────────────────────────────────────────────────────────

float4 main(PSInput input) : SV_Target0
{
    float3 n            = computeNormal(input);
    float4 albedoSample = sampleAtlas(u_albedo, u_albedoSampler, 0, input.v_uv);
    float3 baseColor    = albedoSample.rgb;
    float  alpha        = albedoSample.a;
    float3 color        = baseColor * AMBIENT;

    uint lightCount, stride;
    LightBlock.GetDimensions(lightCount, stride);

    for (uint i = 0; i < lightCount; ++i)
    {
        int   type         = LightBlock[i].light_info.x;
        int   cameraOffset = LightBlock[i].light_info.y; // СЧИТАЕТСЯ ГЛОБАЛЬНО ПО ИСТОЧНИКАМ СВЕТА! Используется в ОБЩЕМ буфере световых камер
        float intensity    = 0.0;

        if (type == 0)
        {
            intensity = computeSpotLight(input.v_worldPos, n, LightBlock[i]);
            if (cameraOffset >= 0)
                intensity *= computeShadowSpot(input, cameraOffset);
        }
        else if (type == 1)
        {
            intensity = computeSphereLight(input.v_worldPos, n, LightBlock[i]);
            if (cameraOffset >= 0)
                intensity *= computeShadowSphere(input, input.v_worldPos,
                                 LightBlock[i].position_radius.xyz, cameraOffset);
        }
        else continue;

        if (intensity <= 0.0) continue;
        color += baseColor * LightBlock[i].color_power.rgb * intensity;
    }

    return float4(color, alpha);
}