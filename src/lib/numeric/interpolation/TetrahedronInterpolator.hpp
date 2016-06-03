#ifndef LIBGCM_TETRAHEDRONINTERPOLATOR_HPP
#define LIBGCM_TETRAHEDRONINTERPOLATOR_HPP

#include <lib/linal/linal.hpp>


namespace gcm {
template<class TValue>
class TetrahedronInterpolator {
public:
	typedef linal::VECTOR<3, TValue>              Gradient;
	typedef linal::SYMMETRIC_MATRIX<3, TValue>    Hessian;

	/**
	 * Linear interpolation in non-degenerate tetrahedron
	 * @param c_i and v_i - points and values
	 * @param q point to interpolate
	 */
	static TValue interpolate(const Real3& c0, const TValue v0,
	                          const Real3& c1, const TValue v1,
	                          const Real3& c2, const TValue v2,
	                          const Real3& c3, const TValue v3,
	                          const Real3& q) {

		Real4 lambda = linal::barycentricCoordinates(c0, c1, c2, c3, q);
		return lambda(0) * v0 + 
		       lambda(1) * v1 + 
		       lambda(2) * v2 +
		       lambda(3) * v3;
	}
	
	
	/**
	 * Quadratic interpolation in non-degenerate tetrahedron
	 * @param c_i points
	 * @param v_i values
	 * @param g_i gradients
	 * @param q point to interpolate
	 */
	static TValue interpolate(
			const Real3& c0, const TValue v0, const Gradient g0,
			const Real3& c1, const TValue v1, const Gradient g1,
			const Real3& c2, const TValue v2, const Gradient g2,
			const Real3& c3, const TValue v3, const Gradient g3,
			const Real3& q) {

		Real4 lambda = linal::barycentricCoordinates(c0, c1, c2, c3, q);
		return lambda(0) * (v0 + linal::dotProduct(g0, q - c0) / 2.0) +
		       lambda(1) * (v1 + linal::dotProduct(g1, q - c1) / 2.0) +
		       lambda(2) * (v2 + linal::dotProduct(g2, q - c2) / 2.0) +
		       lambda(3) * (v3 + linal::dotProduct(g3, q - c3) / 2.0);
	}
	
	
	/**
	 * Given with 6 point-value pairs, determine among them tetrahedron that
	 * contains the query point inside and perform linear interpolation in it
	 * @param c_i and v_i - points and values
	 * @param q point to interpolate
	 */
	static TValue interpolateInOwner(const Real3& c0, const TValue v0,
	                                 const Real3& c1, const TValue v1,
	                                 const Real3& c2, const TValue v2,
	                                 const Real3& c3, const TValue v3,
	                                 const Real3& c4, const TValue v4,
	                                 const Real3& c5, const TValue v5,
	                                 const Real3& q) {
		Real4 lambda;
		
		#define TRY_TETR(a, b, d, e) \
			lambda = linal::barycentricCoordinates(c##a, c##b, c##d, c##e, q); \
			if (lambda(0) >= 0 && lambda(1) >= 0 && lambda(2) >= 0 && lambda(3) >= 0) { \
				return lambda(0) * v##a + lambda(1) * v##b + lambda(2) * v##d + lambda(3) * v##e; \
			}
		
		TRY_TETR(0, 1, 2, 3)
		TRY_TETR(0, 1, 2, 4)
		TRY_TETR(0, 1, 2, 5)
		TRY_TETR(0, 1, 3, 4)
		TRY_TETR(0, 1, 3, 5)
		TRY_TETR(0, 1, 4, 5)
		
		TRY_TETR(0, 2, 3, 4)
		TRY_TETR(0, 2, 3, 5)
		TRY_TETR(0, 2, 4, 5)
		
		TRY_TETR(0, 3, 4, 5)
		
		TRY_TETR(1, 2, 3, 4)
		TRY_TETR(1, 2, 3, 5)
		TRY_TETR(1, 2, 4, 5)
		
		TRY_TETR(1, 3, 4, 5)
		
		TRY_TETR(2, 3, 4, 5)
		
		#undef TRY_TETR
		
		THROW_INVALID_ARG("Containing tetrahedron is not found");
}
	
};


}

#endif // LIBGCM_TETRAHEDRONINTERPOLATOR_HPP