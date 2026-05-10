#ifndef MATH_HLSL
#define MATH_HLSL

float2 unpackUnorm2x16(uint p)
{
    return float2(
        float(p & 0xFFFFu) / 65535.0,
        float(p >> 16u)    / 65535.0
    );
}

#endif