#ifndef LIBGCM_SIMPLEX_CONTACTCORRECTOR_HPP
#define LIBGCM_SIMPLEX_CONTACTCORRECTOR_HPP

#include <list>

#include <libgcm/engine/simplex/DefaultMesh.hpp>
#include <libgcm/engine/simplex/common.hpp>
#include <libgcm/rheology/models/ElasticModel.hpp>
#include <libgcm/rheology/models/AcousticModel.hpp>
#include <libgcm/rheology/materials/IsotropicMaterial.hpp>


namespace gcm {
namespace simplex {

/**
 * @defgroup Contact correctors
 * Classes for applying "outer-waves"-correction on contact nodes
 * in order to satisfy some contact condition.
 */

template<typename TGrid>
class AbstractContactCorrector {
public:
	typedef typename TGrid::Iterator    Iterator;
	typedef typename TGrid::RealD       RealD;
	typedef typename TGrid::MatrixDD    MatrixDD;
	
	struct NodesContact {
		/// pair of nodes in contact (their iterators in grids)
		Iterator first, second;
		/// contact normal (the direction is from first to second)
		RealD normal;
	};
	
	/**
	 * Apply contact correction for all nodes from the list
	 * along the contact normal direction.
	 * It's supposed that gcm-matrices in contact nodes are written
	 * in local basis and the first direction calculation (stage 0)
	 * performed along contact normal direction.
	 * Thus, this correction must be called after first stage only,
	 * because other directions are degenerate a priori.
	 */
	virtual void applyInLocalBasis(
			std::shared_ptr<AbstractMesh<TGrid>> a,
			std::shared_ptr<AbstractMesh<TGrid>> b,
			std::list<NodesContact> nodesInContact) = 0;
	
	/**
	 * Apply contact correction for all nodes from the list
	 * along the direction of the given stage.
	 * It's supposed that gcm-matrices in contact nodes are written
	 * in global basis as in inner nodes.
	 * Thus, this correction must be called after all stages.
	 */
	virtual void applyInGlobalBasis(
			const int stage,
			std::shared_ptr<AbstractMesh<TGrid>> a,
			std::shared_ptr<AbstractMesh<TGrid>> b,
			std::list<NodesContact> nodesInContact) = 0;
	
	
protected:
	/**
	 * General expression of linear contact condition is:
	 *     B1_A * u_A = B1_B * u_B,
	 *     B2_A * u_A = B2_B * u_B,
	 * where u is pde-vector and B is border matrices.
	 * Given with inner-calculated pde vectors, we correct them
	 * with outer waves combination (Omega) in order to satisfy contact condition.
	 * @see BorderCorrector
	 */
	template<typename PdeVector, typename MatrixOmega, typename MatrixB>
	void
	correctNodesContact(
			PdeVector& uA,
			const MatrixOmega& OmegaA, const MatrixB& B1A, const MatrixB& B2A,
			PdeVector& uB,
			const MatrixOmega& OmegaB, const MatrixB& B1B, const MatrixB& B2B) {
		
		const auto R = linal::invert(B1A * OmegaA);
		const auto p = R * (B1B * uB - B1A * uA);
		const auto Q = R * (B1B * OmegaB);
		
		const auto A = (B2B * OmegaB) - ((B2A * OmegaA) * Q);
		const auto f = ((B2A * OmegaA) * p) + (B2A * uA) - (B2B * uB);
		
		const auto alphaB = linal::solveLinearSystem(A, f);
		const auto alphaA = p + Q * alphaB;
		
		uA += OmegaA * alphaA;
		uB += OmegaB * alphaB;
	}
	
	USE_AND_INIT_LOGGER("gcm.simplex.ContactCorrector")
};



template<typename ModelA, typename MaterialA,
         typename ModelB, typename MaterialB,
         typename TGrid,
         typename ContactMatrixCreator>
class ConcreteContactCorrector : public AbstractContactCorrector<TGrid> {
public:
	typedef DefaultMesh<ModelA, TGrid, MaterialA> MeshA;
	typedef DefaultMesh<ModelB, TGrid, MaterialB> MeshB;
	static const int DIMENSIONALITY = MeshA::DIMENSIONALITY;
	
	typedef AbstractContactCorrector<TGrid> Base;
	typedef typename Base::NodesContact     NodesContact;
	typedef typename Base::RealD            RealD;
	typedef typename Base::MatrixDD         MatrixDD;
	
	virtual void applyInLocalBasis(
			std::shared_ptr<AbstractMesh<TGrid>> a,
			std::shared_ptr<AbstractMesh<TGrid>> b,
			std::list<NodesContact> nodesInContact) override {
		
		std::shared_ptr<MeshA> meshA = std::dynamic_pointer_cast<MeshA>(a);
		assert_true(meshA);
		std::shared_ptr<MeshB> meshB = std::dynamic_pointer_cast<MeshB>(b);
		assert_true(meshB);
		
		for (const NodesContact& nodesContact : nodesInContact) {
			const auto OmegaA = getOuterMatrixFromGcmMatricesInLocalBasis<ModelA>(
					meshA->matrices(nodesContact.first));
			const auto OmegaB = getOuterMatrixFromGcmMatricesInLocalBasis<ModelB>(
					meshB->matrices(nodesContact.second));
			const auto B1A = ContactMatrixCreator::createB1A(nodesContact.normal);
			const auto B1B = ContactMatrixCreator::createB1B(nodesContact.normal);
			const auto B2A = ContactMatrixCreator::createB2A(nodesContact.normal);
			const auto B2B = ContactMatrixCreator::createB2B(nodesContact.normal);
			
			auto& uA = meshA->_pdeNew(0, nodesContact.first);
			auto& uB = meshB->_pdeNew(0, nodesContact.second);
			
			this->correctNodesContact(uA, OmegaA, B1A, B2A,
			                          uB, OmegaB, B1B, B2B);
		}
	}
	
	virtual void applyInGlobalBasis(
			const int stage,
			std::shared_ptr<AbstractMesh<TGrid>> a,
			std::shared_ptr<AbstractMesh<TGrid>> b,
			std::list<NodesContact> nodesInContact) override {
		
		std::shared_ptr<MeshA> meshA = std::dynamic_pointer_cast<MeshA>(a);
		assert_true(meshA);
		std::shared_ptr<MeshB> meshB = std::dynamic_pointer_cast<MeshB>(b);
		assert_true(meshB);
		const RealD calcDirection =
				meshA->getInnerCalculationBasis().getColumn(stage);
		assert_true(calcDirection ==
				meshB->getInnerCalculationBasis().getColumn(stage));
		
		for (const NodesContact& nodesContact : nodesInContact) {
			
			const real projection = linal::dotProduct(calcDirection, nodesContact.normal);
//			if (projection == 0) { continue; }
			const RealD directionFromAToB =
					calcDirection * Utils::sign(projection);
			const auto OmegaA = ModelA::constructOuterEigenvectors(
					meshA->material(nodesContact.first),
					linal::createLocalBasis(  directionFromAToB));
			const auto OmegaB = ModelB::constructOuterEigenvectors(
					meshB->material(nodesContact.second),
					linal::createLocalBasis( -directionFromAToB));
			
			const auto B1A = ContactMatrixCreator::createB1A(nodesContact.normal);
			const auto B1B = ContactMatrixCreator::createB1B(nodesContact.normal);
			const auto B2A = ContactMatrixCreator::createB2A(nodesContact.normal);
			const auto B2B = ContactMatrixCreator::createB2B(nodesContact.normal);
			
			auto& uA = meshA->_pdeNew(stage, nodesContact.first);
			auto& uB = meshB->_pdeNew(stage, nodesContact.second);
			
			this->correctNodesContact(uA, OmegaA, B1A, B2A,
			                          uB, OmegaB, B1B, B2B);
		}
	}
};



template<typename ModelA, typename ModelB>
struct AdhesionContactMatrixCreator {
	typedef typename ModelA::RealD        RealD;
	typedef typename ModelA::BorderMatrix BorderMatrix;
	
	static BorderMatrix createB1A(const RealD& normal) {
		return ModelA::borderMatrixFixedVelocityGlobalBasis(normal);
	}
	static BorderMatrix createB1B(const RealD& normal) {
		return ModelB::borderMatrixFixedVelocityGlobalBasis(normal);
	}
	static BorderMatrix createB2A(const RealD& normal) {
		return ModelA::borderMatrixFixedForceGlobalBasis(normal);
	}
	static BorderMatrix createB2B(const RealD& normal) {
		return ModelB::borderMatrixFixedForceGlobalBasis(normal);
	}
};
template<typename ModelA, typename ModelB>
struct SlideContactMatrixCreator {
	typedef typename ModelA::RealD        RealD;
	typedef typename ModelA::BorderMatrix BorderMatrix;
	
	// FIXME - this is valid for acoustic model only
	static BorderMatrix createB1A(const RealD& normal) {
		return ModelA::borderMatrixFixedVelocity(normal);
	}
	static BorderMatrix createB1B(const RealD& normal) {
		return ModelB::borderMatrixFixedVelocity(normal);
	}
	static BorderMatrix createB2A(const RealD& normal) {
		return ModelA::borderMatrixFixedForce(normal);
	}
	static BorderMatrix createB2B(const RealD& normal) {
		return ModelB::borderMatrixFixedForce(normal);
	}
};



template<typename TGrid>
class ContactCorrectorFactory {
public:
	static const int DIMENSIONALITY = TGrid::DIMENSIONALITY;
	typedef ElasticModel<DIMENSIONALITY>     ElasticModelD;
	typedef AcousticModel<DIMENSIONALITY>    AcousticModelD;
	
	
	static std::shared_ptr<AbstractContactCorrector<TGrid>> create(
			const ContactConditions::T condition,
			const Models::T model1, const Materials::T material1,
			const Models::T model2, const Materials::T material2) {
		
		switch (condition) {
			case ContactConditions::T::ADHESION:
				if (model1 == Models::T::ELASTIC &&
				    model2 == Models::T::ELASTIC &&
				    material1 == Materials::T::ISOTROPIC &&
				    material2 == Materials::T::ISOTROPIC) {
					
					return std::make_shared<ConcreteContactCorrector<
							ElasticModelD, IsotropicMaterial,
							ElasticModelD, IsotropicMaterial, TGrid,
							AdhesionContactMatrixCreator<ElasticModelD, ElasticModelD>>>();
					
				} else {
					THROW_UNSUPPORTED("Incompatible or unsupported contact conditions, \
							models and materials combination");
				}
				
			case ContactConditions::T::SLIDE:
				if (model1 == Models::T::ACOUSTIC &&
				    model2 == Models::T::ACOUSTIC &&
				    material1 == Materials::T::ISOTROPIC &&
				    material2 == Materials::T::ISOTROPIC) {
					
					return std::make_shared<ConcreteContactCorrector<
							AcousticModelD, IsotropicMaterial,
							AcousticModelD, IsotropicMaterial, TGrid,
							SlideContactMatrixCreator<AcousticModelD, AcousticModelD>>>();
					
				} else {
					THROW_UNSUPPORTED("Incompatible or unsupported contact conditions, \
							models and materials combination");
				}
				
			default:
				THROW_INVALID_ARG("Unknown type of contact condition");
		}
	}
};


} // namespace simplex
} // namespace gcm


#endif // LIBGCM_SIMPLEX_CONTACTCORRECTOR_HPP
