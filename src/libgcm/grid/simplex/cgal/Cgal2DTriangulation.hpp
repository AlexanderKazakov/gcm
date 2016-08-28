#ifndef LIBGCM_CGAL2DTRIANGULATION_HPP
#define LIBGCM_CGAL2DTRIANGULATION_HPP

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>

#include <libcgalmesher/Cgal2DMesher.hpp>

#include <libgcm/linal/linal.hpp>
#include <libgcm/util/task/Task.hpp>
#include <libgcm/util/infrastructure/infrastructure.hpp>


namespace gcm {

/**
 * 2D triangulation by CGAL library.
 * Triangle named "cell"
 * (we rename CGAL faces as cells in order to unify
 * 2D and 3D triangulations external representation).
 * @tparam VertexInfo type of auxiliary information stored in vertices
 * @tparam CellInfo   type of auxiliary information stored in cells
 */
template<typename VertexInfo, typename CellInfo>
class Cgal2DTriangulation {
public:
	
	typedef CGAL::Exact_predicates_inexact_constructions_kernel        K;
	typedef CGAL::Triangulation_vertex_base_with_info_2<VertexInfo, K> Vb;
	typedef CGAL::Triangulation_face_base_with_info_2<CellInfo, K>     Cb;
	typedef CGAL::Triangulation_data_structure_2<Vb, Cb>               Tds;
	/// The triangulation type
	typedef CGAL::Delaunay_triangulation_2<K, Tds>                     Triangulation;
	
	typedef typename Triangulation::Vertex_handle           VertexHandle;
	typedef typename Triangulation::Face_handle             CellHandle;
	typedef typename Triangulation::Geom_traits::Vector_2   CgalVectorD;
	typedef typename Triangulation::Point                   CgalPointD;
	typedef typename Triangulation::All_faces_iterator      AllCellsIterator;
	typedef typename Triangulation::Line_face_circulator    LineFaceCirculator;
	
	/// Space dimensionality
	static const int DIMENSIONALITY = 2;
	/// Point (Vector) in DIMENSIONALITY space
	typedef linal::Vector<DIMENSIONALITY> RealD;
	/// An *estimation* of maximal possible number of vertices connected 
	/// with some inner vertex in the grid (it can be more in a very rare cases)
	static const int MAX_NUMBER_OF_NEIGHBOR_VERTICES = 8;
	
	
	Cgal2DTriangulation(const Task& task) {
		assert_true(Task::SimplexGrid::Mesher::CGAL_MESHER == task.simplexGrid.mesher);
		
		// convert task to cgal mesher format
		typedef cgalmesher::Cgal2DMesher::TaskBody Body;
		std::vector<Body> bodies;
		for (const auto& b : task.simplexGrid.bodies) {
			bodies.push_back({b.id, b.outer, b.inner});
		}
		
		LOG_DEBUG("Call Cgal2DMesher");
		cgalmesher::Cgal2DMesher::triangulate(
				task.simplexGrid.spatialStep, bodies, triangulation);
		
		LOG_INFO("Number of all vertices after meshing: " << triangulation.number_of_vertices());
		LOG_INFO("Number of all cells after meshing: " << triangulation.number_of_faces());
	}
	
	
	/** All cells iteration begin */
	AllCellsIterator allCellsBegin() const {
		return triangulation.all_faces_begin();
	}
	/** All cells iteration end */
	AllCellsIterator allCellsEnd() const {
		return triangulation.all_faces_end();
	}
	
	
	/**
	 * Returns all incident to vh cells in counterclockwise order.
	 * If ch provided (must be incident), it will be the first in the answer.
	 * @threadsafe
	 */
	std::list<CellHandle> allIncidentCells(
			const VertexHandle vh, const CellHandle ch = CellHandle()) const {
		
		std::list<CellHandle> ans;
		/// Don't know why and how, but reading incident cells from
		/// CGAL triangulation by several threads is not thread-safe.
		/// So we have critical section here
		#pragma omp critical
		{
		auto faceCirculator = triangulation.incident_faces(vh, ch);
		auto begin = faceCirculator;
		do { ans.push_back(faceCirculator++); }
		while (faceCirculator != begin);
		}
		return ans;
	}
	
	
	/**
	 * Returns unity normal to contact surface between given cells
	 * (must be neighbors). Direction of normal is from "from" to "to"
	 */
	static RealD contactNormal(const CellHandle from, const CellHandle to) {
		int oppositeVertexIndex = from->index(to);
		
		VertexHandle cw = from->vertex(Triangulation::cw(oppositeVertexIndex));
		VertexHandle ccw = from->vertex(Triangulation::ccw(oppositeVertexIndex));
		
		RealD along = realD(cw->point()) - realD(ccw->point());
		return linal::normalize(
				linal::perpendicularClockwise(along));
	}
	
	
	static CellHandle someCellOfVertex(const VertexHandle vh) {
		return vh->face();
	}
	
	
	static real minimalCellHeight(const CellHandle ch) {
		return linal::minimalHeight(
				realD(ch->vertex(0)->point()),
				realD(ch->vertex(1)->point()),
				realD(ch->vertex(2)->point()));
	}
	
	
	/// @name convertion between CGAL and gcm data types
	/// @{
	static CgalPointD cgalPointD(const RealD& p) {
		return CgalPointD(p(0), p(1));
	}
	
	static RealD realD(const CgalPointD& p) {
		return {p.x(), p.y()};
	}
	
	static CgalVectorD cgalVectorD(const RealD& p) {
		return CgalVectorD(p(0), p(1));
	}
	/// @}
	
	
	/** Is triangle with a small layer around contains the point */
	static bool contains(const CellHandle ch, const RealD& q) {
		RealD a = realD(ch->vertex(0)->point());
		RealD b = realD(ch->vertex(1)->point());
		RealD c = realD(ch->vertex(2)->point());
		auto lambda = linal::barycentricCoordinates(a, b, c, q);
		return (lambda(0) > -EQUALITY_TOLERANCE) &&
		       (lambda(1) > -EQUALITY_TOLERANCE) &&
		       (lambda(2) > -EQUALITY_TOLERANCE);
	}
	
	
protected:
	/// CGAL triangulation structure
	Triangulation triangulation;
	
	USE_AND_INIT_LOGGER("gcm.Cgal2DTriangulation")
};


}

#endif // LIBGCM_CGAL2DTRIANGULATION_HPP