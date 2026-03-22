#include "PCH.h"
#include "PositionStructure.h"

//  адры SpriteSheet
float u_size = 1.0f / 1.0f;
float v_size = 1.0f / 1.0f;

int frame_x = 0;
int frame_y = 0;

float u0 = frame_x * u_size;
float v0 = frame_y * v_size;
float u1 = u0 + u_size;
float v1 = v0 + v_size;

//std::vector<PosUV> MakeVerticesFromFrame()
//{
//    float u0 = frame_x * u_size;
//    float v0 = frame_y * v_size;
//    float u1 = u0 + u_size;
//    float v1 = v0 + v_size;
//
//    return {
//        { -0.8f,  0.8f, u0, v0 }, // top-left
//        {  0.8f,  0.8f, u1, v0 }, // top-right
//        {  0.8f, -0.8f, u1, v1 }, // bottom-right
//        { -0.8f, -0.8f, u0, v1 }  // bottom-left
//    };
//}


//std::vector<PosUVNormal> MakeCubeVerticesNorm()
//{
//    float u0 = frame_x * u_size;
//    float v0 = frame_y * v_size;
//    float u1 = u0 + u_size;
//    float v1 = v0 + v_size;
//
//    return {
//        // Front face (z = +0.8)
//        { -0.8f,  0.8f,  0.8f,  u0, v0,   0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 0.0f },
//        {  0.8f,  0.8f,  0.8f,  u1, v0,   0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 0.0f },
//        {  0.8f, -0.8f,  0.8f,  u1, v1,   0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 0.0f },
//        { -0.8f, -0.8f,  0.8f,  u0, v1,   0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 0.0f },
//
//        // Back face (z = -0.8)
//        { -0.8f,  0.8f, -0.8f,  u1, v0,   0.0f, 0.0f, -1.0f,   -1.0f, 0.0f, 0.0f },
//        {  0.8f,  0.8f, -0.8f,  u0, v0,   0.0f, 0.0f, -1.0f,   -1.0f, 0.0f, 0.0f },
//        {  0.8f, -0.8f, -0.8f,  u0, v1,   0.0f, 0.0f, -1.0f,   -1.0f, 0.0f, 0.0f },
//        { -0.8f, -0.8f, -0.8f,  u1, v1,   0.0f, 0.0f, -1.0f,   -1.0f, 0.0f, 0.0f },
//
//        // Left face (x = -0.8)
//        { -0.8f,  0.8f, -0.8f,  u0, v0,  -1.0f, 0.0f, 0.0f,     0.0f, 0.0f, -1.0f },
//        { -0.8f,  0.8f,  0.8f,  u1, v0,  -1.0f, 0.0f, 0.0f,     0.0f, 0.0f, -1.0f },
//        { -0.8f, -0.8f,  0.8f,  u1, v1,  -1.0f, 0.0f, 0.0f,     0.0f, 0.0f, -1.0f },
//        { -0.8f, -0.8f, -0.8f,  u0, v1,  -1.0f, 0.0f, 0.0f,     0.0f, 0.0f, -1.0f },
//
//        // Right face (x = +0.8)
//        { 0.8f,  0.8f,  0.8f,  u0, v0,   1.0f, 0.0f, 0.0f,      0.0f, 0.0f, 1.0f },
//        { 0.8f,  0.8f, -0.8f,  u1, v0,   1.0f, 0.0f, 0.0f,      0.0f, 0.0f, 1.0f },
//        { 0.8f, -0.8f, -0.8f,  u1, v1,   1.0f, 0.0f, 0.0f,      0.0f, 0.0f, 1.0f },
//        { 0.8f, -0.8f,  0.8f,  u0, v1,   1.0f, 0.0f, 0.0f,      0.0f, 0.0f, 1.0f },
//
//        // Top face (y = +0.8)
//        { -0.8f,  0.8f, -0.8f,  u0, v1,   0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },
//        {  0.8f,  0.8f, -0.8f,  u1, v1,   0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },
//        {  0.8f,  0.8f,  0.8f,  u1, v0,   0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },
//        { -0.8f,  0.8f,  0.8f,  u0, v0,   0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },
//
//        // Bottom face (y = -0.8)
//        { -0.8f, -0.8f,  0.8f,  u1, v0,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f, 0.0f },
//        {  0.8f, -0.8f,  0.8f,  u1, v1,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f, 0.0f },
//        {  0.8f, -0.8f, -0.8f,  u0, v1,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f, 0.0f },
//        { -0.8f, -0.8f, -0.8f,  u0, v0,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f, 0.0f },
//    };
//}


const std::vector<Uint16> IndicesCube = {
    // Front face
    0, 1, 2,  2, 3, 0,
    // Back face
    4, 6, 5,  4, 7, 6,
    // Left face
    8, 9, 10,  10, 11, 8,
    // Right face
    12, 13, 14,  14, 15, 12,
    // Top face
    16, 17, 18,  18, 19, 16,
    // Bottom face
    20, 21, 22,  22, 23, 20,
};


const std::vector<Uint16> IndicesSquare = {
    0, 1, 2,   // первый треугольник
    2, 3, 0    // второй треугольник
};


void UpdateRotationByMouse(float mouse_x, float mouse_y, float &angle_x, float &angle_y, std::vector<MatrixParams>& current_params)
{
	static float last_x = 0, last_y = 0;
    static bool first_mouse = true;
    if (first_mouse) {
        last_x = mouse_x;
        last_y = mouse_y;
        first_mouse = false;
    }
    float sensitivity = 0.2f;
	float dx = mouse_x - last_x;
	float dy = mouse_y - last_y;
    angle_y += dx * sensitivity;
    angle_x += dy * sensitivity;
    // ќграничение вертикального угла
    if (angle_x > 89.0f)  angle_x = 89.0f;
    if (angle_x < -89.0f) angle_x = -89.0f;
    for (auto& param : current_params) {
        param.angleX = glm::radians(angle_x);
        param.angleY = glm::radians(angle_y);
        param.angleZ = 0.0f;
	}

    last_x = mouse_x;
	last_y = mouse_y;
}

constexpr int stacks = 12;    // по широте
constexpr int slices = 24;    // по долготе
constexpr float radius = 0.8f; // чтобы по размеру совпадало с кубом

//std::vector<PosUV> MakeSphereVertices() {
//    std::vector<PosUV> vertices;
//
//    for (int i = 0; i <= stacks; ++i) {
//        float v = float(i) / stacks;
//        float phi = v * 3.1415; // от 0 до pi
//
//        for (int j = 0; j <= slices; ++j) {
//            float u = float(j) / slices;
//            float theta = u * 2.0f * 3.1415f; // от 0 до 2pi
//
//            float x = radius * std::sin(phi) * std::cos(theta);
//            float y = radius * std::cos(phi);
//            float z = radius * std::sin(phi) * std::sin(theta);
//
//            vertices.push_back({ x, y, z, u, v });
//        }
//    }
//    return vertices;
//}

//std::vector<Uint16> MakeSphereIndices() {
//    std::vector<Uint16> indices;
//
//    for (int i = 0; i < stacks; ++i) {
//        for (int j = 0; j < slices; ++j) {
//            Uint16 first = i * (slices + 1) + j;
//            Uint16 second = first + slices + 1;
//
//            // каждый "квадратик" разбиваетс€ на два треугольника
//            indices.push_back(first);
//            indices.push_back(second);
//            indices.push_back(first + 1);
//
//            indices.push_back(second);
//            indices.push_back(second + 1);
//            indices.push_back(first + 1);
//        }
//    }
//    return indices;
//}