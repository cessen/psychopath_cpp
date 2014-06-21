#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "numtype.h"
#include <cmath>
#include <vector>
#include <iostream>


#include "config.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "transform.hpp"
#include "ray.hpp"

/*
 * A virtual camera.
 */
class Camera
{
public:
	std::vector<Transform> transforms;
	std::vector<float> fovs;
	std::vector<float> tfovs;
	std::vector<float> aperture_radii;
	std::vector<float> focus_distances;

	Camera(std::vector<Transform> &transforms_, std::vector<float> &fovs_, std::vector<float> &aperture_radii_, std::vector<float> &focus_distances_) {
		transforms = transforms_;
		fovs = fovs_;
		aperture_radii = aperture_radii_;
		focus_distances = focus_distances_;

		// Make sure we have needed values for everything
		if (transforms.size() == 0)
			std::cout << "WARNING: camera has no transform(s)!\n";

		if (fovs.size() == 0)
			std::cout << "WARNING: camera has no fov(s)!\n";

		if (aperture_radii.size() == 0 || focus_distances.size() == 0) {
			aperture_radii = {0.0f};
			focus_distances = {1.0f};

			if (aperture_radii.size() == 0 && focus_distances.size() != 0)
				std::cout << "WARNING: camera has aperture radius but no focus distance.  Disabling focal blur.\n";
			else if (aperture_radii.size() != 0 && focus_distances.size() == 0)
				std::cout << "WARNING: camera has focus distance but no aperture radius.  Disabling focal blur.\n";
		}

		// Convert angle fov into linear fov
		tfovs.clear();
		for (auto&& i: fovs)
			tfovs.emplace_back(sin(i/2) / cos(i/2));
		fovs.clear();

		// Can't have focus distance of zero
		for (auto&& f: focus_distances) {
			if (f <= 0.0f) {
				std::cout << "WARNING: camera focal distance is zero or less.  Disabling focal blur.\n";
				aperture_radii = {0.0f};
				focus_distances = {1.0f};
				break;
			}
		}
	}

	/*
	 * Generates a camera ray based on the given information.
	 */
	WorldRay generate_ray(float x, float y, float dx, float dy, float time, float u, float v) const {
		WorldRay wray;

		wray.type = WorldRay::CAMERA;
		wray.time = time;

		// Get time-interpolated camera settings
		const Transform transform = lerp_seq(time, transforms);
		const float tfov = lerp_seq(time, tfovs);
		const float aperture_radius = lerp_seq(time, aperture_radii);
		const float focus_distance = lerp_seq(time, focus_distances);

		// Ray origin
		wray.o.x = aperture_radius * ((u * 2) - 1);
		wray.o.y = aperture_radius * ((v * 2) - 1);
		wray.o.z = 0.0;
		square_to_circle(&wray.o.x, &wray.o.y);

		// Ray direction
		wray.d.x = (x * tfov) - (wray.o.x / focus_distance);
		wray.d.y = (y * tfov) - (wray.o.y / focus_distance);
		wray.d.z = 1.0;
		wray.d.normalize();

		// Ray image plane differentials
		wray.odx = Vec3(0.0f, 0.0f, 0.0f);
		wray.ody = Vec3(0.0f, 0.0f, 0.0f);
		wray.ddx = Vec3(dx*tfov, 0.0f, 0.0f);
		wray.ddy = Vec3(0.0f, dy*tfov, 0.0f);

		// Transform the ray
		return wray.transformed(transform);
	}
};

#endif
