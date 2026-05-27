#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL3/SDL.h>
#include "ShaderData.h"


struct PosUVNormal {
    float x, y, z;       // позиция
    float u, v;          // UV
    float nx, ny, nz;    // normal
    float tx, ty, tz;  // tangent
};
struct PosOnly { float x, y, z; };

const VertexFormat FMT_PosUVNormal = {
    {
        { POSITION, offsetof(PosUVNormal, x),  SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3 },
        { UV,       offsetof(PosUVNormal, u),  SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2 },
        { NORMAL,   offsetof(PosUVNormal, nx), SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3 },
        { TANGENT,  offsetof(PosUVNormal, tx), SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3 },
    },
    sizeof(PosUVNormal)
};

const VertexFormat FMT_Pos = {
    { { POSITION, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3 } },
    sizeof(PosOnly)
};


//std::vector<PosUV> MakeCubeVertices();
std::vector<PosUVNormal> MakeCubeVerticesNorm();

//std::vector<PosUV> MakeSphereVertices();
std::vector<Uint16> MakeSphereIndices();


extern const std::vector<Uint16> IndicesCube;
extern const std::vector<Uint16> IndicesSquare;