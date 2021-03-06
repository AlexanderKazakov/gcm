#include <libgcm/engine/simplex/Engine.hpp>
#include <libgcm/grid/simplex/cgal/CgalTriangulation.hpp>

#include <limits>

using namespace gcm;
using namespace gcm::simplex;


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
Engine<Dimensionality, TriangulationT>::
Engine(const Task& task) :
		AbstractEngine(task),
		triangulation(task),
		movable(task.simplexGrid.movable),
		borderCalcMode(task.simplexGrid.borderCalcMode),
		gcmType(task.globalSettings.gcmType),
		splittingType(task.globalSettings.splittingType),
		stageVsLayerMap(createStageVsLayerMap(splittingType)) {
	
	initializeCalculationBasis(task);
	createMeshes(task);
	createContacts(task);
	for (auto vertexIter  = triangulation.verticesBegin();
	          vertexIter != triangulation.verticesEnd(); ++vertexIter) {
		addBorderOrContact(vertexIter);
	}
	
	LOG_INFO("Found contacts:");
	for (const auto& contact : contacts) {
		LOG_INFO("For bodies " << contact.first.first << " and "
				<< contact.first.second << " number of contact nodes = "
				<< contact.second.nodesInContact.size());
	}
	LOG_INFO("Found borders (except non-reflection cases):");
	for (const Body& body : bodies) {
		for (size_t i = 0; i < body.borders.size(); i++) {
			LOG_INFO("For body " << body.mesh->id
					<< " and border condition number " << i
					<< " number of border nodes = "
					<< body.borders[i].borderNodes.size());
		}
	}
	applyPlainBorderContactCorrection(Clock::Time());
	
	afterConstruction(task);
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
createMeshes(const Task& task) {
	for (const auto& taskBody : task.bodies) {
		Body body;
		const GridId gridId = taskBody.first;
		auto factory = createAbstractFactory(taskBody.second);
		
		body.mesh = factory->createMesh(
				task, gridId, {&triangulation}, numberOfNextPdeTimeLayers());
		body.mesh->setUpPde(task, calculationBasis.basis, borderCalcMode);
		
		body.gcm = factory->createGcm(gcmType);
		
		for (const Snapshotters::T snapType : task.globalSettings.snapshottersId) {
			body.snapshotters.push_back(
					factory->createSnapshotter(task, snapType));
		}
		
		for (const Odes::T odeType : taskBody.second.odes) {
			body.odes.push_back(factory->createOde(odeType));
		}
		
		for (const Task::BorderCondition& condition : task.borderConditions) {
			Border border;
			border.correctionArea = condition.area;
			border.useForMulticontactNodes = condition.useForMulticontactNodes;
			border.borderCorrector = BorderCorrectorFactory<Grid>::create(
					gcmType,
					condition,
					task.bodies.at(body.mesh->id).modelId,
					task.bodies.at(body.mesh->id).materialId);
			body.borders.push_back(border);
		}
		
		bodies.push_back(body);
	}
	
	assert_eq(task.bodies.size(), bodies.size());
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
nextTimeStep() {
	changeCalculationBasis();
	
	applyPlainBorderContactCorrection(Clock::Time() + Clock::TimeStep());
	for (int stage = 0; stage < Dimensionality; stage++) {
		gcmStage(stage, Clock::Time(), Clock::TimeStep());
	}
	if (splittingType == SplittingType::SUMM) {
		for (const Body& body : bodies) {
			body.mesh->averageNewPdeLayersToCurrent();
		}
	}
	
	for (const Body& body : bodies) {
		for (typename Body::OdePtr ode : body.odes) {
			ode->apply(*body.mesh, Clock::TimeStep());
		}
	}
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
gcmStage(const int stage, const real currentTime, const real timeStep) {
	for (const Body& body : bodies) {
		body.gcm->beforeStage(stageVsLayerMap[(size_t)stage], stage, *body.mesh);
	}
	for (const Body& body : bodies) {
		body.gcm->contactAndBorderStage(
				stageVsLayerMap[(size_t)stage], stage, timeStep, *body.mesh);
	}
	correctContactsAndBorders(stage, currentTime + timeStep);
	for (const Body& body : bodies) {
		body.gcm->innerStage(stageVsLayerMap[(size_t)stage], stage, timeStep, *body.mesh);
	}
	for (const Body& body : bodies) {
		body.gcm->afterStage(stageVsLayerMap[(size_t)stage], stage, *body.mesh);
	}
	if (splittingType == SplittingType::PRODUCT) {
		for (const Body& body : bodies) {
			body.mesh->swapCurrAndNextPdeTimeLayer(0);
		}
	}
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
correctContactsAndBorders(const int stage, const real timeAtNextLayer) {
	switch (borderCalcMode) {
		case BorderCalcMode::GLOBAL_BASIS:
			for (const auto& contact : contacts) {
				contact.second.contactCorrector->applyInGlobalBasis(
						stageVsLayerMap[(size_t)stage],
						stage,
						getBody(contact.first.first).mesh,
						getBody(contact.first.second).mesh,
						contact.second.nodesInContact);
			}
			for (const Body& body : bodies) {
				for (const Border& border : body.borders) {
					border.borderCorrector->applyInGlobalBasis(
							stageVsLayerMap[(size_t)stage],
							stage,
							body.mesh,
							border.borderNodes,
							timeAtNextLayer);
				}
			}
			break;
			
		case BorderCalcMode::LOCAL_BASIS:
			if (stage != 0) { return; }
			for (const auto& contact : contacts) {
				contact.second.contactCorrector->applyInLocalBasis(
						getBody(contact.first.first).mesh,
						getBody(contact.first.second).mesh,
						contact.second.nodesInContact);
			}
			for (const Body& body : bodies) {
				for (const Border& border : body.borders) {
					border.borderCorrector->applyInLocalBasis(
							body.mesh,
							border.borderNodes,
							timeAtNextLayer);
				}
			}
			break;
			
		default:
			THROW_BAD_CONFIG("Unknown border calculation mode");
	}
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
applyPlainBorderContactCorrection(const real timeForBorderCondition) {
/// Just set some values in border and contact nodes to the values they should
/// have according to border and contact conditions
	for (const auto& contact : contacts) {
		contact.second.contactCorrector->applyPlainCorrection(
				getBody(contact.first.first).mesh,
				getBody(contact.first.second).mesh,
				contact.second.nodesInContact);
	}
	for (const Body& body : bodies) {
		for (const Border& border : body.borders) {
			border.borderCorrector->applyPlainCorrection(
					body.mesh,
					border.borderNodes,
					timeForBorderCondition);
		}
	}
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
createContacts(const Task& task) {
	
	std::set<GridId> gridsIds;
	for (const Body& body : bodies) {
		gridsIds.insert(body.mesh->id);
	}
	
	for (const GridsPair gridsPair : Utils::makePairs(gridsIds)) {
		
		ContactConditions::T condition = task.contactCondition.defaultCondition;
		auto conditionIter = task.contactCondition.gridToGridConditions.find(gridsPair);
		if (conditionIter != task.contactCondition.gridToGridConditions.end()) {
			condition = conditionIter->second;
		}
		
		Contact contact;
		contact.contactCorrector = ContactCorrectorFactory<Grid>::create(
				gcmType,
				condition,
				task.bodies.at(gridsPair.first).modelId,
				task.bodies.at(gridsPair.first).materialId,
				task.bodies.at(gridsPair.second).modelId,
				task.bodies.at(gridsPair.second).materialId);
		
		contacts.insert({ gridsPair, contact });
	}
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
addBorderOrContact(const VertexHandle vh) {
	std::set<GridId> incidentGrids = triangulation.incidentGridsIds(vh);
	if (incidentGrids.size() == 1) { return; }
	
	if (incidentGrids.erase((size_t)EmptySpaceFlag)) {
		for (const GridId id : incidentGrids) {
			addBorderNode(vh, id);
		}
	} else {
		if (incidentGrids.size() == 2) {
			addContactNode(vh, {*incidentGrids.begin(), *incidentGrids.rbegin()});
		} else {
			for (const GridId id : incidentGrids) {
				addBorderNode(vh, id);
			}
		}
	}
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
addContactNode(const VertexHandle vh, const GridsPair gridsIds) {
	
	Iterator firstIter = getBody(gridsIds.first).mesh->localVertexIndex(vh);
	Iterator secondIter = getBody(gridsIds.second).mesh->localVertexIndex(vh);
	RealD normal = getBody(gridsIds.first).mesh->contactNormal(firstIter, gridsIds.second);
	if (normal != RealD::Zeros()) {
		contacts.at(gridsIds).nodesInContact.push_back(
				{ firstIter, secondIter, normal });
	}
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
addBorderNode(const VertexHandle vh, const GridId gridId) {
	assert_ne(gridId, EmptySpaceFlag);
	std::shared_ptr<Mesh> mesh = getBody(gridId).mesh;
	Iterator iter = mesh->localVertexIndex(vh);
	const bool isMulticontact = mesh->borderNormal(iter) == RealD::Zeros();
	
	// for a concrete node, not more than one border condition can be applied
	Border* chosenBorder = nullptr;
	for (Border& border : getBody(gridId).borders) {
		if ( border.correctionArea->contains(mesh->coords(iter)) &&
				(!isMulticontact || border.useForMulticontactNodes) ) {
			chosenBorder = &border;
		}
	}
	if (chosenBorder == nullptr) { return; }
	
	RealD normal = mesh->commonNormal(iter);
	assert_true(normal != RealD::Zeros());
	chosenBorder->borderNodes.push_back({ iter, normal });
}


template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
void Engine<Dimensionality, TriangulationT>::
writeSnapshots(const int step) {
	for (Body& body : bodies) {
		for (typename Body::SnapPtr snapshotter : body.snapshotters) {
			snapshotter->snapshot(body.mesh.get(), step);
		}
	}
}



template class Engine<2, CgalTriangulation>;
template class Engine<3, CgalTriangulation>;

