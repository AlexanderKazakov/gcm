#ifndef LIBGCM_CONCEPTS_HPP
#define LIBGCM_CONCEPTS_HPP

#include <map>
#include <string>

/**
 * The file contains structs with enumerations and some properties for physical concepts used by the program.
 * The aim is to formalize and unify interconnection between parts of the program.
 * Use it every time when parts of the program communicate to each other by this concepts.
 */

namespace gcm {

	/**
     * For all physical quantities used in the program.
     */
	struct PhysicalQuantities {
		enum class QUANTITY {
			Vx /* Velocity component along x-axis */,
			Vy /* Velocity component along y-axis */,
			Vz /* Velocity component along z-axis */,

			Sxx /* Component of tension tensor */,
			Sxy /* Component of tension tensor */,
			Sxz /* Component of tension tensor */,
			Syy /* Component of tension tensor */,
			Syz /* Component of tension tensor */,
			Szz /* Component of tension tensor */,

			RHO /* Density */,
			PRESSURE /* Pressure = -Sxx (1D), -(Sxx + Syy)/2 (2D), -(Sxx + Syy + Szz)/3 (3D) */,

			SIZE_OF_ENUM /* This MUST be at the last position here! DO NOT insert anything after that!!! */
		};

		/** string names of thq quantities */
		static const std::map<QUANTITY, std::string> NAME;
	};

	/**
	 * For all types of waves used in the program.
	 */
	struct Waves {
		enum class WAVE {
			P_FORWARD, P_BACKWARD,
			S1_FORWARD, S1_BACKWARD,
			S2_FORWARD, S2_BACKWARD,

			SIZE_OF_ENUM /* This MUST be at the last position here! DO NOT insert anything after that!!! */
		};

		static const std::map<WAVE, std::string> NAME;
	};

	/**
	 * For all types of border conditions used in the program.
	 */
	struct BorderCondition {
		enum class CONDITION {
			NON_REFLECTION,
			FREE_BORDER,

			SIZE_OF_ENUM /* This MUST be at the last position here! DO NOT insert anything after that!!! */
		};

		static const std::map<CONDITION, std::string> NAME;
	};

	/* Six faces of axis-aligned cube */
	enum class CUBIC_BORDERS {X_LEFT, X_RIGHT, Y_LEFT, Y_RIGHT, Z_LEFT, Z_RIGHT};
};


#endif // LIBGCM_CONCEPTS_HPP
