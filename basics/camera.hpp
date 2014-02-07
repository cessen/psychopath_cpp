#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "numtype.h"
#include <cmath>
#include <vector>


#include "config.hpp"
#include "utils.hpp"
#include "timebox.hpp"
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
	TimeBox<Transform> transforms;
	float fov, tfov;
	float lens_diameter, focus_distance;

	Camera(std::vector<Transform> &trans, float fov_, float lens_diameter_, float focus_distance_) {
		transforms.init(trans.size());
		for (uint32_t i=0; i < trans.size(); i++)
			transforms[i] = trans[i];

		fov = fov_;
		tfov = sin(fov/2) / cos(fov/2);

		lens_diameter = lens_diameter_;
		focus_distance = focus_distance_;
	}

	/*
	 * Generates a camera ray based on the given information.
	 */
	Ray generate_ray(float x, float y, float dx, float dy, float time, float u, float v) const {
		WorldRay wray;

		wray.type = Ray::CAMERA;
		wray.time = time;

		// Ray origin
		wray.o.x = lens_diameter * ((u * 2) - 1) * 0.5;
		wray.o.y = lens_diameter * ((v * 2) - 1) * 0.5;
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

		// Get transform matrix
		uint32_t ia;
		float alpha;
		Ray ray;
		if (calc_time_interp(transforms.size(), time, &ia, &alpha)) {
			Transform trans;
			trans = lerp(alpha, transforms[ia], transforms[ia+1]);

			ray = wray.to_ray(trans);
		} else {
			ray = wray.to_ray(transforms[0]);
		}

		return ray;
	}
};

#endif
