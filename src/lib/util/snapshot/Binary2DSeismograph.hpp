#ifndef LIBGCM_BINARY2DSEISMOGRAPH_HPP
#define LIBGCM_BINARY2DSEISMOGRAPH_HPP

#include <fstream>

#include <lib/grid/StructuredGrid.hpp>
#include <lib/util/Logging.hpp>

namespace gcm {
	template<class TNode> class StructuredGrid;

	/**
	 * For 2D binary seismography for inverse problem
	 */
	template<class TNode>
	class Binary2DSeismograph {
	public:

		void startSeismo(StructuredGrid<TNode>* grid);
		void finishSeismo();

	private:
		USE_AND_INIT_LOGGER("gcm.Binary2DSeismograph");
		int seismoNumber = 0;

		real* surface = nullptr; // values storage on current time step to write

		void writeNextTimeStep(StructuredGrid<TNode>* grid);

		std::fstream seismoFileStream;
		void openSeismoFileStream(const std::string& fileName);
		void closeSeismoFileStream();

		std::string makeFileNameForSeismo(StructuredGrid<TNode>* grid);
	};
}

#endif // LIBGCM_BINARY2DSEISMOGRAPH_HPP
