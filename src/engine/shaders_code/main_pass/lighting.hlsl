#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

struct Light {
    float4 position_radius;
    float4 direction_angle;
    float4 color_power;
    int4   light_info;
};

static const float AMBIENT   = 0.75;
static const float EDGE_SOFT = 0.30;

// toLight = lightPos - fragPos, dist = length(toLight) — считаются в main, передаём готовыми
float computeSpotLight(float3 fragPos, float3 normal, Light light, float3 toLight, float dist)
{
    float  nearRadius = light.position_radius.w;
    float3 lightDir   = normalize(light.direction_angle.xyz);
    float  tanHalf    = abs(light.direction_angle.w);
    float  power      = light.color_power.a;
    float  maxRange   = asfloat(light.light_info.z);

    float3 v = -toLight;                            // fragPos - lightPos
    float  L = dot(v, lightDir);
    if (L < 0.0) return 0.0;

    float d2          = dist * dist;
    float coneRadius  = nearRadius + L * tanHalf;
    float distToAxis  = sqrt(max(d2 - L * L, 0.0));
    float radiusAtten = smoothstep(coneRadius, coneRadius * (1.0 - EDGE_SOFT), distToAxis);
    if (radiusAtten <= 0.0) return 0.0;

    float rangeAtten = (maxRange > 1e-4) ? saturate(1.0 - dist / maxRange) : 0.0;

    float NdotL   = dot(normal, toLight / dist);    // toLight/dist = normalize(-v)
    const float wrap = 0.4;
    float diffuse = max((NdotL + wrap) / (1.0 + wrap), 0.0);

    return diffuse * radiusAtten * rangeAtten * power;
}

// toLight = lightPos - fragPos, dist = length(toLight) — считаются в main, передаём готовыми
float computeSphereLight(float3 fragPos, float3 normal, Light light, float3 toLight, float dist)
{
    float  R        = light.position_radius.w;
    float  power    = light.color_power.a;
    float  maxRange = asfloat(light.light_info.z);

    float rangeAtten = (maxRange > 1e-4) ? saturate(1.0 - dist / maxRange) : 0.0;

    if (dist <= R) return power * rangeAtten;

    float3 N        = normalize(normal);
    float3 L        = toLight / dist;               // normalize(toLight)
    float  sinAlpha = clamp(R / dist, 0.0, 1.0);
    float  cosAlpha = sqrt(max(1.0 - sinAlpha * sinAlpha, 0.0));
    float  NL       = dot(N, L);

    float3 D;
    if (NL >= cosAlpha) {
        D = N;
    } else {
        float3 T    = N - L * NL;
        float  tLen = length(T);
        D = tLen < 1e-6 ? L : normalize(L * cosAlpha + (T / tLen) * sinAlpha);
    }

    float dotL = max(dot(N, D), 0.0) * 0.5 + 0.5;
    return dotL * rangeAtten * power;
}

#endif