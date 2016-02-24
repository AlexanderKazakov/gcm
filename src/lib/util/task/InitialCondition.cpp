#include <lib/util/task/InitialCondition.hpp>
#include <lib/rheology/models/Model.hpp>


using namespace gcm;

template<class TModel>
void InitialCondition<TModel>::initialize(const Task &task) {

	for (auto& vectorInitCondition : task.initialCondition.vectors) {
		assert_eq(PdeVector::M, vectorInitCondition.list.size());
		conditions.push_back(Condition(vectorInitCondition.area, 
			                 PdeVector(vectorInitCondition.list)));
	}

	for (auto& wave : task.initialCondition.waves) {
		assert_lt(wave.direction, TModel::DIMENSIONALITY);
		typename TModel::Material material;
		material.initialize(task);
		GCM_MATRICES gcmMatrices(material);
		auto A = gcmMatrices.A(wave.direction);
		int columnNumber = GCM_MATRICES::WAVE_COLUMNS.at(wave.waveType);
		PdeVector tmp;
		tmp = A.U1.getColumn(columnNumber);
		real currentValue = PdeVector::QUANTITIES.at(wave.quantity).Get(tmp);
		assert_ne(currentValue, 0.0);
		tmp *= wave.quantityValue / currentValue;
		conditions.push_back(Condition(wave.area, tmp));
	}

	for (auto& quantityInitCondition : task.initialCondition.quantities) {
		PdeVector tmp;
		linal::clear(tmp);
		PdeVector::QUANTITIES.at(quantityInitCondition.physicalQuantity).Set
				(quantityInitCondition.value, tmp);
		conditions.push_back(Condition(quantityInitCondition.area, tmp));
	}
}

template<class TModel>
void InitialCondition<TModel>::apply(PdeVector &v, const linal::Vector3 &coords) const {
	linal::clear(v);
	for (auto& condition : conditions) {
		if (condition.area->contains(coords)) {
			v += condition.pdeVector;
		}
	}
}


template class InitialCondition<Elastic1DModel>;
template class InitialCondition<Elastic2DModel>;
template class InitialCondition<Elastic3DModel>;
template class InitialCondition<OrthotropicElastic3DModel>;
template class InitialCondition<ContinualDamageElastic2DModel>;
template class InitialCondition<IdealPlastic2DModel>;

template class InitialCondition<SuperDuperModel>;
