#ifndef LIBGCM_GRID_HPP
#define LIBGCM_GRID_HPP

#include <vector>
#include <mpi.h>

#include "lib/util/Logging.hpp"
#include "lib/task/Task.hpp"
#include "lib/linal/Linal.hpp"

namespace gcm {
	template<class TModel> class MpiStructuredSolver;

	template <class TModel>
	class Grid {
	public:
		typedef typename TModel::Node Node;
		typedef typename Node::Vector Vector;
		typedef typename Node::Matrix Matrix;

		/** @param task properties and initial conditions etc */
		void initialize(const Task &task);

		int getRank() const { return rank; };
		int getNumberOfWorkers() const { return numberOfWorkers; };
		real getMaximalLambda() const { return maximalLambda; };

		/**
		 * Change rheology in some area
		 *
		 * @param rho2rho0 = (rho in the area) / (default rho)
		 * @param lambda2lambda0 = (lambda in the area) / (default lambda)
		 * @param mu2mu0 = (mu in the area) / (default mu)
		 */
		virtual void changeRheology(const real &rho2rho0, const real &lambda2lambda0, const real &mu2mu0) = 0;

	protected:
		int rank = 0; // index of core
		int numberOfWorkers = 0; // number of cores

		/* Node storage */
		std::vector<Node> nodes;

		/* GcmMatrices that is common for majority of nodes */
		std::shared_ptr<typename TModel::GcmMatrices> defaultMatrix;
		real maximalLambda = 0.0; // maximal eigenvalue among all nodes all GcmMatrices of the mesh

		virtual void initializeImpl(const Task &task) = 0;
		virtual void applyInitialConditions(const Task &task) = 0;
		virtual real getMinimalSpatialStep() const = 0;
		virtual void applyBorderConditions() = 0;
		
		friend class MpiStructuredSolver<TModel>;
	};
}

#endif // LIBGCM_GRID_HPP
