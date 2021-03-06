#ifndef LIBGCM_SIMPLEX_BORDERCORRECTOR_HPP
#define LIBGCM_SIMPLEX_BORDERCORRECTOR_HPP

#include <list>

#include <libgcm/engine/simplex/AbstractMesh.hpp>
#include <libgcm/engine/simplex/common.hpp>
#include <libgcm/util/task/BorderCondition.hpp>
#include <libgcm/rheology/models/ElasticModel.hpp>
#include <libgcm/rheology/models/AcousticModel.hpp>
#include <libgcm/rheology/materials/IsotropicMaterial.hpp>


namespace gcm {
namespace simplex {

/**
 * @defgroup Border correctors
 * Classes for applying "outer-waves"-correction on border nodes
 * in order to satisfy some border condition.
 * @see BorderCondition
 */

template<typename TGrid>
class AbstractBorderCorrector {
public:
	typedef typename TGrid::Iterator    Iterator;
	typedef typename TGrid::RealD       RealD;
	typedef typename TGrid::MatrixDD    MatrixDD;
	
	struct NodeBorder {
		/// iterator of the node in grid
		Iterator iterator;
		/// border normal (the direction is outside the grid)
		RealD normal;
	};
	
	/**
	 * Apply border correction for all nodes from the list
	 * along the border normal direction.
	 * It's supposed that gcm-matrices in border nodes are written
	 * in local basis and the first direction calculation (stage 0)
	 * performed along border normal direction.
	 * Thus, this correction must be called after first stage only,
	 * because other directions are degenerate a priori.
	 */
	virtual void applyInLocalBasis(
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real timeAtNextLayer) const = 0;
	
	/**
	 * Apply border correction for all nodes from the list
	 * along the direction of the given stage.
	 * It's supposed that gcm-matrices in border nodes are written
	 * in global basis as in inner nodes.
	 * Thus, this correction must be called after all stages.
	 * @param nextPdeLayerIndex is equal to stage if 
	 * splitting by directions is performed by summ, but if
	 * splitting by directions is done by product, it equals to 0 on all stages
	 */
	virtual void applyInGlobalBasis(
			const int nextPdeLayerIndex,
			const int stage,
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real timeAtNextLayer) const = 0;
	
	/**
	 * Just set the values in nodes to those ones they should have 
	 * from boundary conditions. Used in order to establish 
	 * compatibility of initial and boundary conditions.
	 */
	virtual void applyPlainCorrection(
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real currentTime) const = 0;
};



template<typename Model, typename Material, typename TGrid,
         typename BorderMatrixCreator>
class BorderCorrectorInPdeVectors : public AbstractBorderCorrector<TGrid> {
public:
	typedef DefaultMesh<Model, TGrid, Material> Mesh;
	typedef typename Mesh::PdeVector            PdeVector;
	typedef typename Mesh::PdeVariables         PdeVariables;
	typedef typename Mesh::WaveIndices          WaveIndices;
	static const int DIMENSIONALITY = Mesh::DIMENSIONALITY;
	static const int OUTER_NUMBER = Model::OUTER_NUMBER;
	
	typedef AbstractBorderCorrector<TGrid>      Base;
	typedef typename Base::NodeBorder           NodeBorder;
	typedef typename Base::RealD                RealD;
	typedef typename Base::MatrixDD             MatrixDD;
	
	BorderCorrectorInPdeVectors(const Task::BorderCondition& bc) :
			borderCondition(bc) { }
	
	virtual void applyInLocalBasis(
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real timeAtNextLayer) const override {
		const int stage = 0; ///< the only valid stage number
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(grid);
		assert_true(mesh);
		const auto b = borderCondition.b(timeAtNextLayer);
		
		for (const NodeBorder& nodeBorder: borderNodes) {
			const auto Omega = getColumnsFromGcmMatrices<Model>(
					stage, Model::RIGHT_INVARIANTS, mesh->matrices(nodeBorder.iterator));
			const auto B = BorderMatrixCreator::create(nodeBorder.normal);
			
			PdeVector& u = mesh->_pdeNew(stage, nodeBorder.iterator);
			const auto correction = calculateOuterWaveCorrection(u, Omega, B, b, 0);
			assert_true(correction.isSuccessful);
			u += correction.value;
		}
	}
	
	virtual void applyInGlobalBasis(
			const int nextPdeLayerIndex,
			const int stage,
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real timeAtNextLayer) const override {
		if (borderNodes.empty()) { return; }
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(grid);
		assert_true(mesh);
		
		const auto b = borderCondition.b(timeAtNextLayer);
		static constexpr real EPS = 1e-3; //EQUALITY_TOLERANCE;
		const real minValidDeterminantFabs = EPS * getMaximalPossibleDeterminant(
				*mesh, borderNodes.front(), stage);
		
		for (const NodeBorder& nodeBorder: borderNodes) {
			const WaveIndices outers = mesh->waveIndices(nodeBorder.iterator);
			const auto B = BorderMatrixCreator::create(nodeBorder.normal);
			PdeVariables& u = mesh->_pdeVarsNew(nextPdeLayerIndex, nodeBorder.iterator);
			
			if (outers == Model::RIGHT_INVARIANTS || outers == Model::LEFT_INVARIANTS) {
			/// Normal case for border corrector
				const auto Omega = getColumnsFromGcmMatrices<Model>(
						stage, outers, mesh->matrices(nodeBorder.iterator));
				const auto correction = calculateOuterWaveCorrection(
						u, Omega, B, b, minValidDeterminantFabs);
				if (correction.isSuccessful) {
					u += correction.value;
				} else {
					Model::applyPlainBorderCorrection(u,
							borderCondition.type, nodeBorder.normal, b);
				}
				
			} else {
			/// Double-outer case or fully inner case:
			/// apply correction as average from both sides
				const auto OmegaR = getColumnsFromGcmMatrices<Model>(
						stage, Model::RIGHT_INVARIANTS, mesh->matrices(nodeBorder.iterator));
				const auto OmegaL = getColumnsFromGcmMatrices<Model>(
						stage, Model::LEFT_INVARIANTS, mesh->matrices(nodeBorder.iterator));
				const auto correctionR = calculateOuterWaveCorrection(
						u, OmegaR, B, b, minValidDeterminantFabs);
				const auto correctionL = calculateOuterWaveCorrection(
						u, OmegaL, B, b, minValidDeterminantFabs);
				if (correctionR.isSuccessful && correctionL.isSuccessful) {
					u += (correctionR.value + correctionL.value) / 2;
				} else {
					Model::applyPlainBorderCorrection(u,
							borderCondition.type, nodeBorder.normal, b);
				}
			}
		}
	}
	
	virtual void applyPlainCorrection(
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real currentTime) const override {
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(grid);
		assert_true(mesh);
		const auto b = borderCondition.b(currentTime);
		for (const NodeBorder& nodeBorder: borderNodes) {
			PdeVariables& u = mesh->_pdeVars(nodeBorder.iterator);
			Model::applyPlainBorderCorrection(u,
					borderCondition.type, nodeBorder.normal, b);
		}
	}
	
	
private:
	const BorderCondition<Model> borderCondition;
	
	real getMaximalPossibleDeterminant(
			const Mesh& mesh, const NodeBorder& node, const int s) const {
		auto matrix = mesh.matrices(node.iterator);
		const auto Omega = getColumnsFromGcmMatrices<Model>(
				s, Model::RIGHT_INVARIANTS, matrix);
		/// maximal determinant appears when calculation direction is equal to normal
		const auto B = BorderMatrixCreator::create(matrix->basis.getColumn(s));
		const auto b = borderCondition.b(0);
		PdeVector tmp;
		auto correction = calculateOuterWaveCorrection(tmp, Omega, B, b, 0);
		assert_true(correction.isSuccessful);
		assert_gt(correction.determinantFabs, 0);
		return correction.determinantFabs;
	}
};



template<typename Model, typename Material, typename TGrid,
         typename BorderMatrixCreator>
class BorderCorrectorInRiemannInvariants: public AbstractBorderCorrector<TGrid> {
public:
	typedef DefaultMesh<Model, TGrid, Material> Mesh;
	typedef typename Mesh::PdeVector            PdeVector;
	typedef typename Mesh::GCM_MATRICES         GcmMatrices;
	static const int DIMENSIONALITY = Mesh::DIMENSIONALITY;
	static const int OUTER_NUMBER = Model::OUTER_NUMBER;
	
	typedef AbstractBorderCorrector<TGrid>      Base;
	typedef typename Base::NodeBorder           NodeBorder;
	
	BorderCorrectorInRiemannInvariants(const Task::BorderCondition& bc) :
			pdeCorrector(bc) { }
		
	virtual void applyInLocalBasis(
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real timeAtNextLayer) const override {
		const int stage = 0; ///< the only valid stage number
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(grid);
		assert_true(mesh);
		
		convertToPdeVariables(stage, stage, mesh, borderNodes);
		pdeCorrector.applyInLocalBasis(grid, borderNodes, timeAtNextLayer);
		convertToRiemannInvariants(stage, stage, mesh, borderNodes);
	}
	
	virtual void applyInGlobalBasis(
			const int nextPdeLayerIndex,
			const int stage,
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real timeAtNextLayer) const override {
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(grid);
		assert_true(mesh);
		
		convertToPdeVariables(nextPdeLayerIndex, stage, mesh, borderNodes);
		pdeCorrector.applyInGlobalBasis(
				nextPdeLayerIndex, stage, grid, borderNodes, timeAtNextLayer);
		convertToRiemannInvariants(nextPdeLayerIndex, stage, mesh, borderNodes);
	}
	
	virtual void applyPlainCorrection(
			std::shared_ptr<AbstractMesh<TGrid>> grid,
			std::list<NodeBorder> borderNodes,
			const real currentTime) const override {
		/// @note PDE/Riemann convertion is not performed!
		pdeCorrector.applyPlainCorrection(grid, borderNodes, currentTime);
	}
	
	
private:
	const BorderCorrectorInPdeVectors<
			Model, Material, TGrid, BorderMatrixCreator> pdeCorrector;
	
	
	void convertToPdeVariables(const int nextPdeLayerIndex, const int stage,
			std::shared_ptr<Mesh>& mesh, std::list<NodeBorder> borderNodes) const {
		for (const NodeBorder& nodeBorder: borderNodes) {
			PdeVector& u = mesh->_pdeNew(nextPdeLayerIndex, nodeBorder.iterator);
			const GcmMatrices& gcmMatrices = *(mesh->matrices(nodeBorder.iterator));
			u = gcmMatrices(stage).U1 * u;
		}
	}
	
	void convertToRiemannInvariants(const int nextPdeLayerIndex, const int stage,
			std::shared_ptr<Mesh>& mesh, std::list<NodeBorder> borderNodes) const {
		for (const NodeBorder& nodeBorder: borderNodes) {
			PdeVector& u = mesh->_pdeNew(nextPdeLayerIndex, nodeBorder.iterator);
			const GcmMatrices& gcmMatrices = *(mesh->matrices(nodeBorder.iterator));
			u = gcmMatrices(stage).U * u;
		}
	}
};



template<typename TModel>
struct FixedForceBorderMatrixCreator {
	typedef typename TModel::RealD        RealD;
	typedef typename TModel::BorderMatrix BorderMatrix;
	static BorderMatrix create(const RealD& normal) {
		return TModel::borderMatrixFixedForce(normal);
	}
};
template<typename TModel>
struct FixedVelocityBorderMatrixCreator {
	typedef typename TModel::RealD        RealD;
	typedef typename TModel::BorderMatrix BorderMatrix;
	static BorderMatrix create(const RealD& normal) {
		return TModel::borderMatrixFixedVelocity(normal);
	}
};



template<typename TGrid>
class BorderCorrectorFactory {
public:
	static const int DIMENSIONALITY = TGrid::DIMENSIONALITY;
	typedef ElasticModel<DIMENSIONALITY>     ElasticModelD;
	typedef AcousticModel<DIMENSIONALITY>    AcousticModelD;
	
	static std::shared_ptr<AbstractBorderCorrector<TGrid>> create(
			const GcmType gcmType,
			const Task::BorderCondition& condition,
			const Models::T model, const Materials::T material) {
		
		if (material != Materials::T::ISOTROPIC) {
			THROW_UNSUPPORTED("Unsupported material");
		}
		
		// TODO - this is a horrible shit:
		switch (gcmType) {
		
		case GcmType::ADVECT_RIEMANN_INVARIANTS:
		switch (condition.type) {
			case BorderConditions::T::FIXED_FORCE:
				switch (model) {
					case Models::T::ELASTIC:
						return std::make_shared<BorderCorrectorInRiemannInvariants<
								ElasticModelD, IsotropicMaterial, TGrid,
								FixedForceBorderMatrixCreator<ElasticModelD>>>(
										condition);
					case Models::T::ACOUSTIC:
						return std::make_shared<BorderCorrectorInRiemannInvariants<
								AcousticModelD, IsotropicMaterial, TGrid,
								FixedForceBorderMatrixCreator<AcousticModelD>>>(
										condition);
					default:
						THROW_INVALID_ARG("Unknown type of model");
				}
			case BorderConditions::T::FIXED_VELOCITY:
				switch (model) {
					case Models::T::ELASTIC:
						return std::make_shared<BorderCorrectorInRiemannInvariants<
								ElasticModelD, IsotropicMaterial, TGrid,
								FixedVelocityBorderMatrixCreator<ElasticModelD>>>(
										condition);
					case Models::T::ACOUSTIC:
						return std::make_shared<BorderCorrectorInRiemannInvariants<
								AcousticModelD, IsotropicMaterial, TGrid,
								FixedVelocityBorderMatrixCreator<AcousticModelD>>>(
										condition);
					default:
						THROW_INVALID_ARG("Unknown type of model");
				}
			default:
				THROW_INVALID_ARG("Unknown type of border condition");
		}
		
		case GcmType::ADVECT_PDE_VECTORS:
		switch (condition.type) {
			case BorderConditions::T::FIXED_FORCE:
				switch (model) {
					case Models::T::ELASTIC:
						return std::make_shared<BorderCorrectorInPdeVectors<
								ElasticModelD, IsotropicMaterial, TGrid,
								FixedForceBorderMatrixCreator<ElasticModelD>>>(
										condition);
					case Models::T::ACOUSTIC:
						return std::make_shared<BorderCorrectorInPdeVectors<
								AcousticModelD, IsotropicMaterial, TGrid,
								FixedForceBorderMatrixCreator<AcousticModelD>>>(
										condition);
					default:
						THROW_INVALID_ARG("Unknown type of model");
				}
			case BorderConditions::T::FIXED_VELOCITY:
				switch (model) {
					case Models::T::ELASTIC:
						return std::make_shared<BorderCorrectorInPdeVectors<
								ElasticModelD, IsotropicMaterial, TGrid,
								FixedVelocityBorderMatrixCreator<ElasticModelD>>>(
										condition);
					case Models::T::ACOUSTIC:
						return std::make_shared<BorderCorrectorInPdeVectors<
								AcousticModelD, IsotropicMaterial, TGrid,
								FixedVelocityBorderMatrixCreator<AcousticModelD>>>(
										condition);
					default:
						THROW_INVALID_ARG("Unknown type of model");
				}
			default:
				THROW_INVALID_ARG("Unknown type of border condition");
		}
		
		default:
		THROW_UNSUPPORTED("Unknown type of gcm-method");
		
		}
	}
};


} // namespace simplex
} // namespace gcm


#endif // LIBGCM_SIMPLEX_BORDERCORRECTOR_HPP
