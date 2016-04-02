#ifndef DISPLACEMENT_SHADER_HPP
#define DISPLACEMENT_SHADER_HPP

class DisplacementShader {
	~DisplacementShader() {}

	/**
	 * @brief Evaluates the displacement shader for the given surface
	 *        parameters.
	 *
	 * TODO: differential geometry as input.
	 * TODO: surface normal and normal differentials in output.
	 *
	 * @param u Surface U parameter.
	 * @param v Surface V parameter.
	 * @param id Surface id number.
	 *
	 * @return A BBox, with min an max displacement coordinates
	 */
	virtual BBox evaluate(float32 u, float32 v, uint_i id) = 0;
};

#endif // DISPLACEMENT_SHADER_HPP