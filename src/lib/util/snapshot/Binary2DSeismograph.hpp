#ifndef LIBGCM_BINARY2DSEISMOGRAPH_HPP
#define LIBGCM_BINARY2DSEISMOGRAPH_HPP

#include <lib/mesh/DefaultMesh.hpp>
#include <lib/util/Logging.hpp>
#include <lib/util/snapshot/Snapshotter.hpp>

namespace gcm {
	/**
	 * For 2D binary seismography for inverse problem
	 */
	template<class TMesh>
	class Binary2DSeismograph : public Snapshotter {
	public:
		const std::string FILE_EXTENSION = std::string("bin");
		const std::string FOLDER_NAME    = std::string("2dseismo");

	protected:
		virtual void initializeImpl(const Task& task) override;
		virtual void beforeStatementImpl(const Statement&) override;
		virtual void snapshotImpl(const AbstractGrid* grid, const int step) override;
		virtual void afterStatementImpl() override;

	private:
		USE_AND_INIT_LOGGER("gcm.Binary2DSeismograph");
		precision* surface = nullptr; // values storage on current time step to write
		int sizeY = 0;
		precision hY = 1;
		precision tau = 1;
		
		std::ofstream fileStream;

		void writeHeadOfTable();
	};
}

#endif // LIBGCM_BINARY2DSEISMOGRAPH_HPP
