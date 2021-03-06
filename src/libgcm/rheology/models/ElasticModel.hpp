#ifndef LIBGCM_ELASTICMODEL_HPP
#define LIBGCM_ELASTICMODEL_HPP

#include <libgcm/rheology/models/Model.hpp>


namespace gcm {


template<int Dimensionality>
class ElasticModel {
public:
	static const Models::T Type = Models::T::ELASTIC;
	static const int DIMENSIONALITY = Dimensionality;
	
	typedef VelocitySigmaVariables<DIMENSIONALITY> PdeVariables;
	typedef typename PdeVariables::PdeVector       PdeVector;
	
	typedef linal::Vector<DIMENSIONALITY>          RealD;
	typedef linal::Matrix<DIMENSIONALITY,
	                      DIMENSIONALITY>          MatrixDD;
	typedef linal::SymmetricMatrix<DIMENSIONALITY> SigmaD;
	
	
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
	static const int OUTER_NUMBER = DIMENSIONALITY;
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
	 * Construct gcm matrices for calculation in global orthonormal basis
	 */
	static void constructGcmMatrices(GcmMatricesPtr m,
			std::shared_ptr<const OrthotropicMaterial> material,
			const MatrixDD& basis = MatrixDD::Identity());
	
	
	/**
	 * Construct gcm matrix for stage along direction given by the last column of the basis.
	 * I.e, velocity in p-waves are along the last column of the basis and
	 * velocity in s-waves are along the two first columns of the basis.
	 * The basis and l represents those ones from Chelnokov PhD thesis page 21.
	 * Isotropic materials only.
	 * @see Chelnokov PhD thesis (note: there are some mistakes there)
	 * @param m gcm matrix to fill in
	 * @param material isotropic material
	 * @param basis local orthonormal basis; last column - direction of waves propagation
	 * @param l scale of variables change along the direction of waves propagation
	 */
	static void constructGcmMatrix(
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
	 * basis for the case of fixed force on border.
	 * @see BorderCondition
	 * @param p border normal
	 */
	static BorderMatrix borderMatrixFixedForce
	(const linal::Vector<DIMENSIONALITY>& p) {
		BorderMatrix B_;
		auto S = linal::createLocalBasis(p);
		/// T * p = S * f, f - fixed force in local basis
		/// S^T * (T * p) = f
		/// S_{ik} * T_{ij} * p_{j} = f_{k}
		/// G_{k}_{ij} * T_{ij} = f_{k}
		
		for (int k = 0; k < DIMENSIONALITY; k++) {
			SigmaD G = SigmaD::Zeros();	
			for (int i = 0; i < DIMENSIONALITY; i++) {
				for (int j = 0; j < DIMENSIONALITY; j++) {
					G(i, j) += S(i, k) * p(j);
				}
			}
			PdeVariables pde = PdeVariables::Zeros();
			pde.setSigma(G);
			B_.setRow(k, pde);
		}
		return B_;
	}
	
	
	/**
	 * Matrix of linear border condition in local (connected with border) 
	 * basis for the case of fixed velocity on border.
	 * @see BorderCondition
	 */	
	static BorderMatrix borderMatrixFixedVelocity
	(const linal::Vector<DIMENSIONALITY>& borderNormal) {
		BorderMatrix B_;
		auto S = linal::createLocalBasis(borderNormal);
		/// v = S * V, V - fixed velocity in local basis
		/// S^T * v = V		
		
		for (int i = 0; i < DIMENSIONALITY; i++) {
			PdeVariables pde = PdeVariables::Zeros();
			pde.setVelocity(S.getColumn(i));
			B_.setRow(i, pde);
			
		}
		return B_;
	}
	
	
	/**
	 * Matrix of linear border condition in global basis
	 * for the case of fixed force on border.
	 * @see BorderCondition
	 */
	static BorderMatrix borderMatrixFixedForceGlobalBasis
	(const linal::Vector<DIMENSIONALITY>& borderNormal) {
	/// T * p = f, f - fixed force in global basis
		BorderMatrix B_;
		for (int i = 0; i < DIMENSIONALITY; i++) {
			PdeVariables pde = PdeVariables::Zeros();
			for (int j = 0; j < DIMENSIONALITY; j++) {
				pde.sigma(i, j) = borderNormal(j);
			}
			B_.setRow(i, pde);
			
		}
		return B_;
	}
	
	
	/**
	 * Matrix of linear border condition in global basis
	 * for the case of fixed velocity on border.
	 * @see BorderCondition
	 */
	static BorderMatrix borderMatrixFixedVelocityGlobalBasis
	(const linal::Vector<DIMENSIONALITY>&) {
		
		BorderMatrix B_;
		for (int i = 0; i < DIMENSIONALITY; i++) {
			PdeVariables pde = PdeVariables::Zeros();
			pde.velocity(i) = 1;
			B_.setRow(i, pde);
		}
		return B_;
	}
	
	
	/**
	 * Just set the values in the node to satisfy the given border condition 
	 * in local basis. I.e, for sigma, we firstly convert the tensor to local
	 * basis, then set all values in local basis to given values, then convert
	 * tensor back to global basis. Used when gcm-correction is degenerate
	 */
	inline static void applyPlainBorderCorrection(
			PdeVariables& u,
			const BorderConditions::T type,
			const RealD& normal, const BorderVector& value) {
		switch (type) {
		case BorderConditions::T::FIXED_FORCE: {
			MatrixDD sigmaGlobal = getSigmaFrom(u);
			MatrixDD S = linal::createLocalBasis(normal);
			MatrixDD S_T = linal::transpose(S);
			MatrixDD sigmaLocal = S_T * sigmaGlobal * S;
			sigmaLocal.setColumn(DIMENSIONALITY - 1, value);
			sigmaLocal.setRow(DIMENSIONALITY - 1, value);
			sigmaGlobal = S * sigmaLocal * S_T;
			setSigmaTo(u, sigmaGlobal);
			break;
		}
		case BorderConditions::T::FIXED_VELOCITY: {
			RealD velocityLocal = value;
			MatrixDD S = linal::createLocalBasis(normal);
			RealD velocityGlobal = S * velocityLocal;
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
		if (type != ContactConditions::T::ADHESION) {
			THROW_UNSUPPORTED("Unsupported contact condition");
		}
		
		RealD velocity = (uA.getVelocity() + uB.getVelocity()) / 2;
		uA.setVelocity(velocity);
		uB.setVelocity(velocity);
		
		MatrixDD sigmaGlobalA = getSigmaFrom(uA);
		MatrixDD sigmaGlobalB = getSigmaFrom(uB);
		MatrixDD S = linal::createLocalBasis(normal);
		MatrixDD S_T = linal::transpose(S);
		MatrixDD sigmaLocalA = S_T * sigmaGlobalA * S;
		MatrixDD sigmaLocalB = S_T * sigmaGlobalB * S;
		RealD sigmaNormal = (
				sigmaLocalA.getColumn(DIMENSIONALITY - 1) +
				sigmaLocalB.getColumn(DIMENSIONALITY - 1)) / 2;
		sigmaLocalA.setColumn(DIMENSIONALITY - 1, sigmaNormal);
		sigmaLocalA.setRow(DIMENSIONALITY - 1, sigmaNormal);
		sigmaLocalB.setColumn(DIMENSIONALITY - 1, sigmaNormal);
		sigmaLocalB.setRow(DIMENSIONALITY - 1, sigmaNormal);
		sigmaGlobalA = S * sigmaLocalA * S_T;
		sigmaGlobalB = S * sigmaLocalB * S_T;
		
		setSigmaTo(uA, sigmaGlobalA);
		setSigmaTo(uB, sigmaGlobalB);
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
		if (type != ContactConditions::T::ADHESION) {
			THROW_UNSUPPORTED("Unsupported contact condition");
		}
		
		uA.setVelocity(uB.getVelocity());
		MatrixDD sigmaGlobalA = getSigmaFrom(uA);
		MatrixDD sigmaGlobalB = getSigmaFrom(uB);
		MatrixDD S = linal::createLocalBasis(normal);
		MatrixDD S_T = linal::transpose(S);
		MatrixDD sigmaLocalA = S_T * sigmaGlobalA * S;
		MatrixDD sigmaLocalB = S_T * sigmaGlobalB * S;
		RealD sigmaNormal = sigmaLocalB.getColumn(DIMENSIONALITY - 1);
		sigmaLocalA.setColumn(DIMENSIONALITY - 1, sigmaNormal);
		sigmaLocalA.setRow(DIMENSIONALITY - 1, sigmaNormal);
		sigmaGlobalA = S * sigmaLocalA * S_T;
		setSigmaTo(uA, sigmaGlobalA);
	}
	
	
private:
	static MatrixDD getSigmaFrom(const PdeVariables& u) {
		MatrixDD sigma = MatrixDD::Zeros();
		for (int i = 0; i < DIMENSIONALITY; i++) {
			for (int j = 0; j < DIMENSIONALITY; j++) {
				sigma(i, j) = u.sigma(i, j);
			}
		}
		return sigma;
	}
	
	static void setSigmaTo(PdeVariables& u, const MatrixDD& sigma) {
		for (int i = 0; i < DIMENSIONALITY; i++) {
			for (int j = 0; j < DIMENSIONALITY; j++) {
				u.sigma(i, j) = sigma(i, j);
			}
		}
	}
	
	
	static SigmaD correctFromTensorToVector(const SigmaD& s) {
	/// Because formulas for tension (sigma) are usually written 
	/// as for symmetric DxD tensor, however in program
	/// we have sigma as vector of length D*(D-1).
	/// It does matter for dot products.
		return (2 * s - linal::Diag(s));
	}
	
	static void constructRotated(GcmMatricesPtr m,
			std::shared_ptr<const OrthotropicMaterial> material);
	
	static void constructNotRotated(GcmMatricesPtr m, const real rho,
			const real c11, const real c12, const real c13,
			const real c22, const real c23, const real c33,
			const real c44, const real c55, const real c66);
	
	static void constructNotRotated(
			GcmMatricesPtr m, const real rho,
			const real c11, const real c12, const real c22, const real c66);
	
	static linal::VECTOR<3, long double> constructEigenvaluesPolynomial(
			ConstGcmMatricesPtr m, const int s);
	
	static std::vector<linal::VECTOR<9, long double>> findEigenvectors(
			const long double l, const linal::Matrix<9, 9>& A,
			const int stage, const int numberOfVectorsToSearch);
	
	static std::vector<linal::VECTOR<9, long double>> findEigenstrings(
			const long double l, const linal::Matrix<9, 9>& A,
			const int stage, const int numberOfStringsToSearch);

	
	/// indices of columns which contains (- 1 / rho) in increasing order
	static void getColumnsWithRho(const int stage, int& i, int& j, int& k);

	/// indices of columns with all zeros in increasing order
	static void getZeroColumns(const int stage, int& i, int& j, int& k);

};


template<int Dimensionality>
inline void ElasticModel<Dimensionality>::
constructGcmMatrix(
		GcmMatrix& m, std::shared_ptr<const IsotropicMaterial> material,
		const MatrixDD& basis, const real l) {
	
	const real rho = material->rho;
	const real lambda = material->lambda;
	const real mu = material->mu;
	
	const real c1 = sqrt((lambda + 2*mu) / rho);
	const real c2 = sqrt(mu / rho);
	
	const RealD n = basis.getColumn(DIMENSIONALITY - 1);
	
	/// fill matrix A along direction n with scale l
	PdeVariables vec;
	linal::clear(m.A);
	/// set DIMENSIONALITY first strings
	for (int i = 0; i < DIMENSIONALITY; i++) {
		linal::clear(vec);
		for (int j = 0; j < DIMENSIONALITY; j++) {
			vec.sigma(i, j) = -l * n(j) / rho;
		}
		m.A.setRow(i, vec);
	}
	/// set DIMENSIONALITY first columns
	for (int i = 0; i < DIMENSIONALITY; i++) {
		linal::clear(vec);
		for (int j = 0; j < DIMENSIONALITY; j++) {
			vec.sigma(i, j)  = -l * mu * n(j);
		}
		for (int j = 0; j < DIMENSIONALITY; j++) {
			vec.sigma(j, j) += -l * (lambda + (i == j) * mu) * n(i);
		}
		m.A.setColumn(i, vec);
	}
	
	/// fill L with eigenvalues
	linal::clear(m.L);
	m.L(0) =  l*c1;
	m.L(1) = -l*c1;
	for (int i = 1; i < DIMENSIONALITY; i++) {
		m.L(2 * i)     =  l*c2;
		m.L(2 * i + 1) = -l*c2;
	}
	
	constructEigenvectors(m.U1, material, basis);
	constructEigenstrings(m.U, material, basis);
	
	m.checkDecomposition(100*EQUALITY_TOLERANCE);
}


template<int Dimensionality>
inline void ElasticModel<Dimensionality>::
constructEigenvectors(
		Matrix& u1,
		std::shared_ptr<const IsotropicMaterial> material, const MatrixDD& basis) {
	/// fill u1 with eigenvectors
	
	const real rho = material->rho;
	const real lambda = material->lambda;
	const real mu = material->mu;
	
	const real c1 = sqrt((lambda + 2*mu) / rho);
	const real c2 = sqrt(mu / rho);
	
	RealD n[DIMENSIONALITY];
	for (int i = 0; i < DIMENSIONALITY; i++) {
		n[i] = basis.getColumn((i + DIMENSIONALITY - 1) % DIMENSIONALITY);
	}
	
	const SigmaD I = SigmaD::Identity();
	
	linal::SYMMETRIC_MATRIX<DIMENSIONALITY, SigmaD> N;
	for (int i = 0; i < DIMENSIONALITY; i++) {
		for (int j = 0; j <= i; j++) {
			N(i, j) = linal::symmDirectProduct(n[i], n[j]);
		}
	}
	
	const real alpha = 0.5; ///< normalizator for U*U1 = I
	
	
	/// p-waves
	PdeVariables vec;
	vec.setVelocity(alpha * n[0]);
	vec.setSigma(-alpha / c1 * (lambda * I + 2 * mu * N(0,0)));
	u1.setColumn(0, vec);
	vec.setSigma(-vec.getSigma());
	u1.setColumn(1, vec);
	
	/// s-waves
	for (int i = 1; i < DIMENSIONALITY; i++) {
		vec.setVelocity(alpha * n[i]);
		vec.setSigma(-2 * alpha * mu / c2 * N(0,i));
		u1.setColumn(2 * i, vec);
		vec.setSigma(-vec.getSigma());
		u1.setColumn(2 * i + 1, vec);
	}
	
	/// zero eigenvalues
	vec.setVelocity(RealD::Zeros());
	switch (DIMENSIONALITY) {
		case 3:
			vec.setSigma(2 * N(1,2));
			u1.setColumn(6, vec);
			vec.setSigma((N(1,1) - N(2,2)) / 2);
			u1.setColumn(7, vec);
			vec.setSigma((N(1,1) + N(2,2)) / 2);
			u1.setColumn(8, vec);
			break;
		case 2:
			vec.setSigma(I - N(0,0));
			u1.setColumn(4, vec);
			break;
		default:
			break;
	}
	
}


template<int Dimensionality>
inline void ElasticModel<Dimensionality>::
constructEigenstrings(
		Matrix& u,
		std::shared_ptr<const IsotropicMaterial> material, const MatrixDD& basis) {
	/// fill u with eigenstrings
	
	const real rho = material->rho;
	const real lambda = material->lambda;
	const real mu = material->mu;
	
	const real c1 = sqrt((lambda + 2*mu) / rho);
	const real c2 = sqrt(mu / rho);
	
	RealD n[DIMENSIONALITY];
	for (int i = 0; i < DIMENSIONALITY; i++) {
		n[i] = basis.getColumn((i + DIMENSIONALITY - 1) % DIMENSIONALITY);
	}
	
	const SigmaD I = SigmaD::Identity();
	
	linal::SYMMETRIC_MATRIX<DIMENSIONALITY, SigmaD> N;
	for (int i = 0; i < DIMENSIONALITY; i++) {
		for (int j = 0; j <= i; j++) {
			N(i, j) = linal::symmDirectProduct(n[i], n[j]);
		}
	}
	
	
	/// p-waves
	PdeVariables vec;
	vec.setVelocity(n[0]);
	vec.setSigma(correctFromTensorToVector(N(0,0) / (-c1 * rho)));
	u.setRow(0, vec);
	vec.setSigma(-vec.getSigma());
	u.setRow(1, vec);
	
	/// s-waves
	for (int i = 1; i < DIMENSIONALITY; i++) {
		vec.setVelocity(n[i]);
		vec.setSigma(correctFromTensorToVector(N(0,i) / (-c2 * rho)));
		u.setRow(2 * i, vec);
		vec.setSigma(-vec.getSigma());
		u.setRow(2 * i + 1, vec);
	}
	
	/// zero eigenvalues
	vec.setVelocity(RealD::Zeros());
	switch (DIMENSIONALITY) {
		case 3:
			vec.setSigma(correctFromTensorToVector(N(1,2)));
			u.setRow(6, vec);
			vec.setSigma(correctFromTensorToVector(N(1,1) - N(2,2)));
			u.setRow(7, vec);
			vec.setSigma(correctFromTensorToVector(
					N(1,1) + N(2,2) - 2 * lambda / (lambda + 2 * mu) * N(0,0)));
			u.setRow(8, vec);
			break;
		case 2:
			vec.setSigma(correctFromTensorToVector(
					N(1,1) - lambda / (lambda + 2 * mu) * N(0,0)));
			u.setRow(4, vec);
			break;
		default:
			break;
	}
	
}


template<int Dimensionality>
inline typename ElasticModel<Dimensionality>::OuterMatrix
ElasticModel<Dimensionality>::
constructOuterEigenvectors(std::shared_ptr<const IsotropicMaterial> material,
		const MatrixDD& basis) {
	
	Matrix u1;
	constructEigenvectors(u1, material, basis);
	
	// FIXME - do smth with MaterialsWavesMap
	OuterMatrix ans;
	for (int i = 0; i < DIMENSIONALITY; i++) {
		ans.setColumn(i, u1.getColumn(1 + 2 * i));
	}
	
	return ans;
}


}

#endif // LIBGCM_ELASTICMODEL_HPP
