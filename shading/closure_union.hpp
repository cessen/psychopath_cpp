#ifndef CLOSURE_UNION_HPP
#define CLOSURE_UNION_HPP

#include <type_traits>
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
	// The following should always be the size and alignment of the
	// largest and largest-aligning surface closure, respectively.
	alignas(GTRClosure) char data[sizeof(GTRClosure)];

	/**
	 * Properly initialize the struct from any surface closure.
	 */
	template <class CLOSURE_TYPE>
	void init(CLOSURE_TYPE closure) {
		static_assert(std::is_base_of<SurfaceClosure, CLOSURE_TYPE>::value, "CLOSURE_TYPE is not derived from SurfaceClosure.");
		new(reinterpret_cast<CLOSURE_TYPE*>(data)) CLOSURE_TYPE(closure);
	}

	/**
	 * Return a pointer to the underlying SurfaceClosure.
	 */
	SurfaceClosure* get() {
		return reinterpret_cast<SurfaceClosure*>(data);
	}

	/**
	 * Return a pointer to the underlying SurfaceClosure.
	 */
	const SurfaceClosure* get() const {
		return reinterpret_cast<const SurfaceClosure*>(data);
	}
};

#endif // CLOSURE_UNION_HPP