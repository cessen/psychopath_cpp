#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "numtype.h"

#include "vector.hpp"
#include "matrix.hpp"

/**
 * @brief A transformation in 3D space.
 *
 * This contains both a transform matrix and its inverse, for convenience
 * when transforming into and out of a space.  This takes up double the space,
 * but the convenience seems worth it so far.
 *
 * This also defines several convenience methods.  Ideally, other code should
 * never have to directly access the matrices, though they are left public
 * just in case.
 */
class Transform
{
public:
	Matrix44 to, from;

	/*
	 * Constructors.
	 */
	// Initialize as identity
	Transform() {
		to.makeIdentity();
		from.makeIdentity();
	}
	// Initialize from a single matrix
	Transform(const Matrix44 &to_) {
		to = to_;
		from = to_.gjInverse();
	}
	// Initalize from a matrix and its already-calculated inverse
	Transform(const Matrix44 &to_, const Matrix44 &from_) {
		to = to_;
		from = from_;
	}



	/*
	 * Basic operations.
	 */

	// Assignment from matrix
	const Transform &operator=(const Matrix44 &m) {
		to = m;
		from = m.inverse();
		return *this;
	}

	// Composition
	Transform operator*(const Transform &b) const {
		return Transform(to * b.to, b.from * from);
	}
	const Transform &operator*=(const Transform &b) {
		to *= b.to;
		from = b.from * from;
		return *this;
	}

	// For basic component-wise arithmetic we could in theory
	// do some inverse operations for the "from" matrix.  But
	// this is ill-defined in some cases, and is not terribly useful
	// in others.  So instead we just apply the same arithmetic
	// to both matrices.  Beware that this means the "from" matrix
	// may not remain as the inverse of "to" in some cases, so be careful!

	// Addition
	Transform operator+(const Transform &b) const {
		return Transform(to+b.to, from+b.from);
	}
	const Transform &operator+=(const Transform &b) {
		to += b.to;
		from += b.from;
		return *this;
	}


	// Subtraction
	Transform operator-(const Transform &b) const {
		return Transform(to-b.to, from-b.from);
	}
	const Transform &operator-=(const Transform &b) {
		to -= b.to;
		to -= b.from;
		return *this;
	}


	// Scalar multiplication
	Transform operator*(const float32 &b) const {
		return Transform(to*b, from*b);
	}
	const Transform &operator*=(const float32 &b) {
		to *= b;
		// This could be flipped to a division, but that's not useful for
		// where it's generally used, which is for interpolation.
		from *= b;
		return *this;
	}


	// Scalar division
	Transform operator/(const float32 &b) const {
		return Transform(to/b, from/b);
	}
	const Transform &operator/=(const float32 &b) {
		to /= b;
		from /= b;
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
		from.multVecMatrix(v, r);
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
		from.multDirMatrix(v, r);
		return r;
	}

	// Transform vector as surface normal
	Vec3 nor_to(const Vec3 &v) const {
		// Transform by the transpose of the inverse
		return dir_transform_transpose(from, v);
	}
	Vec3 nor_from(const Vec3 &v) const {
		return dir_transform_transpose(to, v);
	}




};

#endif // TRANSFORM_HPP
