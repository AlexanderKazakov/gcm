#include <gtest/gtest.h>

#include <lib/util/areas/AxisAlignedBoxArea.hpp>

#include <test/wrappers/Wrappers.hpp>

using namespace gcm;

TEST(Engine, run)
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

	EngineWrapper<IdealElastic2DNode> engine;
	engine.initialize(task);
	IdealElastic2DNode::Vector sWave = engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, 3, 0).u;
	engine.run();
	ASSERT_EQ(sWave, engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, 22, 0).u);
}


TEST(Engine, TwoLayersDifferentRho)
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

		EngineWrapper<IdealElastic2DNode> engine;
		engine.initialize(task);

		real rho2rho0 = rho2rho0Initial * pow(2, i);
		real lambda2lambda0 = 1;
		real mu2mu0 = 1;
		engine.getSolver()->getMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);
		engine.getSolver()->getNewMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);

		int leftNodeIndex = (int) (task.sizes(1) * 0.25);
		IdealElastic2DNode init = engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);

		engine.run();

		int rightNodeIndex = (int) (task.sizes(1) * 0.7);
		IdealElastic2DNode reflect = engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);
		IdealElastic2DNode transfer = engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, rightNodeIndex, 0);

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


TEST(Engine, TwoLayersDifferentE)
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

		EngineWrapper<IdealElastic2DNode> engine;
		engine.initialize(task);

		real rho2rho0 = 1;
		real lambda2lambda0 = E2E0Initial * pow(2, i);
		real mu2mu0 = E2E0Initial * pow(2, i);
		engine.getSolver()->getMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);
		engine.getSolver()->getNewMesh()->changeRheology(rho2rho0, lambda2lambda0, mu2mu0);

		int leftNodeIndex = (int) (task.sizes(1) * 0.25);
		IdealElastic2DNode init = engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);

		engine.run();

		int rightNodeIndex = (int) (task.sizes(1) * 0.7);
		IdealElastic2DNode reflect = engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, leftNodeIndex, 0);
		IdealElastic2DNode transfer = engine.getSolver()->getMesh()->getNodeForTest(task.sizes(0) / 2, rightNodeIndex, 0);

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