#include <lib/gcm_matrices/GcmMatrices.hpp>

using namespace gcm;


template<>
const std::map<Waves::T, int /* number of column in U1 */> GcmMatrices<2, 1, IsotropicMaterial>::WAVE_COLUMNS = {
		{Waves::T::P_FORWARD,  0},
		{Waves::T::P_BACKWARD, 1}
};

template<>
const std::map<Waves::T, int /* number of column in U1 */> GcmMatrices<5, 2, IsotropicMaterial>::WAVE_COLUMNS = {
		{Waves::T::P_FORWARD,   1},
		{Waves::T::P_BACKWARD,  0},
		{Waves::T::S1_FORWARD,  3},
		{Waves::T::S1_BACKWARD, 2}
};

template<>
const std::map<Waves::T, int/* number of column in U1 */> GcmMatrices<9, 3, IsotropicMaterial>::WAVE_COLUMNS = {
		{Waves::T::P_FORWARD,   1},
		{Waves::T::P_BACKWARD,  0},
		{Waves::T::S1_FORWARD,  4},
		{Waves::T::S1_BACKWARD, 2},
		{Waves::T::S2_FORWARD,  5},
		{Waves::T::S2_BACKWARD, 3}
};

template<>
const std::map<Waves::T, int/* number of column in U1 */> GcmMatrices<9, 3, OrthotropicMaterial>::WAVE_COLUMNS = {
		{Waves::T::P_FORWARD,   5},
		{Waves::T::P_BACKWARD,  4},
		{Waves::T::S1_FORWARD,  1},
		{Waves::T::S1_BACKWARD, 0},
		{Waves::T::S2_FORWARD,  3},
		{Waves::T::S2_BACKWARD, 2}
};

template<int M>
real GcmMatrix<M>::getMaximalEigenvalue() const {
	real ans = 0.0;
	for (int i = 0; i < M; i++) {
		ans = fmax(ans, fabs(L(i, i)));
	}
	return ans;
};

template<int TM, int Dimensionality, class TMaterial>
real GcmMatrices<TM, Dimensionality, TMaterial>::getMaximalEigenvalue() const {
	real ans = 0.0;
	for (int i = 0; i < Dimensionality; i++) {
		ans = fmax(ans, A(i).getMaximalEigenvalue());
	}
	return ans;
};


template class GcmMatrices<2, 1, IsotropicMaterial>;
template class GcmMatrices<5, 2, IsotropicMaterial>;
template class GcmMatrices<9, 3, IsotropicMaterial>;
template class GcmMatrices<9, 3, OrthotropicMaterial>;