#ifndef LIBGCM_UTILS_HPP
#define LIBGCM_UTILS_HPP

#include <cmath>
#include <algorithm>
#include <set>

#include <libgcm/util/infrastructure/infrastructure.hpp>


namespace gcm {
class Utils {
public:

	/**
	 * Signum function
	 * @throws Exception for zero argument
	 */
	template<typename T>
	static int sign(const T& t) {
		assert_ne(t, 0);
		return (t > 0) ? 1 : -1;
	}
	
	
	/**
	 * Approach to compare two real numbers with given tolerance.
	 * 
	 * For big numbers, it's same to 
	 * \f$   \abs{f1 - f2} < tolerance * \abs{f1 + f2} / 2   \f$.
	 * For small numbers, it's same to 
	 * \f$   \abs{f1 - f2} < tolerance^(3/2) / 2   \f$.
	 */
	static inline bool
	approximatelyEqual(const real f1, const real f2, 
	                   const real tolerance = EQUALITY_TOLERANCE) {
		
		real relativeError2 =  4 * (f1 - f2) * (f1 - f2) /
		                          ((f1 + f2) * (f1 + f2) + tolerance);
		
		return relativeError2 < tolerance * tolerance;
	}
	
	
	/**
	 * Seed random generator to produce different values
	 */
	static void seedRand() {
		srand((unsigned int)time(0));
	}
	
	
	/**
	 * Produce pseudorandom uniformly distributed real number from min to max inclusive
	 * @note do not forget seedRand
	 */
	static real randomReal(const real min, const real max) {
		return ((max - min) * rand()) / RAND_MAX + min;
	}
	
	
	/**
	 * Any (random choice) element from the given set 
	 * (except the specified one) can be returned as answer.
	 */
	template<typename T>
	static T chooseRandomElementExceptSpecified(
			std::set<T> elements, const T& noChooseMe) {
		elements.erase(noChooseMe);
		int I = int(randomReal(0, (1 - EQUALITY_TOLERANCE) * (real)elements.size()));
		auto iter = elements.begin();
		for (int i = 0; i < I; i++) {
			++iter;
		}
		return *iter;
	}
	
	
	/**
	 * Check is the container has the value
	 */
	template<typename TContainer, typename TValue>
	static bool has(const TContainer& container, const TValue& value) {
		return std::find(container.begin(), container.end(), value) != container.end();
	}
	
	
	/**
	 * Return different number from {0, 1, 2}:
	 * 0,1 -> 2; 1,2 -> 0; 0,2 -> 1.
	 */
	static int other012(const int i, const int j) {
		assert_true(i != j && i >= 0 && i < 3 && j >= 0 && j < 3);
		for (int k = 0; k < 2; k++) {
			if (k != i && k != j) { return k; }
		}
		return 2;
	}
	
	
	/** All possible pairs combined from the set of items */
	template<typename T>
	static std::vector<std::pair<T, T>> makePairs(const std::set<T>& items) {
		
		std::vector<std::pair<T, T>> ans;
		ans.reserve((items.size() * (items.size() - 1)) / 2);
		
		for (auto i = items.begin(); i != items.end(); ++i) {
			for (auto j = std::next(i); j != items.end(); ++j) {
				ans.push_back({*i, *j});
			}
		}
		
		return ans;
	}
	
	
	/**
	 * Find the index of the given value in the given *sorted* array.
	 * Assert that such value is present and unique in the array.
	 */
	template<typename RAIter>
	static size_t findIndexOfValueInSortedArray(
			const RAIter begin, const RAIter end,
			typename std::add_const<decltype(*begin)>::type& value) {
		const std::pair<RAIter, RAIter> p = std::equal_range(begin, end, value);
		assert_true(std::next(p.first) == p.second);
		return (size_t)(p.first - begin);
	}
	
	
	/// @group Logical operations on SORTED ranges @{
	template<typename T>
	static T difference(const T& a, const T& b) {
		T ans;
		std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
				std::inserter(ans, ans.begin()));
		return ans;
	}
	
	template<typename T>
	static T summ(const T& a, const T& b) {
		T ans;
		std::set_union(a.begin(), a.end(), b.begin(), b.end(),
				std::inserter(ans, ans.begin()));
		return ans;
	}
	
	template<typename T>
	static T intersection(const T& a, const T& b) {
		T ans;
		std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
				std::inserter(ans, ans.begin()));
		return ans;
	}
	/// @}
	
};


}


#endif /* LIBGCM_UTILS_HPP */
