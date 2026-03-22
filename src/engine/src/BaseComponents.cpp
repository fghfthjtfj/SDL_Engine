#include "PCH.h"
#include "BaseComponents.h"

void Velocities::MoveByAccelerations(const std::vector<float>& ax, const std::vector<float>& ay, const std::vector<float>& az)
{
    size_t count = x.size();
	if (ax.size() != count) {
		SDL_Log("Velocities::MoveByAccelerations: ax size mismatch!");
		return;
	}
    for (size_t i = 0; i < count; ++i) {
        x[i] += ax[i];
        y[i] += ay[i];
        z[i] += az[i];
    }
}

void Positions::MoveByVelocities(const std::vector<float>& vx, const std::vector<float>& vy, const std::vector<float>& vz)
{
    size_t count = x.size();
    if (vx.size() != count) {
		SDL_Log("Positions::MoveByVelocities: vx size mismatch!");
		return;
    }
    for (size_t i = 0; i < count; ++i) {
        w[i] += vx[i];
        d[i] += vy[i];
        h[i] += vz[i];
    }
}
