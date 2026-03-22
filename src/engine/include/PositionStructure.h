#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL3/SDL.h>



struct PosUVNormal {
    float x, y, z;       // позиция
    float u, v;          // UV
    float nx, ny, nz;    // normal
    float tx, ty, tz;  // tangent
};


struct MatrixParams {
    float angleX;
    float angleY;
    float angleZ;
    float px;
    float py;
    float pz;
    float scale;
};

void MakeAndStoreModelMatrix(std::vector<uint8_t>& scratchBuffer, std::vector<MatrixParams>& params);


//std::vector<PosUV> MakeCubeVertices();
std::vector<PosUVNormal> MakeCubeVerticesNorm();

//std::vector<PosUV> MakeSphereVertices();
std::vector<Uint16> MakeSphereIndices();
void UpdateRotationByMouse(float mouse_x, float mouse_y, float &angle_x, float &angle_y, std::vector<MatrixParams>& current_params);


extern const std::vector<Uint16> IndicesCube;
extern const std::vector<Uint16> IndicesSquare;