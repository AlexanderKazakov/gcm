#include <lib/mesh/grid/Cgal3DGrid.hpp>
#include <lib/mesh/grid/Cgal3DLineWalker.hpp>

#include </home/alex/work/gcm/src/libcgal3dmesher/Cgal3DMesher.hpp> // FIXME

using namespace gcm;


Cgal3DGrid::
Cgal3DGrid(const Task& task) :
	UnstructuredGrid(task),
	effectiveSpatialStep(task.cgal3DGrid.spatialStep), 
	movable(task.cgal3DGrid.movable) {

	LOG_DEBUG("Call Cgal3DMesher");
	cgal_3d_mesher::Cgal3DMesher::triangulate(
			task.cgal3DGrid.spatialStep, task.cgal3DGrid.detectSharpEdges,
			task.cgal3DGrid.polyhedronFileName, triangulation);
	LOG_DEBUG("Number of vertices after meshing: " << triangulation.number_of_vertices());
	LOG_DEBUG("Number of cells after meshing: " << triangulation.number_of_cells());
	
	vertexHandles.resize(triangulation.number_of_vertices());
	size_t vertexIndex = 0;
	for (auto it  = triangulation.finite_vertices_begin();
	          it != triangulation.finite_vertices_end(); it++) {
		it->info() = vertexIndex;
		vertexHandles[vertexIndex] = it;
		vertexIndex++;
	}

	markInnersAndBorders();
}


Real3 Cgal3DGrid::
normal(const Iterator& it) const {
	assert_true(isBorder(it));
	VertexHandle v = vertexHandle(it);
	
	std::vector<CellHandle> incidentCells;
	triangulation.incident_cells(v, std::back_inserter(incidentCells));
	
	std::vector<Real3> facesNormals;
	for (const auto innerCell : incidentCells) {
		if ( !isInDomain(innerCell)) {
		// over all incident inner cells
			continue;
		}
		
		for (int i = 0; i < 4; i++) {
			CellHandle outerCell = innerCell->neighbor(i);
			if ( isInDomain(outerCell) ) {
			// over all cell's outer neighbors ..
				continue;
			}
			
			std::vector<VertexHandle> innerVertexOfInnerCell;
			std::vector<VertexHandle> borderVerticesOfInnerCell = commonVertices(
					innerCell, outerCell, &innerVertexOfInnerCell);
			if ( !Utils::has(borderVerticesOfInnerCell, v) ) {
			// .. which also have our vertex
				continue;
			}
			
			// add normal of the border face of the inner cell
			facesNormals.push_back(linal::oppositeFaceNormal(
					real3(innerVertexOfInnerCell[0]->point()),
					real3(borderVerticesOfInnerCell[0]->point()),
					real3(borderVerticesOfInnerCell[1]->point()),
					real3(borderVerticesOfInnerCell[2]->point())));
		}
	}
	
	assert_false(facesNormals.empty()); // it is possible when grid is too coarse
	return linal::normalize(std::accumulate(
			facesNormals.begin(), facesNormals.end(), Real3::Zeros()));
}


std::set<Cgal3DGrid::Iterator> Cgal3DGrid::
findNeighborVertices(const Iterator& it) const {	
	std::vector<CellHandle> incidentCells;
	triangulation.finite_incident_cells(
			vertexHandle(it), std::back_inserter(incidentCells));
	
	std::set<Iterator> ans;
	for (const auto cell : incidentCells) {
		if (isInDomain(cell)) {
			for (int i = 0; i < 4; i++) {
				ans.insert(getIterator(cell->vertex(i)));
			}
		}
	}
	ans.erase(it);
	return ans;
}


Cgal3DGrid::Cell Cgal3DGrid::
findOwnerCell(const Iterator& it, const Real3& shift) const {
	Cgal3DLineWalker lineWalker(this, it, shift);
	Real3 query = coords(it) + shift;
	
	CellHandle currentCell = lineWalker.cell();
	CellHandle previousCell = currentCell;
	while ( ( currentCell != NULL ) && isInDomain(currentCell) && 
	          !contains(currentCell, query) ) {
		previousCell = currentCell;
		currentCell = lineWalker.next();
	}
	
	return createCell(currentCell, previousCell);
}


Cgal3DGrid::Cell Cgal3DGrid::
locateOwnerCell(const Iterator& it, const Real3& shift) const {
	VertexHandle beginVertex = vertexHandle(it);
	CgalPoint3 query = beginVertex->point() + cgalVector3(shift);
	CellHandle c = triangulation.locate(query, beginVertex->cell());
	return createCell(c, c);
}


std::set<Cgal3DGrid::Iterator> Cgal3DGrid::
findBorderNeighbors(const Iterator& it) const {
	std::set<Iterator> ans = findNeighborVertices(it);
    for (auto neighbor  = ans.begin(); neighbor != ans.end(); ) {
		if ( !isBorder(*neighbor) ) {
			neighbor = ans.erase(neighbor);
		} else {
			++neighbor;
		}
	}
	return ans;
}


Cgal3DGrid::Iterator Cgal3DGrid::
findVertexByCoordinates(const Real3& coordinates) const {
	for (const auto& it : *this) {
		if (coords(it) == coordinates) {
			return it;
		}
	}
	THROW_INVALID_ARG("There isn't a vertex with such coordinates");
}


void Cgal3DGrid::markInnersAndBorders() {
/// insert indices of border vertices into borderIndices
/// and indices of inner vertices into innerIndices
	borderIndices.clear();
	innerIndices.clear();
	
	for (const auto it : *this) {
		if (isBorder(it)) {
			borderIndices.push_back(getIndex(it));
		} else {
			innerIndices.push_back(getIndex(it));
		}
	}
	
	assert_eq(borderIndices.size() + innerIndices.size(), sizeOfAllNodes());
	LOG_DEBUG("Number of border vertices: " << borderIndices.size());
	LOG_DEBUG("Number of inner vertices: " << innerIndices.size());
}


std::vector<Cgal3DGrid::VertexHandle> Cgal3DGrid::
commonVertices(const CellHandle& a, const CellHandle& b, 
		std::vector<VertexHandle>* aHasOnly) const {
/// return common vertices of given cells
/// fill in aHasOnly with vertices which only a has

	std::vector<VertexHandle> common;
	for (int i = 0; i < 4; i++) {
		VertexHandle v = a->vertex(i);
		if (b->has_vertex(v)) {
			common.push_back(v);
		} else {
			if (aHasOnly != nullptr) {
				aHasOnly->push_back(v);
			}
		}
	}
	
	return common;
}


void Cgal3DGrid::
printCell(const CellHandle& f, const std::string& name) const {
	/// debugging helper
	SUPPRESS_WUNUSED(name);
	LOG_DEBUG("Cell " << name << ":");
	for (int i = 0; i < 4; i++) {
		if (triangulation.is_infinite(f->vertex(i))) {
			LOG_DEBUG("\nINFINITE\n");
		} else {
			LOG_DEBUG(real3(f->vertex(i)->point()));
		}
	}
}








