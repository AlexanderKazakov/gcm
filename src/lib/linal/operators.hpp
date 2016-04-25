#ifndef LIBGCM_LINAL_OPERATORS_HPP
#define LIBGCM_LINAL_OPERATORS_HPP

#include <lib/linal/Matrix.hpp>
#include <lib/linal/linearSystems.hpp>

namespace gcm {
namespace linal {

/**
 * @return Negative of matrix m ( - m )
 */
template<int TM, int TN,
         typename TElement,
         template<int, int> class TSymmetry,
         template<int, typename> class TContainer>
MatrixBase<TM, TN, TElement, TSymmetry, TContainer>
operator-(const MatrixBase<TM, TN, TElement, TSymmetry, TContainer>& m) {
	MatrixBase<TM, TN, TElement, TSymmetry, TContainer> result;
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			result(i, j) = -m(i, j);
		}
	}
	return result;
}


/**
 * Computes summ of two matrices.
 * @return Matrix summ ( m1 + m2 )
 */
template<int TM, int TN,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2,
         template<int, typename> class TContainer3 = DefaultContainer>
MatrixBase<TM, TN,
           decltype(TElement1() + TElement2()),
           NonSymmetric, TContainer3>
operator+(const MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
          const MatrixBase<TM, TN, TElement2, TSymmetry2, TContainer2>& m2) {
	MatrixBase<TM, TN,
	           decltype(TElement1() + TElement2()),
	           NonSymmetric, TContainer3> result;
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			result(i, j) = m1(i, j) + m2(i, j);
		}
	}
	return result;
}


/**
 * Computes difference of two matrices.
 * @return Matrix difference ( m1 - m2 )
 */
template<int TM, int TN,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2,
         template<int, typename> class TContainer3 = DefaultContainer>
MatrixBase<TM, TN,
           decltype(TElement1() - TElement2()),
           NonSymmetric, TContainer3>
operator-(const MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
          const MatrixBase<TM, TN, TElement2, TSymmetry2, TContainer2>& m2) {
	MatrixBase<TM, TN,
	           decltype(TElement1() - TElement2()),
	           NonSymmetric, TContainer3> result;
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			result(i, j) = m1(i, j) - m2(i, j);
		}
	}
	return result;
}


/**
 * Computes product of two matrices C = m1 * m2.
 * m1: TMxTN.
 * m2: TNxTK.
 * C:  TMxTK.
 * @return Matrix product ( m1 * m2 )
 */
template<int TM, int TN, int TK,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2,
         template<int, typename> class TContainer3 = DefaultContainer>
MatrixBase<TM, TK,
           decltype(TElement1() * TElement2()),
           NonSymmetric, TContainer3>
operator*(const MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
          const MatrixBase<TN, TK, TElement2, TSymmetry2, TContainer2>& m2) {
	MatrixBase<TM, TK,
	           decltype(TElement1() * TElement2()),
	           NonSymmetric, TContainer3> result;
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TK; j++) {
			result(i, j) = m1(i, 0) * m2(0, j);
			for (int n = 1; n < TN; n++) {
				result(i, j) += m1(i, n) * m2(n, j);
			}
		}
	}
	return result;
}


/**
 * Computes product of two matrices C = m1 * m2, where m2 is diagonal (faster)
 * m1: TMxTN.
 * m2: TNxTN.
 * C:  TMxTN.
 * @return Matrix product ( m1 * m2 )
 */
template<int TM, int TN,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, typename> class TContainer2,
         template<int, typename> class TContainer3 = DefaultContainer>
MatrixBase<TM, TN,
           decltype(TElement1() * TElement2()),
           NonSymmetric, TContainer3>
operator*(const MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
          const MatrixBase<TN, TN, TElement2, Diagonal,   TContainer2>& m2) {
	MatrixBase<TM, TN,
	           decltype(TElement1() * TElement2()),
	           NonSymmetric, TContainer3> result;
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			result(i, j) = m1(i, j) * m2(j);
		}
	}
	return result;
}


/**
 * Computes product of two matrices C = m1 * m2, where m1 is diagonal (faster)
 * m1: TMxTM.
 * m2: TMxTN.
 * C:  TMxTN.
 * @return Matrix product ( m1 * m2 )
 */
template<int TM, int TN,
         typename TElement1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2,
         template<int, typename> class TContainer3 = DefaultContainer>
MatrixBase<TM, TN,
           decltype(TElement1() * TElement2()),
           NonSymmetric, TContainer3>
operator*(const MatrixBase<TM, TM, TElement1, Diagonal,   TContainer1>& m1,
          const MatrixBase<TM, TN, TElement2, TSymmetry2, TContainer2>& m2) {
	MatrixBase<TM, TN,
	           decltype(TElement1() * TElement2()),
	           NonSymmetric, TContainer3> result;
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			result(i, j) = m1(i) * m2(i, j);
		}
	}
	return result;
}


/**
 * Computes product of two diagonal matrices C = m1 * m2 (faster)
 * m1: TMxTM.
 * m2: TMxTM.
 * C:  TMxTM.
 * @return Matrix product ( m1 * m2 ) - diagonal matrix
 */
template<int TM,
         typename TElement1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, typename> class TContainer2,
         template<int, typename> class TContainer3 = DefaultContainer>
MatrixBase<TM, TM,
           decltype(TElement1() * TElement2()),
           Diagonal, TContainer3>
operator*(const MatrixBase<TM, TM, TElement1, Diagonal, TContainer1>& m1,
          const MatrixBase<TM, TM, TElement2, Diagonal, TContainer2>& m2) {
	MatrixBase<TM, TM,
	           decltype(TElement1() * TElement2()),
	           Diagonal, TContainer3> result;
	for (int i = 0; i < TM; i++) {
		result(i) = m1(i) * m2(i);
	}
	return result;
}


/** Multiplication by scalar arithmetic number (real, int, ...) */
template<int TM, int TN,
         typename TElement,
         template<int, int> class TSymmetry,
         template<int, typename> class TContainer,
         typename Number>
typename std::enable_if<std::is_arithmetic<Number>::value,
	MatrixBase<TM, TN, TElement, TSymmetry, TContainer> >::type
operator*(const MatrixBase<TM, TN, TElement, TSymmetry, TContainer>& m,
          const Number x) {
	typedef MatrixBase<TM, TN, TElement, TSymmetry, TContainer> ResultMatrixT;
	ResultMatrixT result;
	for (int i = 0; i < ResultMatrixT::SIZE; i++) {
		result(i) = m(i) * x;
	}
	return result;
}


/** Multiplication by scalar arithmetic number (real, int, ...) */
template<int TM, int TN,
         typename TElement,
         template<int, int> class TSymmetry,
         template<int, typename> class TContainer,
         typename Number>
typename std::enable_if<std::is_arithmetic<Number>::value,
	MatrixBase<TM, TN, TElement, TSymmetry, TContainer> >::type
operator*(const Number x, 
          const MatrixBase<TM, TN, TElement, TSymmetry, TContainer>& m) {
	return m * x;
}


/** Scalar division by real */
template<int TM, int TN,
         typename TElement,
         template<int, int> class TSymmetry,
         template<int, typename> class TContainer>
MatrixBase<TM, TN, TElement, TSymmetry, TContainer> 
operator/(const MatrixBase<TM, TN, TElement, TSymmetry, TContainer>& m,
          const real& x) {
	return m * (1.0 / x);
}


/**
 * Let SLE is A * x = b. So, formally x = b / A.
 * @return solveLinearSystem(A, b)
 */
template<int TM,
         typename TMatrixElement,
         template<int, int> class TSymmetry,
         template<int, typename> class TMatrixContainer,
         typename TVectorElement,
         template<int, typename> class TVectorContainer>
MatrixBase<TM, 1,
           decltype(TVectorElement() / TMatrixElement()),
           NonSymmetric, TVectorContainer>
operator/(const MatrixBase<TM,  1, TVectorElement, NonSymmetric, TVectorContainer>& b,
          const MatrixBase<TM, TM, TMatrixElement, TSymmetry,    TMatrixContainer>& A) {
	
	return solveLinearSystem(A, b);
}


/**
 * Add m2 to m1 modifying m1. 
 */
template<int TM, int TN,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2>
MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>&
operator+=(      MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
           const MatrixBase<TM, TN, TElement2, TSymmetry2, TContainer2>& m2) {
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			m1(i, j) += m2(i, j);
		}
	}
	return m1;
}


/**
 * Subtract m2 from m1 modifying m1. 
 */
template<int TM, int TN,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2>
MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>&
operator-=(      MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
           const MatrixBase<TM, TN, TElement2, TSymmetry2, TContainer2>& m2) {
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			m1(i, j) -= m2(i, j);
		}
	}
	return m1;
}


/**
 * Scalar multiplication m by x modifying m. 
 */
template<int TM, int TN,
         typename TElement,
         template<int, int> class TSymmetry,
         template<int, typename> class TContainer,
         typename Number>
typename std::enable_if<std::is_arithmetic<Number>::value,
	MatrixBase<TM, TN, TElement, TSymmetry, TContainer> >::type&
operator*=(MatrixBase<TM, TN, TElement, TSymmetry, TContainer>& m,
           const Number& x) {
	typedef MatrixBase<TM, TN, TElement, TSymmetry, TContainer> ResultMatrixT;
	for (int i = 0; i < ResultMatrixT::SIZE; i++) {
		m(i) *= x;
	}
	return m;
}


/**
 * Scalar division m by real x modifying m. 
 */
template<int TM, int TN,
         typename TElement,
         template<int, int> class TSymmetry,
         template<int, typename> class TContainer>
MatrixBase<TM, TN, TElement, TSymmetry, TContainer>&
operator/=(MatrixBase<TM, TN, TElement, TSymmetry, TContainer>& m,
           const real& x) {
	typedef MatrixBase<TM, TN, TElement, TSymmetry, TContainer> ResultMatrixT;
	for (int i = 0; i < ResultMatrixT::SIZE; i++) {
		m(i) /= x;
	}
	return m;
}


/**
 * Test on EXACT equality.
 */
template<int TM, int TN,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2>
bool operator==(const MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
                const MatrixBase<TM, TN, TElement2, TSymmetry2, TContainer2>& m2) {
	for (int i = 0; i < TM; i++) {
		for (int j = 0; j < TN; j++) {
			if (m1(i, j) != m2(i, j)) {
				return false;
			}
		}
	}
	return true;
}


/**
 * Test on EXACT inequality.
 */
template<int TM, int TN,
         typename TElement1,
         template<int, int> class TSymmetry1,
         template<int, typename> class TContainer1,
         typename TElement2,
         template<int, int> class TSymmetry2,
         template<int, typename> class TContainer2>
bool operator!=(const MatrixBase<TM, TN, TElement1, TSymmetry1, TContainer1>& m1,
                const MatrixBase<TM, TN, TElement2, TSymmetry2, TContainer2>& m2) {
	return !(m1 == m2);
}


}
}


#endif // LIBGCM_LINAL_OPERATORS_HPP
