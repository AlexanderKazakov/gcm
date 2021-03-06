#ifndef LIBGCM_SIMPLEXGRID_HPP
#define LIBGCM_SIMPLEXGRID_HPP

#include <numeric>
#include <list>

#include <libgcm/util/infrastructure/infrastructure.hpp>
#include <libgcm/grid/simplex/UnstructuredGrid.hpp>
#include <libgcm/grid/simplex/VertexInfoAndCellInfo.hpp>


namespace gcm {

namespace simplex {
template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
class Engine;
}

/**
 * Unstructured grid on simplices (aka tetrahedrons in 3D and triangles in 2D).
 * We name simplices as "cells" and their faces as "faces".
 * I.e in 2D, a triangle is cell and a segment is face,
 * and in 3D, a tetrahedron is cell and a triangle is face.
 * @tparam Dimensionality space dimensionality
 * @tparam TriangulationT class provides triangulation features, templated
 * by Dimensionality, VertexInfo, CellInfo
 */
template<int Dimensionality,
         template<int, typename, typename> class TriangulationT>
class SimplexGrid : public UnstructuredGrid {
public:
	/// Index of a global triangulation vertex in the grid
	typedef Iterator::Index LocalVertexIndex;
	
	/// Space dimensionality
	static const int DIMENSIONALITY = Dimensionality;
	/// Number of vertices in cell
	static const int CELL_POINTS_NUMBER = DIMENSIONALITY + 1;
	/// Number of vertices in face
	static const int FACE_POINTS_NUMBER = DIMENSIONALITY;
	
	typedef linal::Matrix<DIMENSIONALITY, DIMENSIONALITY> MatrixDD;
	
	
	typedef CellInfoT<CELL_POINTS_NUMBER> CellInfo;
	/// Indicator that no grid owns the cell (auxiliary empty cell)
	static const GridId EmptySpaceFlag = CellInfo::EmptySpaceFlag;
	
	/// Type of the global triangulation structure
	typedef TriangulationT<Dimensionality, VertexInfo, CellInfo> Triangulation;
	
	
	/// Some sort of pointer to triangulation cell
	typedef typename Triangulation::CellHandle             CellHandle;
	/// Some sort of pointer to triangulation vertex
	typedef typename Triangulation::VertexHandle           VertexHandle;
	
	
	/// Point (Vector) in the space of triangulation
	typedef typename Triangulation::RealD         RealD;
	
	
	/// An *estimation* of maximal possible number of vertices connected 
	/// with some inner vertex in the grid (it can be more in a very rare cases)
	static const int MAX_NUMBER_OF_NEIGHBOR_VERTICES = 
			Triangulation::MAX_NUMBER_OF_NEIGHBOR_VERTICES;
	
	
	/// Triangulation cell as set of its vertices iterators
	typedef elements::Element<Iterator, CELL_POINTS_NUMBER> Cell;
	
	
	/// @name Iterators 
	///@{
	
	/** Iteration over all nodes */
	///@{
	typedef Iterator ForwardIterator;
	ForwardIterator begin() const { return 0; }
	ForwardIterator end()   const { return sizeOfRealNodes(); }
	
	typedef Iterator VtkIterator;
	VtkIterator vtkBegin() const { return begin(); }
	VtkIterator vtkEnd()   const { return end();   }
	///@}
	
	/** Iteration over all contact nodes */
	///@{
	typedef typename std::vector<LocalVertexIndex>::const_iterator ContactIterator;
	ContactIterator contactBegin() const { return contactIndices.begin(); }
	ContactIterator contactEnd()   const { return contactIndices.end();   }
	///@}
	
	/** Iteration over all border nodes */
	///@{
	typedef typename std::vector<LocalVertexIndex>::const_iterator BorderIterator;
	BorderIterator borderBegin() const { return borderIndices.begin(); }
	BorderIterator borderEnd()   const { return borderIndices.end();   }
	///@}
	
	/** Iteration over all inner nodes */
	///@{
	typedef typename std::vector<LocalVertexIndex>::const_iterator InnerIterator;
	InnerIterator innerBegin() const { return innerIndices.begin(); }
	InnerIterator innerEnd()   const { return innerIndices.end();   }
	///@}
	
	/** Iteration over all cells belongs to the grid */
	///@{
	typedef typename std::vector<CellHandle>::const_iterator CellIterator;
	CellIterator cellBegin() const { return cellHandles.begin(); }
	CellIterator cellEnd()   const { return cellHandles.end();   }
	///@}
	
	///@}
	
	
	/** Struct for grid constructor */
	struct ConstructionPack {
		Triangulation* triangulation;
	};
	
	SimplexGrid(const GridId id_, const ConstructionPack& constructionPack);
	virtual ~SimplexGrid() { }
	
	
	/** Size of nodes related directly to this grid */
	size_t sizeOfRealNodes() const { return vertexHandles.size(); }
	
	/** Size of real nodes plus auxiliary fixture nodes */
	size_t sizeOfAllNodes() const { return sizeOfRealNodes(); }
	
	
	/** 
	 * Normal to the contact surface between this and neighbor grids.
	 * Direction of normal is OUTside this grid.
	 * If grid with neighborId have no contact with this one 
	 * in that point, RealD::Zeros() is returned.
	 */
	RealD contactNormal(const Iterator& it, const GridId neighborId) const {
		return normal(it, [=](const CellHandle outerCell) {
				return outerCell->info().getGridId() == neighborId;});
	}
	
	/** Normal to the contact surface in specified contact node */
	RealD contactNormal(const Iterator& it) const {
		return contactNormal(it, contactGridId(it));
	}
	
	/** Normal to the free border surface of this grid */
	RealD borderNormal(const Iterator& it) const {
		return contactNormal(it, EmptySpaceFlag);
	}
	
	/** Normal to all connected neighbors and free border of this grid */
	RealD commonNormal(const Iterator& it) const {
		return normal(it, [=](const CellHandle outerCell) {
				return outerCell->info().getGridId() != id;});
	}
	
	/** Id of the grid the node is in contact with */
	GridId contactGridId(const Iterator& it) const {
		std::set<GridId> incidentGrids = gridsAroundVertex(it);
		assert_true(incidentGrids.erase(id));
		assert_eq(incidentGrids.size(), 1);
		const GridId ans = *incidentGrids.begin();
		assert_ne(ans, EmptySpaceFlag);
		return ans;
	}
	
	
	bool isInner(const Iterator& it) const {
		return borderState(it) == BorderState::INNER;
	}
	bool isBorder(const Iterator& it) const {
		return borderState(it) == BorderState::BORDER;
	}
	
	
	/**
	 * Find cell contains point on specified distance (shift)
	 * from specified point (it) performing line walk from it to (it + shift).
	 * 
	 * Possible answer cases:
	 * 1. In some bad degenerate cases cell.n == 0 and points are not set. FIXME
	 * 2. If the line goes out of the grid immediately (from border in outer 
	 * direction), cell.n == 0 and points are not set.
	 * 3. If the line goes inside the grid, but go outside before reaching
	 * the target point through some subcell (face or edge or even point),
	 * cell.n == number of vertices in such subcell and only cell.n points are set.
	 * 4. If the line goes through the grid and reach the target point,
	 * returned cell.n == CELL_POINTS_NUMBER and all CELL_POINTS_NUMBER points are set.
	 * 
	 * @note for convex grids and answer case 4
	 * result is the same with locateOwnerCell
	 */
	Cell findCellCrossedByTheRay(const Iterator& it, const RealD& shift) const;
	
	
	/**
	 * Locate cell contains point on specified distance (shift)
	 * from specified vertex (it) by triangulation->locate function.
	 * It uses different algorithm than findCellCrossedByTheRay.
	 * @see findCellCrossedByTheRay
	 */
	Cell locateOwnerCell(const Iterator& it, const RealD& shift) const {
		CellHandle ch = triangulation->locateOwnerCell(vertexHandle(it), shift);
		if (belongsToTheGrid(ch)) { return createCell(ch); }
		else                      { return createCell(); }
	}
	
	
	/** Average height among all simplices */
	real getAverageHeight() const {
		// TODO - recalculate for movable meshes
		// FIXME - solve the problem with degenerate cells in 3D
		assert_gt(averageSpatialStep, 0);
		return averageSpatialStep;
	}
	
	/** Minimal height among all simplices (can be zero!) */
	real getMinimalHeight() const {
		return minimalSpatialStep;
	}
	
	
	/** Read-only access to points coordinates in DIMENSIONALITY space */
	RealD coordsD(const Iterator& it) const {
		return triangulation->coordsD(vertexHandle(it));
	}
	
	
	/** Read-only access to points coordinates in 3D space */
	Real3 coords(const Iterator& it) const {
		Real3 ans = Real3::Zeros();
		for (int i = 0; i < DIMENSIONALITY; i++) {
			ans(i) = coordsD(it)(i);
		}
		return ans;
	}
	
	
	/** Find node with specified coordinates */
	Iterator findVertexByCoordinates(const RealD& coordinates) const;
	
	
	/** Returns all nodes from this grid connected with given node */
	std::set<Iterator> findNeighborVertices(const Iterator& it) const {
		const auto cells = localIncidentCells(it);
		std::set<Iterator> ans;
		for (const auto cell : cells) {
			for (int i = 0; i < CELL_POINTS_NUMBER; i++) {
				ans.insert(iterator(cell, i));
			}
		}
		ans.erase(it);
		return ans;
	}
	
	
	/** Create Cell with all vertices from CellHandle */
	Cell createCell(const CellHandle ch) const {
		Cell ans;
		ans.n = CELL_POINTS_NUMBER;
		for (int i = 0; i < CELL_POINTS_NUMBER; i++) {
			ans(i) = iterator(ch, i);
		}
		return ans;
	}
	
	
	/**
	 * All minimal heights of all cells of the grid
	 */
	std::vector<real> allMinimalHeights() const;
	
	/**
	 * All minimal heights of all cells of the grid,
	 * which are incident to a border vertex of the grid.
	 * Note that the distance between an inner vertex and a border facet
	 * can be less than the minimal of heights, perpendicular to a border facet.
	 * But it cannot be less than the minimal height of all cells,
	 * incident to a border vertex, i.e, in returned vector.
	 * TODO - or check not heights, but lengths of cells along calc directions?
	 */
	std::vector<real> allMinimalBorderHeights() const;
	
	
	/** Debugging helper */
	void printCell(const Cell& c) const {
		std::cout << "Cell n == " << c.n << ":" << std::endl;
		for (int i = 0; i < c.n; i++) {
			std::cout << "iter: " << getIndex(c(i)) << " coords: " << coordsD(c(i));
		}
	}
	
	
	/** Debugging helper */
	void writeCellsToVtk(const std::vector<CellHandle>& cells,
			const std::string filename = "cells") const;
	
	
private:
	/// Data
	///@{
	
	/// Pointer to global triangulation structure contains several grids
	Triangulation * const triangulation;
	
	/// Pointers to triangulation vertices this grid owns.
	/// LocalVertexIndex is the index of VertexHandle in this vector.
	/// One vertex can be shared between several grids
	std::vector<VertexHandle> vertexHandles;
	
	std::vector<LocalVertexIndex> contactIndices; ///< indices of contact vertices in vertexHandles
	std::vector<LocalVertexIndex> borderIndices;  ///< indices of border vertices in vertexHandles
	std::vector<LocalVertexIndex> innerIndices;   ///< indices of inner vertices in vertexHandles
	
	/// Pointers to global triangulation cells this grid owns.
	/// Each cell is marked by id of the grid the cell belongs to.
	/// A cell in triangulation can belong to the only one grid (unlike vertices)
	std::vector<CellHandle> cellHandles;
	
	/// Minimal among all cells heights of this grid
	real minimalSpatialStep = 0;
	/// Average among all cells minimal heights of this grid
	real averageSpatialStep = 0;
	
	///@}
	
	friend class simplex::Engine<Dimensionality, TriangulationT>;
	USE_AND_INIT_LOGGER("gcm.SimplexGrid")
	
	/** Iterator of vertex at indexInCell position in ch */
	static LocalVertexIndex iterator(
			const CellHandle ch, const int indexInCell) {
		return ch->info().localVertexIndices[indexInCell]; 
	}
	
	
	/** Returns local index of the given vertex */
	LocalVertexIndex localVertexIndex(const VertexHandle vh) const {
		std::list<CellHandle> incidentCells = triangulation->allIncidentCells(vh);
		for (const CellHandle ch : incidentCells) {
			if (belongsToTheGrid(ch)) { return localVertexIndex(vh, ch); }
		}
		THROW_UNSUPPORTED("Given vertex does not belong to this grid");
	}
	
	/** Returns local index of the given vertex using info from given cell */
	static LocalVertexIndex localVertexIndex(
			const VertexHandle vh, const CellHandle ch) {
		return iterator(ch, ch->index(vh));
	}
	
	
	/** Opposite to localVertexIndex */
	VertexHandle vertexHandle(const LocalVertexIndex index) const {
		return vertexHandles[index];
	}
	
	
	/** Is given cell belong to this grid */
	bool belongsToTheGrid(const CellHandle ch) const {
		return ch->info().getGridId() == id;
	}
	
	
	/** Create Cell without vertices */
	Cell createCell() const {
		Cell ans;
		ans.n = 0;
		return ans;
	}
	
	
	/** Fill in innerIndices, borderIndices, contactIndices */
	void markInnersAndBorders();
	
	
	enum class BorderState {
		MULTICONTACT,
		CONTACT,
		BORDER,
		INNER
	};
	BorderState borderState(const LocalVertexIndex it) const {
		std::set<GridId> incidentGrids = gridsAroundVertex(it);
		assert_true(incidentGrids.erase(id));
		if (incidentGrids.empty()) { return BorderState::INNER; }
		if (incidentGrids.size() > 1) { return BorderState::MULTICONTACT; }
		if (*incidentGrids.begin() == EmptySpaceFlag) { return BorderState::BORDER; }
		return BorderState::CONTACT;
	}
	
	
	/** Incident cells which belong to this grid */
	std::list<CellHandle> localIncidentCells(const LocalVertexIndex it) const {
		VertexHandle vh = vertexHandle(it);
		std::list<CellHandle> ans = triangulation->allIncidentCells(vh);
		auto listIter = ans.begin();
		while (listIter != ans.end()) {
			if (belongsToTheGrid(*listIter)) {
				++listIter;
			} else {
				listIter = ans.erase(listIter);
			}
		}
		return ans;
	}
	
	
	/** All different gridIds from all cells incident to the vertex */
	std::set<GridId> gridsAroundVertex(const Iterator it) const {
		VertexHandle vh = vertexHandle(it);
		std::list<CellHandle> cells = triangulation->allIncidentCells(vh);
		std::set<GridId> ans;
		for (const auto cell : cells) {
			ans.insert(cell->info().getGridId());
		}
		return ans;
	}
	
	
	template<typename Predicate>
	RealD normal(const Iterator& it, const Predicate isOuterCellToUse) const {
		std::list<RealD> facesNormals;
		const std::list<CellHandle> localCells = localIncidentCells(it);
		for (const CellHandle localCell : localCells) {
			for (int i = 0; i < CELL_POINTS_NUMBER; i++) {
				CellHandle outerCell = localCell->neighbor(i);
				if(isOuterCellToUse(outerCell) &&
					Utils::has(Triangulation::commonVertices(
							localCell, outerCell), vertexHandle(it))) {
					facesNormals.push_back(
							Triangulation::contactNormal(localCell, outerCell));
				}
			}
		}
		if (facesNormals.empty()) { return RealD::Zeros(); };
		return linal::normalize(std::accumulate(
				facesNormals.begin(), facesNormals.end(), RealD::Zeros()));
	}
	
	
	/** Helper for findCellCrossedByTheRay function to reduce code duplication */
	inline Cell checkLineWalkFoundCell(const Iterator& it,
			const std::vector<CellHandle>& cellsAlong,
			const RealD& start, const RealD& query) const;
	
	
	void collectCellHeightsStatistics();
};


}

#endif // LIBGCM_SIMPLEXGRID_HPP
