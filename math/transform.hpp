#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "numtype.h"

#include <string>

#include "utils.hpp"
#include "range.hpp"
#include "vector.hpp"
#include "matrix.hpp"

/**
 * @brief A transformation in 3D space.
 *
 * This is basically a wrapper for a transform matrix.  It defines several
 * convenience methods.
 * Ideally, other code should never have to directly access the matrix, though
 * it is left public just in case.
 */
class Transform
{
public:
	Matrix44 to;

	/*
	 * Constructors.
	 */
	// Initialize as identity
	Transform() {
		to.makeIdentity();
	}
	// Initialize from a single matrix
	Transform(const Matrix44 &to_): to {to_} {}


	/*
	 * Get inverse of this transform.
	 */
	Transform inverse() const {
		return Transform(to.inverse());
	}



	/*
	 * Basic operations.
	 */

	// Assignment from matrix
	const Transform &operator=(const Matrix44 &m) {
		to = m;
		return *this;
	}

	// Composition
	Transform operator*(const Transform &b) const {
		return Transform(to * b.to);
	}
	const Transform &operator*=(const Transform &b) {
		to *= b.to;
		return *this;
	}

	// Addition
	Transform operator+(const Transform &b) const {
		return Transform(to+b.to);
	}
	const Transform &operator+=(const Transform &b) {
		to += b.to;
		return *this;
	}


	// Subtraction
	Transform operator-(const Transform &b) const {
		return Transform(to-b.to);
	}
	const Transform &operator-=(const Transform &b) {
		to -= b.to;
		return *this;
	}


	// Scalar multiplication
	Transform operator*(const float &b) const {
		return Transform(to*b);
	}
	const Transform &operator*=(const float &b) {
		to *= b;
		return *this;
	}


	// Scalar division
	Transform operator/(const float &b) const {
		return Transform(to/b);
	}
	const Transform &operator/=(const float &b) {
		to /= b;
		return *this;
	}


	/*
	 * Methods to calculate or get information about the transform.
	 */
	// Calculates and returns the inverse scale factors of the matrix
	Vec3 get_inv_scale() const {
		Vec3 scale;
		scale[0] = std::sqrt((to[0][0]*to[0][0]) + (to[0][1]*to[0][1]) + (to[0][2]*to[0][2]));
		scale[1] = std::sqrt((to[1][0]*to[1][0]) + (to[1][1]*to[1][1]) + (to[1][2]*to[1][2]));
		scale[2] = std::sqrt((to[2][0]*to[2][0]) + (to[2][1]*to[2][1]) + (to[2][2]*to[2][2]));
		return scale;
	}



	/*
	 * Transforming vectors.
	 *
	 * Right now we're doing weird conversions between Vec3 and Imath::Vec3.
	 * TODO: eliminate the need for these weird conversions
	 */
	// Transform vector as position
	Vec3 pos_to(const Vec3 &v) const {
		const ImathVec3 a(v.x, v.y, v.z);
		ImathVec3 r;

		to.multVecMatrix(a, r);

		return Vec3(r.x, r.y, r.z);
	}
	Vec3 pos_from(const Vec3 &v) const {
		const ImathVec3 a(v.x, v.y, v.z);
		ImathVec3 r;

		to.inverse().multVecMatrix(a, r);

		return Vec3(r.x, r.y, r.z);
	}

	// Transform vector as direction
	Vec3 dir_to(const Vec3 &v) const {
		const ImathVec3 a(v.x, v.y, v.z);
		ImathVec3 r;

		to.multDirMatrix(a, r);

		return Vec3(r.x, r.y, r.z);
	}
	Vec3 dir_from(const Vec3 &v) const {
		const ImathVec3 a(v.x, v.y, v.z);
		ImathVec3 r;

		to.inverse().multDirMatrix(a, r);

		return Vec3(r.x, r.y, r.z);
	}

	// Transform vector as surface normal
	Vec3 nor_to(const Vec3 &v) const {
		const ImathVec3 a(v.x, v.y, v.z);
		ImathVec3 r;

		// Transform by the transpose of the inverse
		r = dir_transform_transpose(to.inverse(), a);

		return Vec3(r.x, r.y, r.z);
	}
	Vec3 nor_from(const Vec3 &v) const {
		const ImathVec3 a(v.x, v.y, v.z);
		ImathVec3 r;

		r = dir_transform_transpose(to, a);

		return Vec3(r.x, r.y, r.z);
	}



	std::string to_string() const {
		std::string s;
		s.append("[");
		s.append(std::to_string(to[0][0]));
		s.append(" ");
		s.append(std::to_string(to[1][0]));
		s.append(" ");
		s.append(std::to_string(to[2][0]));
		s.append(" ");
		s.append(std::to_string(to[3][0]));
		s.append("\n ");

		s.append(std::to_string(to[0][1]));
		s.append(" ");
		s.append(std::to_string(to[1][1]));
		s.append(" ");
		s.append(std::to_string(to[2][1]));
		s.append(" ");
		s.append(std::to_string(to[3][1]));
		s.append("\n ");

		s.append(std::to_string(to[0][2]));
		s.append(" ");
		s.append(std::to_string(to[1][2]));
		s.append(" ");
		s.append(std::to_string(to[2][2]));
		s.append(" ");
		s.append(std::to_string(to[3][2]));
		s.append("\n ");

		s.append(std::to_string(to[0][3]));
		s.append(" ");
		s.append(std::to_string(to[1][3]));
		s.append(" ");
		s.append(std::to_string(to[2][3]));
		s.append(" ");
		s.append(std::to_string(to[3][3]));

		s.append("]");
		return s;
	}


};



/**
 * Merges two vectors of Transforms, interpreting the vectors as
 * being the Transforms over time.  The result is a vector that
 * is the multiplication of the two vectors of Transforms.
 */
static inline std::vector<Transform> merge(const std::vector<Transform>::const_iterator& a_begin, const std::vector<Transform>::const_iterator& a_end,
        const std::vector<Transform>::const_iterator& b_begin, const std::vector<Transform>::const_iterator& b_end)
{
	auto a = make_range(a_begin, a_end);
	auto b = make_range(b_begin, b_end);
	std::vector<Transform> c;

	if (a.size() == 0) {
		c.insert(c.begin(), b.begin(), b.end());
	} else if (b.size() == 0) {
		c.insert(c.begin(), a.begin(), a.end());
	} else if (a.size() == b.size()) {
		for (int i = 0; i < a.size(); ++i)
			c.emplace_back(a[i] * b[i]);
	} else if (a.size() > b.size()) {
		const float s = a.size() - 1;
		for (int i = 0; i < a.size(); ++i)
			c.emplace_back(a[i] * lerp_seq(i/s, b.begin(), b.end()));
	} else if (a.size() < b.size()) {
		const float s = b.size() - 1;
		for (int i = 0; i < b.size(); ++i)
			c.emplace_back(b[i] * lerp_seq(i/s, a.begin(), a.end()));
	}

	return c;
}

static inline std::vector<Transform> merge(const std::vector<Transform>& a, const std::vector<Transform>& b)
{
	return merge(a.cbegin(), a.cend(), b.cbegin(), b.cend());
}

#endif // TRANSFORM_HPP
