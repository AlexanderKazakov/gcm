#ifndef LIBGCM_INMMESHLOADER_HPP
#define LIBGCM_INMMESHLOADER_HPP

#include <lib/util/FileUtils.hpp>
#include <lib/util/StringUtils.hpp>

#include <lib/mesh/grid/Cgal3DGrid.hpp>
#include <lib/mesh/DefaultMesh.hpp>


namespace gcm {
	
/**
 * Class for loading unstructured tetrahedral 3d meshes 
 * by Yuri Vassilevski group from Institute of Numerical Mathematics
 */
class InmMeshLoader {
public:
	typedef Cgal3DGrid::VertexHandle    VertexHandle;
	typedef Cgal3DGrid::CellHandle      CellHandle;
	typedef Cgal3DGrid::Triangulation   Triangulation;
	
	static const int Dimensionality = 3;
	static const int NumberOfCellVertices = Dimensionality + 1;
	typedef linal::Vector<Dimensionality>            Point;
	typedef std::array<size_t, NumberOfCellVertices> Cell;
	typedef int                                      Material;
	
	static const char delimiter = ' ';
	static const int emptyMaterialFlag = 0;
	
	
	static void load(const std::string fileName, Triangulation& triangulation) {
		std::vector<Real3> points;
		std::map<Cell, Material> materials;
		
		USE_AND_INIT_LOGGER("InmMeshLoader")
		LOG_INFO("Start reading from file \"" << fileName << "\" ...");
		readFromFile(fileName, points, materials);
		
		
		LOG_INFO("Start adding points ...");
		CellHandle insertHint = CellHandle();
		for (size_t i = 0; i < points.size(); i++) {
			auto point = Cgal3DGrid::cgalPoint3(points[i]);
			VertexHandle vh = triangulation.insert(point, insertHint);
			vh->info() = i + 1; // number of the vertex in INM mesh format
			insertHint = vh->cell();
			if (i % 100000 == 0 && i != 0) { LOG_INFO(i << " points have been loaded"); }			
		}
		
		
		LOG_INFO("Start adding materials to cells ...");
		size_t counter = 0, matchCounter = 0;
		for (auto cell =  triangulation.finite_cells_begin();
		          cell != triangulation.finite_cells_end(); ++cell) {
			Cell inmCell = {
					cell->vertex(0)->info(),
					cell->vertex(1)->info(),
					cell->vertex(2)->info(),
					cell->vertex(3)->info(),
			};
			std::sort(inmCell.begin(), inmCell.end());
			
			const auto material = materials.find(inmCell);
			if (material != materials.end()) {
				++matchCounter;
				cell->info() = (size_t)(*material).second;
			} else {
				cell->info() = (size_t)emptyMaterialFlag;
			}
			
			if (++counter % 500000 == 0) { LOG_INFO(counter << " cells have been loaded"); }
		}
		
		
		LOG_INFO("Total number of given cells: " << materials.size());
		LOG_INFO("Total number of matched cells: " << matchCounter);
		size_t missedCellsCount = materials.size() - matchCounter;
		real missedCellsRatio = (real) missedCellsCount / (real) materials.size();
		LOG_INFO("Total number of missed cells: " << missedCellsCount
				<< ", percentage: " << missedCellsRatio * 100 << "%");
		
		
		correctHangedCells(triangulation);
	}
	
private:
	
	static void correctHangedCells(Triangulation& triangulation) {
		size_t emptyHangsCounter = 0, otherHangsCounter = 0;
		USE_AND_INIT_LOGGER("InmMeshLoader")
		LOG_INFO("Start replacing hanged cells ...");
		
		for (auto cell =  triangulation.finite_cells_begin();
		          cell != triangulation.finite_cells_end(); ++cell) {
			
			size_t& cellMaterialFlag = cell->info();
			size_t neighbor0MaterialFlag = cell->neighbor(0)->info();
			size_t neighbor1MaterialFlag = cell->neighbor(1)->info();
			size_t neighbor2MaterialFlag = cell->neighbor(2)->info();
			size_t neighbor3MaterialFlag = cell->neighbor(3)->info();
			
			if ( neighbor0MaterialFlag != cellMaterialFlag &&
			     neighbor0MaterialFlag == neighbor1MaterialFlag &&
			     neighbor0MaterialFlag == neighbor2MaterialFlag &&
			     neighbor0MaterialFlag == neighbor3MaterialFlag ) {
			// if all four neighbors have the same material different from the cell one
				
				if (cellMaterialFlag == emptyMaterialFlag) { ++emptyHangsCounter; }
				else { ++otherHangsCounter; }
				
				cellMaterialFlag = neighbor0MaterialFlag; // set material from neighbors
			}
		}
		
		LOG_INFO("Replaced " << emptyHangsCounter << " single empty cells and "
				<< otherHangsCounter << " single non-empty cells");
	}
	
	
public:
	
	static void readFromFile(const std::string fileName,
			std::vector<Real3>& points,
			std::map<Cell, Material>& materials) {
		points.clear();
		materials.clear();
		
		std::ifstream input;
		FileUtils::openTextFileStream(input, fileName);
		
		readPoints(input, points);
		readCells(input, materials);
		checkEndOfFile(input);
		
		FileUtils::closeFileStream(input);
	}
	
	
private:
	
	static void readPoints(std::ifstream& input, std::vector<Real3>& points) {
		std::string numberOfPointsStr;
		std::getline(input, numberOfPointsStr);
		assert_eq(StringUtils::split(numberOfPointsStr, delimiter).size(), 1);
		size_t numberOfPoints = std::stoul(numberOfPointsStr);
		assert_ge(numberOfPoints, NumberOfCellVertices);
		
		points.reserve(numberOfPoints);
		for (size_t i = 0; i < numberOfPoints; i++) {
			std::string pointStr;
			std::getline(input, pointStr);
			
			const auto coords = StringUtils::split(pointStr, delimiter);
			assert_eq(coords.size(), Dimensionality);
			
			real x = std::stod(coords[0]);
			real y = std::stod(coords[1]);
			real z = std::stod(coords[2]);
			points.push_back(Real3({x, y, z}));
		}
	}
	
	
	static void readCells(std::ifstream& input, std::map<Cell, Material>& materials) {
		std::string numberOfCellsStr;
		std::getline(input, numberOfCellsStr);
		assert_eq(StringUtils::split(numberOfCellsStr, delimiter).size(), 1);
		size_t numberOfCells = std::stoul(numberOfCellsStr);
		assert_ge(numberOfCells, 1);
		
		for (size_t i = 0; i < numberOfCells; i++) {
			std::string cellStr;
			std::getline(input, cellStr);
			
			const auto cellStrs = StringUtils::split(cellStr, delimiter);
			assert_eq(cellStrs.size(), NumberOfCellVertices + 1);
			
			Cell cell;
			for (size_t j = 0; j < NumberOfCellVertices; j++) {
				cell[j] = std::stoul(cellStrs[j]);
			}
			std::sort(cell.begin(), cell.end()); // sort to simplify search later
			Material material = std::stoi(cellStrs[NumberOfCellVertices]);
			
			materials.insert({cell, material});
		}
	}
	
	
	static void checkEndOfFile(std::ifstream& input) {
		std::string zeroLine; // end of file
		std::getline(input, zeroLine);
		assert_eq(StringUtils::split(zeroLine, delimiter).size(), 1);
		assert_eq(std::stoi(zeroLine), 0);
	}
	
	
};


}


#endif // LIBGCM_INMMESHLOADER_HPP