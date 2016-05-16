#ifndef LIBGCM_VTKSNAPSHOTTER_HPP
#define LIBGCM_VTKSNAPSHOTTER_HPP

#include <lib/util/snapshot/Snapshotter.hpp>
#include <lib/mesh/grid/CubicGrid.hpp>
#include <lib/mesh/grid/Cgal2DGrid.hpp>

#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkTriangle.h>


namespace gcm {

template<typename TGrid> struct VtkTypesBase;

template<> struct VtkTypesBase<Cgal2DGrid> {
	typedef vtkUnstructuredGrid          GridType;
	typedef vtkXMLUnstructuredGridWriter WriterType;
};
template<> struct VtkTypesBase<CubicGrid<1>> {
	typedef vtkStructuredGrid          GridType;
	typedef vtkXMLStructuredGridWriter WriterType;
};
template<> struct VtkTypesBase<CubicGrid<2>> {
	typedef vtkStructuredGrid          GridType;
	typedef vtkXMLStructuredGridWriter WriterType;
};
template<> struct VtkTypesBase<CubicGrid<3>> {
	typedef vtkStructuredGrid          GridType;
	typedef vtkXMLStructuredGridWriter WriterType;
};


template<typename TGrid>
struct VtkTypes : public VtkTypesBase<TGrid> {
	typedef VtkTypesBase<TGrid>       Base;
	typedef typename Base::GridType   GridType;
	typedef typename Base::WriterType WriterType;
	
	static vtkSmartPointer<GridType> NewGrid() {
		return vtkSmartPointer<GridType>::New();
	}

	static vtkSmartPointer<WriterType> NewWriter() {
		return vtkSmartPointer<WriterType>::New();
	}
	
	static std::string FileExtension() {
		return NewWriter()->GetDefaultFileExtension();
	}
};


class VtkUtils {
public:

/**
 * Write cubic grid geometry to vtk
 */
template<int D>
static void 
writeGeometry(const CubicGrid<D>& gcmGrid, vtkSmartPointer<vtkStructuredGrid> vtkGrid) {
	Int3 sizes = Int3::Ones();
	for (int i = 0; i < D; i++) {
		sizes(i) = gcmGrid.sizes(i);
	}
	vtkGrid->SetDimensions(sizes(0), sizes(1), sizes(2));

	auto points = vtkSmartPointer<vtkPoints>::New();
	points->Allocate((vtkIdType)gcmGrid.sizeOfRealNodes(), 0);
	for (auto it = gcmGrid.vtkBegin(); it != gcmGrid.vtkEnd(); ++it) {
		auto coords = gcmGrid.coords(it);
		real point[3] = {coords(0), coords(1), coords(2)};
		points->InsertNextPoint(point);
	}
	vtkGrid->SetPoints(points);
}

/**
 * Write CGAL 2D grid geometry to vtk
 */
static void 
writeGeometry(const Cgal2DGrid& gcmGrid, vtkSmartPointer<vtkUnstructuredGrid> vtkGrid) {
	auto points = vtkSmartPointer<vtkPoints>::New();
	points->Allocate((vtkIdType)gcmGrid.sizeOfRealNodes(), 0);
	for (auto it = gcmGrid.vtkBegin(); it != gcmGrid.vtkEnd(); ++it) {
		auto coords = gcmGrid.coords(it);
		real point[3] = {coords(0), coords(1), coords(2)};
		points->InsertNextPoint(point);
	}
	vtkGrid->SetPoints(points);

	auto triangle = vtkSmartPointer<vtkTriangle>::New();
	for (auto it = gcmGrid.cellBegin(); it != gcmGrid.cellEnd(); ++it) {
		auto verticesIndices = gcmGrid.getVerticesOfCell(it);
		int i = 0;
		for (const auto& vI : verticesIndices) {
			triangle->GetPointIds()->SetId(i, (vtkIdType)vI);
			i++;
		}
		vtkGrid->InsertNextCell(triangle->GetCellType(), triangle->GetPointIds());
	}
}

/**
 * Write vtk grid to vtk file with vtk writer
 */
template<typename VtkGridType, typename VtkWriterType>
static void
writeToFile(vtkSmartPointer<VtkGridType> vtkGrid,
            vtkSmartPointer<VtkWriterType> vtkWriter,
            const std::string name) {
#ifdef CONFIG_VTK_5
	vtkWriter->SetInput(vtkGrid);
#else
	vtkWriter->SetInputData(vtkGrid);
#endif
	vtkWriter->SetFileName(name.c_str());
	vtkWriter->Write();
}

/**
 * Write geometry of the given grid to vtk file
 */
template<typename GridType>
static void dumpGridToVtk(const GridType& grid, const std::string name = "grid") {
	typedef VtkTypes<GridType> VTK_TYPES;
	
	auto vtkGrid = VTK_TYPES::NewGrid();
	writeGeometry(grid, vtkGrid);
	writeToFile(vtkGrid, VTK_TYPES::NewWriter(), name + "." + VTK_TYPES::FileExtension());
}

};


template<typename TMesh>
class VtkSnapshotter : public Snapshotter {

typedef typename TMesh::Grid           Grid;
typedef VtkTypes<Grid>                 VTK_TYPES;
typedef typename VTK_TYPES::GridType   VtkGrid;
typedef typename VTK_TYPES::WriterType VtkWriter;

const std::string FOLDER_NAME = std::string("vtk");

bool enableSnapshotting = false;
std::vector<PhysicalQuantities::T> quantitiesToSnap;

virtual void beforeStatementImpl(const Statement& statement) override {
	enableSnapshotting = statement.vtkSnapshotter.enableSnapshotting;
	quantitiesToSnap = statement.vtkSnapshotter.quantitiesToSnap;
}

virtual void snapshotImpl(const AbstractGrid* _mesh, const int step) override {
	if (!enableSnapshotting) { return; }
	
	mesh = dynamic_cast<const TMesh*>(_mesh);
	assert_true(mesh);
	
	// note: we allocate a structure of size of the whole mesh at every time step!
	vtkGrid = VTK_TYPES::NewGrid();
	VtkUtils::writeGeometry(*mesh, vtkGrid);

	for (auto& quantity : TMesh::Model::PdeVariables::VECTORS) {
		writeQuantity(quantity.first, &VtkSnapshotter::insertVector, 3);
	}
	for (auto& quantity : quantitiesToSnap) {
		writeQuantity(quantity, &VtkSnapshotter::insertQuantity, 1);
	}
	for (auto& quantity : TMesh::Model::InternalOde::QUANTITIES) {
		writeQuantity(quantity.first, &VtkSnapshotter::insertOdeQuantity, 1);
	}
	
	VtkUtils::writeToFile(vtkGrid, VTK_TYPES::NewWriter(), makeFileNameForSnapshot(
			step, VTK_TYPES::FileExtension(), FOLDER_NAME));
}


USE_AND_INIT_LOGGER("gcm.VtkSnapshotter")

const TMesh* mesh; ///< local grid
vtkSmartPointer<VtkGrid> vtkGrid; ///< vtk grid


/**
 * The code below is for writing pde, ode, etc data for any rheology model
 */

typedef void (VtkSnapshotter::*InsertFunc)(PhysicalQuantities::T quantity,
                                           vtkSmartPointer<vtkFloatArray> vtkArr,
                                           const typename TMesh::VtkIterator& it);

void writeQuantity(PhysicalQuantities::T quantity, InsertFunc insertFunc,
                   const int numOfComponents) {
	auto vtkArr = vtkSmartPointer<vtkFloatArray>::New();
	vtkArr->SetNumberOfComponents(numOfComponents);
	vtkArr->Allocate((vtkIdType)mesh->sizeOfRealNodes(), 0);
	vtkArr->SetName(PhysicalQuantities::NAME.at(quantity).c_str());
	for (auto it = mesh->vtkBegin(); it != mesh->vtkEnd(); ++it) {
		(this->*insertFunc)(quantity, vtkArr, it);
	}
	vtkGrid->GetPointData()->AddArray(vtkArr);
}

void insertVector(PhysicalQuantities::T quantity, vtkSmartPointer<vtkFloatArray> vtkArr,
                  const typename TMesh::VtkIterator& it) {
	auto Get = TMesh::Model::PdeVariables::VECTORS.at(quantity).Get;
	auto linalVec = Get(mesh->pde(it));
	float vtkVec[3];
	for (int i = 0; i < 3; i++) {
		vtkVec[i] = (float) linalVec(i);
	}
	vtkArr->InsertNextTuple(vtkVec);
}

void insertQuantity(PhysicalQuantities::T quantity, vtkSmartPointer<vtkFloatArray> vtkArr,
                    const typename TMesh::VtkIterator& it) {
	auto Get = TMesh::Model::PdeVariables::QUANTITIES.at(quantity).Get;
	vtkArr->InsertNextValue((float)Get(mesh->pde(it)));
}

void insertOdeQuantity(PhysicalQuantities::T quantity, vtkSmartPointer<vtkFloatArray> vtkArr,
                       const typename TMesh::VtkIterator& it) {
	auto Get = TMesh::Model::InternalOde::QUANTITIES.at(quantity).Get;
	vtkArr->InsertNextValue((float)Get(mesh->ode(it)));
}


};


}

#endif // LIBGCM_VTKSNAPSHOTTER_HPP
