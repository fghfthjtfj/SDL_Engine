#include "main_pass/math.hlsl"
#include "main_pass/material.hlsl"
#include "main_pass/lighting.hlsl"
#include "main_pass/shadowPCF.hlsl"

// [[vk::combinedImageSampler]]
// Texture2DArray<float2>  u_shadowMomentsArray : register(t0, space2);
// [[vk::combinedImageSampler]]
// SamplerState            u_shadowMomentsSampler : register(s0, space2);

[[vk::combinedImageSampler]]
Texture2DArray<float>     u_shadowDepthArray  : register(t0, space2);
[[vk::combinedImageSampler]]
SamplerComparisonState    u_shadowSampler     : register(s0, space2);


[[vk::combinedImageSampler]]
Texture2DArray          u_albedo         : register(t1, space2);
[[vk::combinedImageSampler]]
SamplerState            u_albedoSampler  : register(s1, space2);

[[vk::combinedImageSampler]]
Texture2DArray          u_normal         : register(t2, space2);
[[vk::combinedImageSampler]]
SamplerState            u_normalSampler  : register(s2, space2);

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
    float4                                       sv_pos          : SV_Position;
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
static const float VOLUME_BOUNDARY_SOFTNESS = 1.1;

float4 main(PSInput input) : SV_Target0
{
    float3 n = computeNormal(
    u_normal, u_normalSampler, textures[1].data,
    input.v_uv,
    input.v_worldTangent,
    input.v_worldBitangent,
    input.v_worldNormal
);
    float4 albedoSample = sampleAtlas(u_albedo, u_albedoSampler, textures[0].data, input.v_uv);
    float3 baseColor    = albedoSample.rgb;
    float  alpha        = albedoSample.a;
    float3 color        = baseColor * AMBIENT;

    uint lightCount, stride;
    LightBlock.GetDimensions(lightCount, stride);

    for (uint i = 0; i < lightCount; ++i)
    {
        int   type         = LightBlock[i].light_info.x;
        int   cameraOffset = LightBlock[i].light_info.y;
        float intensity    = 0.0;

//         if (type == 0)
//         {
//             intensity = computeSpotLight(input.v_worldPos, n, LightBlock[i]);
//             if (cameraOffset >= 0)
//                 intensity *= computeShadowVSM(
//                     u_shadowMomentsArray, u_shadowMomentsSampler,
//                     getLightSpacePos(input, cameraOffset),
//                     cameraOffset
//                 );
//         }
//         else if (type == 1)
//         {
//             intensity = computeSphereLight(input.v_worldPos, n, LightBlock[i]);
//             if (cameraOffset >= 0)
//             {
//                 int face = getCubeFace(input.v_worldPos - LightBlock[i].position_radius.xyz);
//                 intensity *= computeShadowVSM(
//                     u_shadowMomentsArray, u_shadowMomentsSampler,
//                     getLightSpacePos(input, cameraOffset + face),
//                     cameraOffset + face
//                 );
//             }
//         }
        if (type == 0)
        {
            intensity = computeSpotLight(input.v_worldPos, n, LightBlock[i]);
            if (cameraOffset >= 0 && intensity > 0.0)
            {
                float3 lightDir = normalize(LightBlock[i].position_radius.xyz - input.v_worldPos);
                float  ndotl    = max(dot(n, lightDir), 0.0);

                // Маска grazing-угла: 0 при перпендикуляре к свету, 1 на нормальных углах
                // 0.05 = ~87° от нормали (3° от edge-on)
                // 0.15 = ~81° от нормали (9° от edge-on)
                float grazingMask = smoothstep(0.05, 0.15, ndotl);

                intensity *= computeShadowPCF(
                    u_shadowDepthArray, u_shadowSampler,
                    getLightSpacePos(input, cameraOffset),
                    cameraOffset,
                    input.sv_pos.xy
                ) * grazingMask;
            }
        }
        else if (type == 1)
{
    intensity = computeSphereLight(input.v_worldPos, n, LightBlock[i]);
    if (cameraOffset >= 0 && intensity > 0.0)
    {
        float3 toLight = LightBlock[i].position_radius.xyz - input.v_worldPos;
        float  dist    = length(toLight);
        float  R       = LightBlock[i].position_radius.w;

        float shadow      = 1.0;
        float grazingMask = 1.0;

        if (dist > R)  // Внутри R всё уже 1.0 — экономим PCF
        {
            float3 lightDir     = toLight / dist;
            float  ndotl        = max(dot(n, lightDir), 0.0);
            float  physicalMask = smoothstep(0.05, 0.15, ndotl);

            int face = getCubeFace(-toLight);
            float pcfShadow = computeShadowPCF(
                u_shadowDepthArray, u_shadowSampler,
                getLightSpacePos(input, cameraOffset + face),
                cameraOffset + face,
                input.sv_pos.xy
            );

            // Compile-time const → компилятор схлопнет ветку при softness == 1.0
            float volumeFactor = (VOLUME_BOUNDARY_SOFTNESS > 1.0)
                ? smoothstep(R, R * VOLUME_BOUNDARY_SOFTNESS, dist)
                : 1.0;

            shadow      = lerp(1.0, pcfShadow,    volumeFactor);
            grazingMask = lerp(1.0, physicalMask, volumeFactor);
        }

        intensity *= shadow * grazingMask;
    }
}
        else continue;

        if (intensity <= 0.0) continue;
        color += baseColor * LightBlock[i].color_power.rgb * intensity;
    }

    return float4(color, alpha);
}