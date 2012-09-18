#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "numtype.h"

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
	Transform(const Matrix44 &to_) {
		to = to_;
	}

	
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
	Transform operator*(const float32 &b) const {
		return Transform(to*b);
	}
	const Transform &operator*=(const float32 &b) {
		to *= b;
		return *this;
	}


	// Scalar division
	Transform operator/(const float32 &b) const {
		return Transform(to/b);
	}
	const Transform &operator/=(const float32 &b) {
		to /= b;
		return *this;
	}



	/*
	 * Transforming vectors.
	 */
	// Transform vector as position
	Vec3 pos_to(const Vec3 &v) const {
		Vec3 r;
		to.multVecMatrix(v, r);
		return r;
	}
	Vec3 pos_from(const Vec3 &v) const {
		Vec3 r;
		to.inverse().multVecMatrix(v, r);
		return r;
	}

	// Transform vector as direction
	Vec3 dir_to(const Vec3 &v) const {
		Vec3 r;
		to.multDirMatrix(v, r);
		return r;
	}
	Vec3 dir_from(const Vec3 &v) const {
		Vec3 r;
		to.inverse().multDirMatrix(v, r);
		return r;
	}

	// Transform vector as surface normal
	Vec3 nor_to(const Vec3 &v) const {
		// Transform by the transpose of the inverse
		return dir_transform_transpose(to.inverse(), v);
	}
	Vec3 nor_from(const Vec3 &v) const {
		return dir_transform_transpose(to, v);
	}




};

#endif // TRANSFORM_HPP
