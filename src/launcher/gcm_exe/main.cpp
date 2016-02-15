#include <chrono>

#include <lib/Engine.hpp>
#include <lib/rheology/models/Model.hpp>
#include <lib/grid/StructuredGrid.hpp>
#include <lib/grid/Cgal2DGrid.hpp>
#include <lib/numeric/solvers/DefaultSolver.hpp>
#include <lib/util/snapshot/VtkStructuredSnapshotter.hpp>
#include <lib/util/snapshot/VtkCgal2DSnapshotter.hpp>


using namespace gcm;

Task parseTask();
Task parseTaskDemo();


int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);
	USE_AND_INIT_LOGGER("gcm.main");

	Engine engine;
	engine.setSolver(new DefaultSolver<StructuredGrid<SuperDuperModel>>());
	engine.setSnapshotter(new VtkStructuredSnapshotter<StructuredGrid<SuperDuperModel>>());
	/*engine.setSolver(new DefaultSolver<StructuredGrid<Elastic2DModel>>());
	engine.setSnapshotter(new VtkStructuredSnapshotter<StructuredGrid<Elastic2DModel>>());*/
	/*engine.setSolver(new DefaultSolver<Cgal2DGrid<Elastic2DModel>>());
	engine.setSnapshotter(new VtkCgal2DSnapshotter<Cgal2DGrid<Elastic2DModel>>());*/

	try {
		engine.initialize(parseTaskDemo());

		auto t1 = std::chrono::high_resolution_clock::now();
		engine.run();
		auto t2 = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
		std::cout << "Time of calculation, microseconds = " << duration << std::endl;
	} catch (Exception e) {
		LOG_FATAL(e.what());
	}

	MPI_Finalize();
	return 0;
}


Task parseTask() {
	Task task;

	task.accuracyOrder = 2;

	task.lengthes = {1, 1, 1};
	task.sizes = {11, 11, 1};

	real rho = 4; // default density
	real lambda = 2; // default Lame parameter
	real mu = 1; // default Lame parameter
	task.yieldStrength = 1;
	task.continualDamageParameter = 1;
	task.isotropicMaterial = IsotropicMaterial(rho, lambda, mu, task.yieldStrength, task.continualDamageParameter);
	task.orthotropicMaterial = OrthotropicMaterial(rho, {360, 70, 70, 180, 70, 90, 10, 10, 10},
	                                               task.yieldStrength, task.continualDamageParameter);

	task.CourantNumber = 0.9; // number from Courant–Friedrichs–Lewy condition

	task.enableSnapshotting = true;
	task.numberOfSnaps = 11;
	task.stepsPerSnap = 1;

	Task::InitialCondition::Quantity pressure;
	pressure.physicalQuantity = PhysicalQuantities::T::PRESSURE;
	pressure.value = 10.0;
	pressure.area = std::make_shared<SphereArea>(0.2, linal::Vector3({1, 1, 0}));
	task.initialCondition.quantities.push_back(pressure);

	/*Task::InitialCondition::Wave wave;
	wave.waveType = Waves::T::P_FORWARD;
	wave.direction = 0;
	wave.quantity = PhysicalQuantities::T::PRESSURE;
	wave.quantityValue = 10.0;
	wave.area = std::make_shared<AxisAlignedBoxArea>(linal::Vector3({0.2, -1, -1}), linal::Vector3({0.5, 3, 3}));
	task.initialCondition.waves.push_back(wave);*/

//	task.borderConditions.at(CUBIC_BORDERS::X_LEFT) = BorderCondition::T::FREE_BORDER;

	task.quantitiesToWrite = {PhysicalQuantities::T::PRESSURE};

	return task;
}

Task parseTaskDemo() {
	Task task;

	task.accuracyOrder = 2;

	task.lengthes = {4, 2, 1};
	task.sizes = {100, 50, 25};

	real rho = 4; // default density
	real lambda = 2; // default Lame parameter
	real mu = 1; // default Lame parameter
	task.yieldStrength = 1;
	task.continualDamageParameter = 1;
	task.isotropicMaterial = IsotropicMaterial(rho, lambda, mu, task.yieldStrength, task.continualDamageParameter);
	task.orthotropicMaterial = OrthotropicMaterial(rho, {360, 70, 70, 180, 70, 90, 10, 10, 10},
	                                               task.yieldStrength, task.continualDamageParameter);

	task.CourantNumber = 0.9; // number from Courant–Friedrichs–Lewy condition

	task.enableSnapshotting = true;
	task.numberOfSnaps = 10;
	task.stepsPerSnap = 1;

	Task::InitialCondition::Quantity pressure;
	pressure.physicalQuantity = PhysicalQuantities::T::PRESSURE;
	pressure.value = 10.0;
	pressure.area = std::make_shared<SphereArea>(0.2, linal::Vector3({2, 1, 0.5}));
	task.initialCondition.quantities.push_back(pressure);

	/*Task::InitialCondition::Wave wave;
	wave.waveType = Waves::T::P_FORWARD;
	wave.direction = 0;
	wave.quantity = PhysicalQuantities::T::PRESSURE;
	wave.quantityValue = 10.0;
	wave.area = std::make_shared<AxisAlignedBoxArea>(linal::Vector3({0.2, -1, -1}), linal::Vector3({0.5, 3, 3}));
	task.initialCondition.waves.push_back(wave);*/

	task.borderConditions.at(CUBIC_BORDERS::X_LEFT) = BorderCondition::T::FREE_BORDER;

	task.quantitiesToWrite = {PhysicalQuantities::T::PRESSURE};

	return task;
}
