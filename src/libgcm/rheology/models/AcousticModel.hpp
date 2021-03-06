#ifndef LIBGCM_ACOUSTICMODEL_HPP
#define LIBGCM_ACOUSTICMODEL_HPP

#include <libgcm/rheology/models/Model.hpp>


namespace gcm {


/**
 * The model where tension tensor reduced to single pressure scalar.
 */
template<int Dimensionality>
class AcousticModel {
public:
	static const Models::T Type = Models::T::ACOUSTIC;
	static const int DIMENSIONALITY = Dimensionality;
	
	typedef AcousticVariables<DIMENSIONALITY> PdeVariables;
	typedef typename PdeVariables::PdeVector  PdeVector;
	
	typedef linal::Vector<DIMENSIONALITY>     RealD;
	typedef linal::Matrix<DIMENSIONALITY,
	                      DIMENSIONALITY>     MatrixDD;
	
	static const int PDE_SIZE = PdeVector::M;
	
	typedef GcmMatrices<PDE_SIZE, DIMENSIONALITY> GCM_MATRICES;
	typedef typename GCM_MATRICES::GcmMatrix      GcmMatrix;
	typedef typename GCM_MATRICES::Matrix         Matrix;
	typedef void                                  OdeVariables;
	typedef std::shared_ptr<GCM_MATRICES>         GcmMatricesPtr;
	typedef std::shared_ptr<const GCM_MATRICES>   ConstGcmMatricesPtr;
	
	static const MaterialsWavesMap MATERIALS_WAVES_MAP;
	
	/// Number of characteristics with slopes of the same sign.
	/// It is equal to number of outer characteristics in border node.
	static const int OUTER_NUMBER = 1;
	/// Matrix of linear border condition "B * \vec{u} = b"
	/// @see BorderCondition
	typedef linal::Matrix<OUTER_NUMBER, PDE_SIZE> BorderMatrix;
	/// Vector of linear border condition "B * \vec{u} = b"
	typedef linal::Vector<OUTER_NUMBER>           BorderVector;
	/// Matrix of outer eigenvectors
	typedef linal::Matrix<PDE_SIZE, OUTER_NUMBER> OuterMatrix;
	
	typedef std::vector<int> WaveIndices;
	/// Indices of invariants with positive eigenvalues (sorted ascending)
	static const WaveIndices  LEFT_INVARIANTS;
	/// Indices of invariants with negative eigenvalues (sorted ascending)
	static const WaveIndices RIGHT_INVARIANTS;
	
	/**
	 * Construct gcm matrices for calculation in given basis
	 */
	static void constructGcmMatrices(GcmMatricesPtr m,
			std::shared_ptr<const IsotropicMaterial> material,
			const MatrixDD& basis = MatrixDD::Identity()) {
		m->basis = basis;
		for (int i = 0; i < DIMENSIONALITY; i++) {
			RealD n = basis.getColumn(i);
			constructGcmMatrix((*m)(i), material, linal::createLocalBasis(n));
		}
	}
	
	
	/**
	 * Construct gcm matrix along the given direction.
	 * @see ElasticModel::constructGcmMatrix for details
	 */
	inline static void 
	constructGcmMatrix(
			GcmMatrix& m, std::shared_ptr<const IsotropicMaterial> material,
			const MatrixDD& basis, const real l = 1);
	
	/** Construct matrix U1 for given basis. @see constructGcmMatrix */
	static void constructEigenvectors(Matrix& u1,
			std::shared_ptr<const IsotropicMaterial> material, const MatrixDD& basis);
	
	/** Construct matrix U for given basis. @see constructGcmMatrix */
	static void constructEigenstrings(Matrix& u,
			std::shared_ptr<const IsotropicMaterial> material, const MatrixDD& basis);
	
	/** Construct matrix with waves which are outer if given basis is local border */
	static OuterMatrix constructOuterEigenvectors(
			std::shared_ptr<const IsotropicMaterial> material, const MatrixDD& basis);
	
	
	/**
	 * Matrix of linear border condition in local (connected with border) 
	 * basis for the case of fixed pressure on border.
	 * @see BorderCondition
	 */
	static BorderMatrix borderMatrixFixedForce
	(const linal::Vector<DIMENSIONALITY>&) {
		PdeVariables pde = PdeVariables::Zeros();
		pde.pressure() = 1;
		BorderMatrix B_;
		B_.setRow(0, pde);
		return B_;
	}
	
	
	/**
	 * Matrix of linear border condition in local (connected with border) 
	 * basis for the case of fixed normal velocity on border.
	 * @see BorderCondition
	 */
	static BorderMatrix borderMatrixFixedVelocity
	(const linal::Vector<DIMENSIONALITY>& borderNormal) {
		PdeVariables pde = PdeVariables::Zeros();
		pde.setVelocity(borderNormal);
		BorderMatrix B_;
		B_.setRow(0, pde);
		return B_;
	}
	
	
	/**
	 * Just set the values in the node to satisfy the given border condition 
	 * in local basis. I.e, for velocity, we firstly convert the velocity to
	 * local basis, then set the normal component to given value, then convert
	 * it back to global basis. Used when gcm-correction is degenerate
	 */
	inline static void applyPlainBorderCorrection(
			PdeVariables& u,
			const BorderConditions::T type,
			const RealD& normal, const BorderVector& value) {
		switch (type) {
		case BorderConditions::T::FIXED_FORCE: {
			u.pressure() = value(0);
			break;
		}
		case BorderConditions::T::FIXED_VELOCITY: {
			RealD velocityGlobal = u.getVelocity();
			MatrixDD S = linal::createLocalBasisTranspose(normal);
			RealD velocityLocal = S * velocityGlobal;
			velocityLocal(DIMENSIONALITY - 1) = value(0);
			velocityGlobal = linal::transposeMultiply(S, velocityLocal);
			u.setVelocity(velocityGlobal);
			break;
		}
		default:
			THROW_UNSUPPORTED("Unknown border condition type");
		}
	}
	
	
	/**
	 * Just set some values in the nodes to average 
	 * in order to satisfy the given contact condition in local basis.
	 * I.e, we firstly convert the PDE variables to
	 * local basis, then set the its components to satisfy contact conditions,
	 * then convert it back to global basis.
	 * Used when gcm-correction is degenerate and both nodes has outer invariants
	 */
	inline static void applyPlainContactCorrectionAsAverage(
			PdeVariables& uA, PdeVariables& uB,
			const ContactConditions::T type, const RealD& normal) {
		if (type != ContactConditions::T::SLIDE) {
			THROW_UNSUPPORTED("Invalid contact condition for acoustic model");
		}
		
		const real pressure = (uA.pressure() + uB.pressure()) / 2;
		uA.pressure() = pressure;
		uB.pressure() = pressure;
		
		RealD velocityGlobalA = uA.getVelocity();
		RealD velocityGlobalB = uB.getVelocity();
		MatrixDD S = linal::createLocalBasisTranspose(normal);
		RealD velocityLocalA = S * velocityGlobalA;
		RealD velocityLocalB = S * velocityGlobalB;
		real normalVelocityLocal = (
				velocityLocalA(DIMENSIONALITY - 1) +
				velocityLocalB(DIMENSIONALITY - 1)) / 2;
		velocityLocalA(DIMENSIONALITY - 1) = normalVelocityLocal;
		velocityLocalB(DIMENSIONALITY - 1) = normalVelocityLocal;
		velocityGlobalA = linal::transposeMultiply(S, velocityLocalA);
		velocityGlobalB = linal::transposeMultiply(S, velocityLocalB);
		uA.setVelocity(velocityGlobalA);
		uB.setVelocity(velocityGlobalB);
	}
	
	
	/**
	 * Just set some values in the node A equal to those one of the node B
	 * in order to satisfy the given contact condition in local basis.
	 * I.e, we firstly convert the PDE variables to
	 * local basis, then set the its components to satisfy contact conditions,
	 * then convert it back to global basis.
	 * Used when gcm-correction is degenerate and only node A has outer invariants
	 */
	inline static void applyPlainContactCorrection(
			PdeVariables& uA, const PdeVariables& uB,
			const ContactConditions::T type, const RealD& normal) {
		if (type != ContactConditions::T::SLIDE) {
			THROW_UNSUPPORTED("Invalid contact condition for acoustic model");
		}
		
		uA.pressure() = uB.pressure();
		RealD velocityGlobalA = uA.getVelocity();
		RealD velocityGlobalB = uB.getVelocity();
		MatrixDD S = linal::createLocalBasisTranspose(normal);
		RealD velocityLocalA = S * velocityGlobalA;
		RealD velocityLocalB = S * velocityGlobalB;
		velocityLocalA(DIMENSIONALITY - 1) = velocityLocalB(DIMENSIONALITY - 1);
		velocityGlobalA = linal::transposeMultiply(S, velocityLocalA);
		uA.setVelocity(velocityGlobalA);
	}
};


template<int Dimensionality>
inline void AcousticModel<Dimensionality>:: 
constructGcmMatrix(
		GcmMatrix& m, std::shared_ptr<const IsotropicMaterial> material,
		const MatrixDD& basis, const real l) {
	
	const real rho = material->rho;
	const real lambda = material->lambda;
	const real c1 = sqrt(lambda / rho);
	
	const RealD n = basis.getColumn(DIMENSIONALITY - 1);
	
	/// fill matrix A along direction n with scale l
	PdeVariables vec;
	linal::clear(m.A);
	
	vec.pressure() = 0;
	vec.setVelocity(l * lambda * n);
	m.A.setRow(DIMENSIONALITY, vec);
	vec.setVelocity(l / rho * n);
	m.A.setColumn(DIMENSIONALITY, vec);
	
	/// fill L with eigenvalues
	linal::clear(m.L);
	m.L(0) =  l*c1;
	m.L(1) = -l*c1;
	
	constructEigenvectors(m.U1, material, basis);
	constructEigenstrings(m.U, material, basis);
	
	m.checkDecomposition(100*EQUALITY_TOLERANCE);
}


template<int Dimensionality>
inline void AcousticModel<Dimensionality>::
constructEigenvectors(
		Matrix& u1,
		std::shared_ptr<const IsotropicMaterial> material, const MatrixDD& basis) {
	/// fill u1 with eigenvectors
	
	const real rho = material->rho;
	const real lambda = material->lambda;
	const real c1 = sqrt(lambda / rho);
	
	RealD n[DIMENSIONALITY];
	for (int i = 0; i < DIMENSIONALITY; i++) {
		n[i] = basis.getColumn((i + DIMENSIONALITY - 1) % DIMENSIONALITY);
	}
	
	
	/// p-waves
	PdeVariables vec;
	vec.setVelocity(n[0]);
	vec.pressure() = c1 * rho;
	u1.setColumn(0, vec);
	vec.pressure() = -vec.pressure();
	u1.setColumn(1, vec);
	
	/// zero eigenvalues
	vec.pressure() = 0;
	for (int i = 1; i < DIMENSIONALITY; i++) {
		vec.setVelocity(n[i]);
		u1.setColumn(i + 1, vec);
	}
	
}


template<int Dimensionality>
inline void AcousticModel<Dimensionality>::
constructEigenstrings(
		Matrix& u,
		std::shared_ptr<const IsotropicMaterial> material, const MatrixDD& basis) {
	/// fill u with eigenstrings
	
	const real rho = material->rho;
	const real lambda = material->lambda;
	const real c1 = sqrt(lambda / rho);
	
	RealD n[DIMENSIONALITY];
	for (int i = 0; i < DIMENSIONALITY; i++) {
		n[i] = basis.getColumn((i + DIMENSIONALITY - 1) % DIMENSIONALITY);
	}
	
	const real alpha = 0.5; ///< normalizer for U1*U == I
	
	
	/// p-waves
	PdeVariables vec;
	vec.setVelocity(alpha * n[0]);
	vec.pressure() = alpha / (c1 * rho);
	u.setRow(0, vec);
	vec.pressure() = -vec.pressure();
	u.setRow(1, vec);
	
	/// zero eigenvalues
	vec.pressure() = 0;
	for (int i = 1; i < DIMENSIONALITY; i++) {
		vec.setVelocity(n[i]);
		u.setRow(i + 1, vec);
	}
	
}


template<int Dimensionality>
inline typename AcousticModel<Dimensionality>::OuterMatrix
AcousticModel<Dimensionality>::
constructOuterEigenvectors(std::shared_ptr<const IsotropicMaterial> material,
		const MatrixDD& basis) {
	
	Matrix u1;
	constructEigenvectors(u1, material, basis);
	
	// FIXME - do smth with MaterialsWavesMap
	OuterMatrix ans;
	ans.setColumn(0, u1.getColumn(1));
	return ans;
}


}

#endif // LIBGCM_ACOUSTICMODEL_HPP
