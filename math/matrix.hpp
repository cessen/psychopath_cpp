#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "numtype.h"

#include <iostream>
#include <cmath>
#include "vector.hpp"

/**
 * @brief A 4x4 transform matrix.
 *
 * Elements are addressed as Column x Row (e.g. [column][row]).  This is the
 * opposite of normal math notation, but is more consistent with computer
 * graphics in e.g. [x][y] notation.
 */
class Matrix44
{
public:
	float32 elements[4][4];

	// Set the matrix to be the identity matrix
	// Wipes out the existing contents of the matrix
	void identity() {
		elements[0][1]
		= elements[0][2]
		  = elements[0][3]
		    = elements[1][0]
		      = elements[1][2]
		        = elements[1][3]
		          = elements[2][0]
		            = elements[2][1]
		              = elements[2][3]
		                = elements[3][0]
		                  = elements[3][1]
		                    = elements[3][2]
		                      = 0.0;

		elements[0][0]
		= elements[1][1]
		  = elements[2][2]
		    = elements[3][3]
		      = 1.0;
	}

	/*
	 * Constructors.
	 */
	Matrix44() {
		// Default is identity matrix
		identity();
	}

	Matrix44(const Matrix44 &m) {
		// Copy the input matrix
		*this = m;
	}


	/*
	 * Transform the matrix in various ways.
	 */
	// Translate
	void translate(const Vec3 &trans) {
		elements[3][0] += trans[0];
		elements[3][1] += trans[1];
		elements[3][2] += trans[2];
	}

	// Rotate
	void rotate(const float32 &angle, Vec3 axis) {
		if (angle != 0.0) {
			Matrix44 r;
			axis.normalize();

			const float32 s = std::sin(angle);
			const float32 c = std::cos(angle);
			const float32 t = 1.0 - c;

			r.elements[0][0] = t * axis.x * axis.x + c;
			r.elements[1][1] = t * axis.y * axis.y + c;
			r.elements[2][2] = t * axis.z * axis.z + c;

			const float32 txy = t * axis.x * axis.y;
			const float32 sz = s * axis.z;

			r.elements[0][1] = txy + sz;
			r.elements[1][0] = txy - sz;

			const float32 txz = t * axis.x * axis.z;
			const float32 sy = s * axis.y;

			r.elements[0][2] = txz - sy;
			r.elements[2][0] = txz + sy;

			const float32 tyz = t * axis.y * axis.z;
			const float32 sx = s * axis.x;

			r.elements[1][2] = tyz + sx;
			r.elements[2][1] = tyz - sx;

			*this = r * (*this);
		}
	}

	// Scale
	void scale(const Vec3 &scl) {
		for (int row=0; row < 2; row++) {
			for (int col=0; col < 3; col++) {
				elements[col][row] *= scl[row];
			}
		}
	}

	// Invert
	// Uses Gauss-Jordan elimination with partial pivoting (see for
	// example, Graphics Gems IV (p554)).
	// Returns true if successful, false if not
	// This can leave the matrix in a strange state on failure
	bool invert() {
		Matrix44 a(*this);	// a evolves from original matrix into identity
		(*this).identity();

		int i;
		int j;
		int i1;

		// Loop over cols of a from left to right, eliminating above and below diag
		for (j = 0; j < 4; j++) {  	// Find largest pivot in column j among rows j..3
			i1 = j;
			for (i = j + 1; i < 4; i++) {
				if (std::fabs(a.elements[i][j]) > std::fabs(a.elements[i1][j])) {
					i1 = i;
				}
			}

			if (i1 != j) {
				// Swap rows i1 and j in a and *this to put pivot on diagonal
				float32 temp;

				temp = a.elements[i1][0];
				a.elements[i1][0] = a.elements[j][0];
				a.elements[j][0] = temp;
				temp = a.elements[i1][1];
				a.elements[i1][1] = a.elements[j][1];
				a.elements[j][1] = temp;
				temp = a.elements[i1][2];
				a.elements[i1][2] = a.elements[j][2];
				a.elements[j][2] = temp;
				temp = a.elements[i1][3];
				a.elements[i1][3] = a.elements[j][3];
				a.elements[j][3] = temp;

				temp = (*this).elements[i1][0];
				(*this).elements[i1][0] = (*this).elements[j][0];
				(*this).elements[j][0] = temp;
				temp = (*this).elements[i1][1];
				(*this).elements[i1][1] = (*this).elements[j][1];
				(*this).elements[j][1] = temp;
				temp = (*this).elements[i1][2];
				(*this).elements[i1][2] = (*this).elements[j][2];
				(*this).elements[j][2] = temp;
				temp = (*this).elements[i1][3];
				(*this).elements[i1][3] = (*this).elements[j][3];
				(*this).elements[j][3] = temp;
			}

			// Scale row j to have a unit diagonal
			if (a.elements[j][j] == 0.0f) {
				return false;
			}
			float32 scale = 1.0f / a.elements[j][j];
			(*this).elements[j][0] *= scale;
			(*this).elements[j][1] *= scale;
			(*this).elements[j][2] *= scale;
			(*this).elements[j][3] *= scale;
			// all elements to left of a[j][j] are already zero
			for (i1 = j + 1; i1 < 4; i1++) {
				a.elements[j][i1] *= scale;
			}
			a.elements[j][j] = 1.0f;

			// Eliminate off-diagonal elements in column j of a, doing identical ops to b
			for (i = 0; i < 4; i++) {
				if (i != j) {
					scale = a.elements[i][j];
					(*this).elements[i][0] -= scale * (*this).elements[j][0];
					(*this).elements[i][1] -= scale * (*this).elements[j][1];
					(*this).elements[i][2] -= scale * (*this).elements[j][2];
					(*this).elements[i][3] -= scale * (*this).elements[j][3];

					// all elements to left of a[j][j] are zero
					// a[j][j] is 1.0
					for (i1 = j + 1; i1 < 4; i1++) {
						a.elements[i][i1] -= scale * a.elements[j][i1];
					}
					a.elements[i][j] = 0.0f;
				}
			}
		}

		return true;
	}


	/*
	 * Easy access to elements.
	 */
	float32 *operator[](const int &i) {
		return &elements[i][0];
	}

	const float32 *operator[](const int &i) const {
		return &elements[i][0];
	}

	/*
	 * Matrix<-->Matrix operations.
	 */
	// Multiplication
	Matrix44 operator*(const Matrix44 &m) const {
		Matrix44 result;
		for (int row=0; row < 4; row++) {
			for (int col=0; col < 4; col++) {
				result[col][row] = 0.0;
				for (int i=0; i < 4; i++) {
					result[col][row] += elements[i][row] * m[col][i];
				}
			}
		}

		return result;
	}
	void operator*=(const Matrix44 &m) {
		(*this) = (*this) * m;
	}

	// Addition
	void operator+=(const Matrix44 &m) {
		for (int row=0; row < 4; row++) {
			for (int col=0; col < 4; col++) {
				elements[col][row] += m[col][row];
			}
		}
	}
	Matrix44 operator+(const Matrix44 &m) const {
		Matrix44 result(*this);
		for (int row=0; row < 4; row++) {
			for (int col=0; col < 4; col++) {
				result[col][row] += m[col][row];
			}
		}

		return result;
	}


	/*
	 * Matrix44<-->Vec3 operations.
	 */
	// Pretends that Vec3 is a Vec4 with a 4th component of 0.0.
	// Good for direction vectors
	Vec3 mult_dir(const Vec3 &v) const {
		Vec3 result;

		for (int row=0; row < 3; row++) {
			result[row] = (elements[0][row] * v[0])
			              + (elements[1][row] * v[1])
			              + (elements[2][row] * v[2]);
		}

		return result;
	}
	// Pretends that Vec3 is a Vec4 with a 4th component of 1.0.
	// Good for position vectors
	Vec3 mult_pos(const Vec3 &v) const {
		Vec3 result;

		for (int row=0; row < 3; row++) {
			result[row] = (elements[0][row] * v[0])
			              + (elements[1][row] * v[1])
			              + (elements[2][row] * v[2])
			              + elements[3][row];
		}

		return result;
	}
	Vec3 operator*(const Vec3 &v) const {
		return mult_pos(v);
	}

	/*
	 * Matrix<-->Scalar operations
	 */
	// Multiplication
	void operator*=(const float32 &f) {
		for (int row=0; row < 4; row++) {
			for (int col=0; col < 4; col++) {
				elements[col][row] *= f;
			}
		}
	}
	Matrix44 operator*(const float32 &f) const {
		Matrix44 result(*this);
		for (int row=0; row < 4; row++) {
			for (int col=0; col < 4; col++) {
				result[col][row] *= f;
			}
		}

		return result;
	}

};

#endif
