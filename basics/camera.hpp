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
		Ray ray;

		ray.time = time;

		// Ray origin
		ray.o.x = lens_diameter * ((u * 2) - 1) * 0.5;
		ray.o.y = lens_diameter * ((v * 2) - 1) * 0.5;
		ray.o.z = 0.0;
		square_to_circle(&ray.o.x, &ray.o.y);

		// Ray direction
		ray.d.x = (x * tfov) - (ray.o.x / focus_distance);
		ray.d.y = (y * tfov) - (ray.o.y / focus_distance);
		ray.d.z = 1.0;
		ray.d.normalize();

		// Ray image plane differentials
		ray.ow = 0.0f;
		ray.dw = std::min(dx*tfov, dy*tfov);
		//ray.odx = Vec3(0.0f, 0.0f, 0.0f);
		//ray.ody = Vec3(0.0f, 0.0f, 0.0f);
		//ray.ddx = Vec3(dx, 0.0f, 0.0f);
		//ray.ddy = Vec3(0.0f, dy, 0.0f);

		//ray.has_differentials = true;

		// Get transform matrix
		uint32_t ia;
		float alpha;

		if (calc_time_interp(transforms.size(), time, &ia, &alpha)) {
			Transform trans;
			trans = lerp(alpha, transforms[ia], transforms[ia+1]);

			ray.apply_transform(trans);
		} else {
			ray.apply_transform(transforms[0]);
		}

		ray.finalize();

		return ray;
	}
};

#endif
