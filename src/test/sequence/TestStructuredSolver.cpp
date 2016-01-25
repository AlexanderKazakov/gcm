#include <gtest/gtest.h>

#include <lib/numeric/gcmethod/MpiStructuredSolver.hpp>
#include <lib/util/areas/AxisAlignedBoxArea.hpp>
#include <lib/util/areas/StraightBoundedCylinderArea.hpp>

using namespace gcm;

TEST(Solver, StageXForward)
{
	for (int accuracyOrder = 1; accuracyOrder < 5; accuracyOrder++) {
		Task task;
		task.accuracyOrder = accuracyOrder;
		task.CourantNumber = 1.0;
		task.material = IsotropicMaterial(4.0, 2.0, 0.5);
		task.sizes(0) = 10;
		task.sizes(1) = 10;
		task.lengthes = {2, 3, 1};
		task.numberOfSnaps = 1;
		task.T = 100.0;

		Task::InitialCondition::Wave wave;
		wave.waveType = Waves::T::P_FORWARD;
		wave.direction = 0; // along x
		wave.quantity = PhysicalQuantities::T::PRESSURE;
		wave.quantityValue = 5;
		linal::Vector3 min({0.3, -1, -1});
		linal::Vector3 max({0.7, 4, 1});
		wave.area = std::make_shared<AxisAlignedBoxArea>(min, max);
		task.initialCondition.waves.push_back(wave);

		MpiStructuredSolver<IdealElastic2DNode> solver;
		solver.initialize(task);
		IdealElastic2DNode::Vector pWave = solver.getMesh()->getNodeForTest(2, 0, 0).u;
		IdealElastic2DNode::Vector zero({0, 0, 0, 0, 0});

		StructuredGrid<IdealElastic2DNode>* mesh = solver.getMesh();
		StructuredGrid<IdealElastic2DNode>* newMesh = solver.getNewMesh();
		for (int i = 0; i < 7; i++) {
			for (int y = 0; y < task.sizes(1); y++) {
				for (int x = 0; x < task.sizes(0); x++) {
					ASSERT_EQ(mesh->getNodeForTest(x, y, 0).u, (x == 2 + i || x == 3 + i) ? pWave : zero)
					<< "accuracyOrder = " << accuracyOrder << " i = " << i << " y = " << y << " x = " << x;
				}
			}
			solver.stage(0, solver.getTauForTest());
			std::swap(mesh, newMesh); // because solver swap them internally
		}
	}
}


TEST(Solver, StageY)
{
	for (int accuracyOrder = 1; accuracyOrder < 5; accuracyOrder++) {
		Task task;
		task.accuracyOrder = accuracyOrder;
		task.CourantNumber = 1.0;
		task.material = IsotropicMaterial(4.0, 2.0, 0.5);
		task.sizes(0) = 10;
		task.sizes(1) = 10;
		task.lengthes = {3, 2, 1};
		task.numberOfSnaps = 1;
		task.T = 100.0;

		Task::InitialCondition::Wave wave;
		wave.waveType = Waves::T::P_FORWARD;
		wave.direction = 1; // along y
		wave.quantity = PhysicalQuantities::T::Vy;
		wave.quantityValue = -2;
		linal::Vector3 min({ -1, 0.3, -1});
		linal::Vector3 max({ 4, 0.7, 1});
		wave.area = std::make_shared<AxisAlignedBoxArea>(min, max);
		task.initialCondition.waves.push_back(wave);

		MpiStructuredSolver<IdealElastic2DNode> solver;
		solver.initialize(task);
		IdealElastic2DNode::Vector pWave = solver.getMesh()->getNodeForTest(0, 2, 0).u;
		IdealElastic2DNode::Vector zero({0, 0, 0, 0, 0});

		StructuredGrid<IdealElastic2DNode>* mesh = solver.getMesh();
		StructuredGrid<IdealElastic2DNode>* newMesh = solver.getNewMesh();
		for (int i = 0; i < 2; i++) {
			for (int y = 0; y < task.sizes(1); y++) {
				for (int x = 0; x < task.sizes(0); x++) {
					ASSERT_EQ(mesh->getNodeForTest(x, y, 0).u, (y == 2 + i || y == 3 + i) ? pWave : zero)
					<< "accuracyOrder = " << accuracyOrder << " i = " << i << " y = " << y << " x = " << x;
				}
			}
			solver.stage(1, solver.getTauForTest());
			std::swap(mesh, newMesh); // because solver swap them internally
		}
	}
}


TEST(Solver, StageYSxx)
{
	for (int accuracyOrder = 1; accuracyOrder < 5; accuracyOrder++) {
		Task task;
		task.accuracyOrder = accuracyOrder;
		task.CourantNumber = 0.7;
		task.material = IsotropicMaterial(4.0, 2.0, 0.5);
		task.sizes(0) = 20;
		task.sizes(1) = 10;
		task.lengthes = {7, 3, 1};
		task.numberOfSnaps = 1;
		task.T = 100.0;

		Task::InitialCondition::Quantity quantity;
		quantity.physicalQuantity = PhysicalQuantities::T::Sxx;
		quantity.value = 10;
		linal::Vector3 begin({3.684, 1.666, -1});
		linal::Vector3 end({3.684, 1.666, 1});
		quantity.area = std::make_shared<StraightBoundedCylinderArea>(0.1, begin, end);
		task.initialCondition.quantities.push_back(quantity);

		MpiStructuredSolver<IdealElastic2DNode> solver;
		solver.initialize(task);
		IdealElastic2DNode::Vector sxxOnly = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, task.sizes(1) / 2, 0).u;
		IdealElastic2DNode::Vector zero({0, 0, 0, 0, 0});

		StructuredGrid<IdealElastic2DNode>* mesh = solver.getMesh();
		StructuredGrid<IdealElastic2DNode>* newMesh = solver.getNewMesh();
		for (int i = 0; i < 7; i++) {
			for (int y = 0; y < task.sizes(1); y++) {
				for (int x = 0; x < task.sizes(0); x++) {
					ASSERT_EQ(mesh->getNodeForTest(x, y, 0).u,
					          (x == task.sizes(0) / 2 && y == task.sizes(1) / 2 ) ? sxxOnly : zero)
					<< "accuracyOrder = " << accuracyOrder << " i = " << i << " y = " << y << " x = " << x;
				}
			}
			solver.stage(1, solver.getTauForTest());
			std::swap(mesh, newMesh); // because solver swap them internally
		}
	}
}


TEST(Solver, calculate)
{
	Task task;
	task.accuracyOrder = 5;
	task.CourantNumber = 4.5;
	task.material = IsotropicMaterial(4.0, 2.0, 0.5);
	task.sizes(0) = 20;
	task.sizes(1) = 40;
	task.lengthes = {7, 3, 1};
	task.numberOfSnaps = 9;
	task.T = 100.0;

	Task::InitialCondition::Wave wave;
	wave.waveType = Waves::T::S1_FORWARD;
	wave.direction = 1; // along y
	wave.quantity = PhysicalQuantities::T::Vx;
	wave.quantityValue = 1;
	linal::Vector3 min({ -1, 0.1125, -1});
	linal::Vector3 max({ 8, 0.6375, 1});
	wave.area = std::make_shared<AxisAlignedBoxArea>(min, max);
	task.initialCondition.waves.push_back(wave);

	MpiStructuredSolver<IdealElastic2DNode> solver;
	solver.initialize(task);
	IdealElastic2DNode::Vector sWave = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, 3, 0).u;
	solver.calculate();
	ASSERT_EQ(sWave, solver.getMesh()->getNodeForTest(task.sizes(0) / 2, 22, 0).u);
}


TEST(Solver, TwoLayersDifferentRho)
{
	real rho2rho0Initial = 0.25;
	int numberOfSnapsInitial = 30;
	for (int i = 0; i < 5; i++) {

		Task task;
		task.accuracyOrder = 3;
		task.CourantNumber = 1.5;
		task.material = IsotropicMaterial(1.0, 2.0, 0.8);
		task.sizes(0) = 50;
		task.sizes(1) = 100;
		task.lengthes = {2, 1, 1};
		task.numberOfSnaps = numberOfSnapsInitial + 2 * i; // in order to catch the impulses

		Task::InitialCondition::Wave wave;
		wave.waveType = Waves::T::P_FORWARD;
		wave.direction = 1; // along y
		wave.quantity = PhysicalQuantities::T::Vy;
		wave.quantityValue = -2;
		linal::Vector3 min({ -1, 0.015, -1});
		linal::Vector3 max({ 4, 0.455, 1});
		wave.area = std::make_shared<AxisAlignedBoxArea>(min, max);
		task.initialCondition.waves.push_back(wave);

		MpiStructuredSolver<IdealElastic2DNode> solver;
		solver.initialize(task);

		real rho2rho0 = rho2rho0Initial * pow(2, i);
		real lambda2lambda0 = 1;
		real mu2mu0 = 1;
		solver.getMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);
		solver.getNewMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);

		int leftNodeIndex = (int) (task.sizes(1) * 0.25);
		IdealElastic2DNode init = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);

		solver.calculate();

		int rightNodeIndex = (int) (task.sizes(1) * 0.7);
		IdealElastic2DNode reflect = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);
		IdealElastic2DNode transfer = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, rightNodeIndex, 0);

		real rho0 = task.material.rho;
		real lambda0 = task.material.lambda;
		real mu0 = task.material.mu;
		real E0 = mu0 * (3 * lambda0 + 2 * mu0) / (lambda0 + mu0); // Young's modulus
		real Z0 = sqrt(E0 * rho0); // acoustic impedance


		real rho = rho2rho0 * rho0;
		real lambda = lambda2lambda0 * lambda0;
		real mu = mu2mu0 * mu0;
		real E = mu * (3 * lambda + 2 * mu) / (lambda + mu); // Young's modulus
		real Z = sqrt(E * rho); // acoustic impedance

		ASSERT_NEAR(reflect.u.sigma(1, 1) / init.u.sigma(1, 1),
		            (Z - Z0) / (Z + Z0),
		            1e-2);
		ASSERT_NEAR(reflect.u.V[1] / init.u.V[1],
		            (Z0 - Z) / (Z + Z0),
		            1e-2);

		ASSERT_NEAR(transfer.u.sigma(1, 1) / init.u.sigma(1, 1),
		            2 * Z / (Z + Z0),
		            1e-2);
		ASSERT_NEAR(transfer.u.V[1] / init.u.V[1],
		            2 * Z0 / (Z + Z0),
		            1e-2);


	}
}


TEST(Solver, TwoLayersDifferentE)
{
	real E2E0Initial = 0.25;
	int numberOfSnapsInitial = 40;
	for (int i = 0; i < 5; i++) {

		Task task;
		task.accuracyOrder = 3;
		task.CourantNumber = 1.5;
		task.material = IsotropicMaterial(1.0, 2.0, 0.8);
		task.sizes(0) = 50;
		task.sizes(1) = 100;
		task.lengthes = {2, 1, 1};
		task.numberOfSnaps = numberOfSnapsInitial - 2 * i; // in order to catch the impulses

		Task::InitialCondition::Wave wave;
		wave.waveType = Waves::T::P_FORWARD;
		wave.direction = 1; // along y
		wave.quantity = PhysicalQuantities::T::Vy;
		wave.quantityValue = -2;
		linal::Vector3 min({ -1, 0.015, -1});
		linal::Vector3 max({ 4, 0.455, 1});
		wave.area = std::make_shared<AxisAlignedBoxArea>(min, max);
		task.initialCondition.waves.push_back(wave);

		MpiStructuredSolver<IdealElastic2DNode> solver;
		solver.initialize(task);

		real rho2rho0 = 1;
		real lambda2lambda0 = E2E0Initial * pow(2, i);
		real mu2mu0 = E2E0Initial * pow(2, i);
		solver.getMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);
		solver.getNewMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);

		int leftNodeIndex = (int) (task.sizes(1) * 0.25);
		IdealElastic2DNode init = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);

		solver.calculate();

		int rightNodeIndex = (int) (task.sizes(1) * 0.7);
		IdealElastic2DNode reflect = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);
		IdealElastic2DNode transfer = solver.getMesh()->getNodeForTest(task.sizes(0) / 2, rightNodeIndex, 0);

		real rho0 = task.material.rho;
		real lambda0 = task.material.lambda;
		real mu0 = task.material.mu;
		real E0 = mu0 * (3 * lambda0 + 2 * mu0) / (lambda0 + mu0); // Young's modulus
		real Z0 = sqrt(E0 * rho0); // acoustic impedance


		real rho = rho2rho0 * rho0;
		real lambda = lambda2lambda0 * lambda0;
		real mu = mu2mu0 * mu0;
		real E = mu * (3 * lambda + 2 * mu) / (lambda + mu); // Young's modulus
		real Z = sqrt(E * rho); // acoustic impedance

		ASSERT_NEAR(reflect.u.sigma(1, 1) / init.u.sigma(1, 1),
		            (Z - Z0) / (Z + Z0),
		            1e-2);
		ASSERT_NEAR(reflect.u.V[1] / init.u.V[1],
		            (Z0 - Z) / (Z + Z0),
		            1e-2);

		ASSERT_NEAR(transfer.u.sigma(1, 1) / init.u.sigma(1, 1),
		            2 * Z / (Z + Z0),
		            1e-2);
		ASSERT_NEAR(transfer.u.V[1] / init.u.V[1],
		            2 * Z0 / (Z + Z0),
		            1e-2);


	}
}