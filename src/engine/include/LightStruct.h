#pragma once
#include <glm/glm.hpp>

//struct LightData {
//    glm::vec4 position_radius;     // xyz = позиция, w = радиус источника (ширина в месте выхода)
//    glm::vec4 direction_angle;     // xyz = направление (нормализованный вектор), w = тангенс угла
//    glm::vec4 color_power;         // rgb = цвет, a = мощность
//};

struct LightLayout {
	float x, y, z; // position						// 12 байт
	float w; // source radius (усечённый конус)		// 16 байта
	float dir_x, dir_y, dir_z;						// 28 байта
	float angle_tan; // tangent of the angle		// 32 байт
	float r, g, b; // color							// 44 байт
	float power;									// 48 байта
	int type;										// 52 байт
	int offset;										// 56 байта
	int padding = 0;								// 60 байт
	int padding2 = 0;								// 64 байт
};

//static const glm::vec3 cubeDirs[6] = {
//	{ -1,  0,  0}, {1,  0,  0},  // +X, -X
//	{ 0,  1,  0}, { 0, -1,  0},  // +Y, -Y
//	{ 0,  0,  1}, { 0,  0, -1}   // +Z, -Z
//};
//static const glm::vec3 cubeUps[6] = {
//	{ 0,  1,  0}, { 0,  1,  0},  // +X, -X (вверх для Vulkan Y-flip)
//	{ 0,  0, -1}, { 0,  0,  1},  // +Y, -Y (инвертированный Z)
//	{ 0,  1,  0}, { 0,  1,  0}   // +Z, -Z (вверх для Vulkan Y-flip)
//};
//

