#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include "ShaderData.h"

using RenderPassName = std::string;

enum MaterialProperty : uint8_t {
    None = 0,
    Z_WRITE = 1 << 0,
    DEPTH_ONLY = 1 << 1,
    MP_CastShadow = 1 << 2,
    MP_ReceiveLight = 1 << 3,
    MP_UI = 1 << 4,
};

struct MaterialDescription
{
    uint16_t required_properties = 0;
    uint16_t forbidden_properties = 0;

    MaterialDescription(
        uint16_t required = 0,
        uint16_t forbidden = 0
    )
        : required_properties(required)
        , forbidden_properties(forbidden)
    {
        Validate();
    }

    bool IsCompatibleWith(const MaterialDescription& pass) const
    {
        if ((required_properties & pass.required_properties) != pass.required_properties)
            return false;

        if (required_properties & pass.forbidden_properties)
            return false;

        return true;
    }

private:
    void Validate() const
    {
        // 1. required и forbidden не должны пересекаться
        if (required_properties & forbidden_properties) {
            SDL_assert(false && "MaterialDescription: required & forbidden overlap");
        }

        // 2. Взаимоисключающие флаги
        //constexpr uint16_t mutually_exclusive[][2] = {
        //    { DepthOnly, Transparent },
        //    { Deferred,  Forward     },
        //    { DepthOnly, ReceiveShadows },
        //    { DepthOnly, MotionVectors },
        //    { Meta, Deferred },
        //    { Meta, Forward },
        //};

        //for (auto& pair : mutually_exclusive) {
        //    if ((required_properties & pair[0]) &&
        //        (required_properties & pair[1]))
        //    {
        //        SDL_assert(false && "MaterialDescription: incompatible required flags");
        //    }
        //}
    }
};



struct Material {
    MaterialDescription description;
    std::unordered_map<RenderPassName, ShaderProgram*> variants;
};