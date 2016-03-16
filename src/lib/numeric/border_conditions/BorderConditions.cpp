#include <lib/numeric/border_conditions/BorderConditions.hpp>
#include <lib/rheology/models/Model.hpp>
#include <lib/numeric/gcm/GridCharacteristicMethod.hpp>

using namespace gcm;

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::initialize(const Task& task) {
	sizes = task.sizes;
	startR = task.startR;
	lengths = task.lengthes;
}

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::beforeStatement(const Statement &statement) {
	for (const auto& bc : statement.borderConditions) {
		for (const auto& q : bc.values) {
			assert_eq(PdeVariables::QUANTITIES.count(q.first), 1);
		}
		conditions.push_back(Condition(bc.area, bc.values));
	}
	for (const auto& fr : statement.fractures) {
		for (const auto& q : fr.values) {
			assert_eq(PdeVariables::QUANTITIES.count(q.first), 1);
		}
		int d = fr.direction;
		int index = (int) (sizes(d) * (fr.coordinate - startR(d)) / lengths(d));
		assert_gt(index, 0); assert_lt(index, sizes(d) - 1);
		fractures.push_back(Fracture(d, index,   - 1, fr.area, fr.values));
		fractures.push_back(Fracture(d, index + 1, 1, fr.area, fr.values));
	}
}

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::applyBorderBeforeStage
(Mesh* mesh_, const real currentTime_, const real timeStep_, const int stage) {
	// handling borders
	mesh = mesh_; currentTime = currentTime_; timeStep = timeStep_; direction = stage;
	// special for x-axis (because MPI partition along x-axis)
	if (direction == 0) {
		if (mesh->getRank() == 0) {
			onTheRight = false;
			handleSide();
		}
		if (mesh->getRank() == mesh->getNumberOfWorkers() - 1) {
			onTheRight = true;
			handleSide();
		}
		return;
	}
	// for other axes
	onTheRight = false;
	handleSide();
	onTheRight = true;
	handleSide();
}

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::handleSide() const {
	auto borderIter = mesh->slice(direction, 0);
	if (onTheRight)
		borderIter = mesh->slice(direction, mesh->sizes(direction) - 1);
	while (borderIter != borderIter.end()) {
		for (const auto& condition : conditions) {
			if (condition.area->contains(mesh->coords(borderIter))) {
				handleBorderPoint(borderIter, condition.values);
			}
		}
		++borderIter;
	}
}

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::handleBorderPoint
(const Iterator& borderIter, const Map& values) const {
	
	int innerSign = onTheRight ? -1 : 1;
	for (int a = 1; a <= mesh->borderSize; a++) {
		auto realIter = borderIter; realIter(direction) += innerSign * a;
		auto virtIter = borderIter; virtIter(direction) -= innerSign * a;
	
		mesh->_pde(virtIter) = mesh->pde(realIter);
		for (const auto& q : values) {
			const auto& quantity = q.first;
			const auto& timeDependency = q.second;
			real realValue = PdeVariables::QUANTITIES.at(quantity).Get(mesh->pde(realIter));
			real virtValue = - realValue + 2 * timeDependency(currentTime);
			PdeVariables::QUANTITIES.at(quantity).Set(virtValue, mesh->_pde(virtIter));
		}
	}
}

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::applyBorderAfterStage
(Mesh* mesh_, const real currentTime_, const real timeStep_, const int stage) {
	// handling inner fractures
	mesh = mesh_; currentTime = currentTime_; timeStep = timeStep_; direction = stage;
	for (const auto& fr : fractures) {
		if (fr.direction == stage) {
			allocateHelpMesh();
			auto sliceIter = mesh->slice(direction, fr.index);
			while (sliceIter != sliceIter.end()) {
				if (fr.area->contains(mesh->coords(sliceIter))) {
					handleFracturePoint(sliceIter, fr.values, fr.normal);
				}
				++sliceIter;
			}
			delete helpMesh;
		}
	}
}

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::allocateHelpMesh() {
	Task helpTask;
	helpTask.dimensionality = 1;
	helpTask.borderSize = mesh->borderSize;
	helpTask.forceSequence = true;
	helpTask.lengthes = {1, 1, 1};
	helpTask.sizes = {1, 1, 1};
	helpTask.sizes(direction) = mesh->borderSize;
	helpMesh = new Mesh(helpTask);
	helpMesh->allocate();
}

template<typename TModel, typename TMaterial>
void BorderConditions<TModel, CubicGrid, TMaterial>::handleFracturePoint
(const Iterator& iter, const Map& values, const int fracNormal) {
	// copy values to helpMesh
	for (int i = 0; i < 2 * mesh->borderSize; i++) {
		Iterator helpMeshIter = {0, 0, 0}; helpMeshIter(direction) += i;
		Iterator realMeshIter = iter;      realMeshIter(direction) += i * fracNormal;
		helpMesh->node(helpMeshIter)->copyFrom(mesh->node(realMeshIter));
	}
	// apply border conditions before stage on the helpMesh
	onTheRight = false;
	Mesh* tmpMesh = mesh;
	mesh = helpMesh;
	handleBorderPoint({0, 0, 0}, values);
	mesh = tmpMesh;
	// calculate stage on the helpMesh
	GridCharacteristicMethod<Mesh>::stage(direction, timeStep * fracNormal, helpMesh);
	// copy calculated values to real mesh
	for (int i = 0; i < mesh->borderSize; i++) {
		Iterator helpMeshIter = {0, 0, 0}; helpMeshIter(direction) += i;
		Iterator realMeshIter = iter;      realMeshIter(direction) += i * fracNormal;
		mesh->_pdeNew(realMeshIter) = helpMesh->pdeNew(helpMeshIter);
	}
}


template class BorderConditions<Elastic1DModel, CubicGrid, IsotropicMaterial>;
template class BorderConditions<Elastic2DModel, CubicGrid, IsotropicMaterial>;
template class BorderConditions<Elastic3DModel, CubicGrid, IsotropicMaterial>;
template class BorderConditions<SuperDuperModel, CubicGrid, IsotropicMaterial>;
template class BorderConditions<Elastic3DModel, CubicGrid, OrthotropicMaterial>;
template class BorderConditions<SuperDuperModel, CubicGrid, OrthotropicMaterial>;


