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

    float3 toC         = C - fragPos;
    float  dist        = length(toC);
    float  d0          = max(dist - R, 0.0);
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

#endif