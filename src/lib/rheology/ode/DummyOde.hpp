#ifndef LIBGCM_DUMMYODE_HPP
#define LIBGCM_DUMMYODE_HPP

#include <cmath>

#include <lib/rheology/variables/GetSetter.hpp>
#include <lib/util/Types.hpp>
#include <lib/util/task/Task.hpp>

namespace gcm {
/**
 * Dummy (stub) for cases when internal ODE is not present
 */
class DummyOde {
public:
	static const bool NonTrivial = false;
	struct Variables { };

	static const std::map<PhysicalQuantities::T, GetSetter<Variables> > QUANTITIES;

	DummyOde(const Statement&) { }

	template<typename TNodePtr>
	void nextStep(TNodePtr, const real) { }
};


}

#endif // LIBGCM_DUMMYODE_HPP
