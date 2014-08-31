#ifndef CLOSURE_UNION_HPP
#define CLOSURE_UNION_HPP

#include "surface_closure.hpp"

/**
 * A structure that uses type erasure to contain any surface closure.
 *
 * init() should be used to initialize the structure from a surface closure
 * of some kind.
 *
 * get() should be used to utilize the contained closure via the returned
 * SurfaceClosure pointer.
 */
struct SurfaceClosureUnion {
	char data[sizeof(GTRClosure)]; // Should always be the size of the largest surface closure

	/**
	 * Properly initialize the struct from any surface closure.
	 */
	template <class CLOSURE_TYPE>
	void init(CLOSURE_TYPE closure) {
		new(reinterpret_cast<CLOSURE_TYPE*>(this)) CLOSURE_TYPE(closure);
	}

	/**
	 * Return a pointer to the underlying SurfaceClosure.
	 */
	SurfaceClosure* get() {
		return reinterpret_cast<SurfaceClosure*>(this);
	}
};

#endif // CLOSURE_UNION_HPP