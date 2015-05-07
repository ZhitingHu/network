#ifndef MMSB_UTIL_HPP_
#define MMSB_UTIL_HPP_

#include "common.hpp"
#include <cmath>
#include <algorithm>
#include <gsl/gsl_sf_psi.h>

namespace mmsb {

struct AscSortByFirstOfUIntPair {
  bool operator() (const pair<uint32, uint32>& lhs,
      const pair<uint32, uint32>& rhs) {
    return (lhs.first < rhs.first)
        || (lhs.first == rhs.first && lhs.second < rhs.second);
  }
};
struct DesSortBySecondOfUIntPair {
  bool operator() (const pair<uint32, uint32>& lhs,
      const pair<uint32, uint32>& rhs) {
    return (lhs.second > rhs.second) 
        || (lhs.second == rhs.second && lhs.first > rhs.first);
  }
};
struct DesSortBySecondOfIntFloatPair {
  bool operator() (const pair<int, float>& lhs,
      const pair<int, float>& rhs) {
    return (lhs.second > rhs.second) 
        || (lhs.second == rhs.second && lhs.first > rhs.first);
  }
};

inline void PrintFloatVec(const vector<float>& v) {
  ostringstream oss;
  for (const auto v_ele : v) {
    oss << v_ele << " ";
  }
  oss << "\n";
  LOG(INFO) << oss.str();
}

/*
 * given log(a) and log(b), return log(a + b)
 *
 */
inline double log_sum(double log_a, double log_b) {
  double v;
  if (log_a < log_b) {
      v = log_b + log(1 + exp(log_a - log_b));
  }
  else {
      v = log_a + log(1 + exp(log_b - log_a));
  }
  return v;
}

 /**
   * Proc to calculate the value of the trigamma, the second
   * derivative of the loggamma function. Accepts positive matrices.
   * From Abromowitz and Stegun.  Uses formulas 6.4.11 and 6.4.12 with
   * recurrence formula 6.4.6.  Each requires workspace at least 5
   * times the size of X.
   *
   **/
inline double trigamma(double x) {
    double p;
    int i;

    x=x+6;
    p=1/(x*x);
    p=(((((0.075757575757576*p-0.033333333333333)*p+0.0238095238095238)
         *p-0.033333333333333)*p+0.166666666666667)*p+1)/x+0.5*p;
    for (i=0; i<6 ;i++) {
        x=x-1;
        p=1/(x*x)+p;
    }
    return(p);
}


/*
 * Caution(zhiting): this seems in-accurate !!
 *
 * taylor approximation of first derivative of the log gamma function
 */
//inline double digamma(double x) {
//  double p;
//  x=x+6;
//  p=1/(x*x);
//  p=(((0.004166666666667*p-0.003968253986254)*p+
//      0.008333333333333)*p-0.083333333333333)*p;
//  p=p+log(x)-0.5/x-1/(x-1)-1/(x-2)-1/(x-3)-1/(x-4)-1/(x-5)-1/(x-6);
//  return p;
//}
inline double digamma(double x) {
  return gsl_sf_psi(x);
}

inline double log_gamma(double x) {
     double z=1/(x*x);

    x=x+6;
    z=(((-0.000595238095238*z+0.000793650793651)
	*z-0.002777777777778)*z+0.083333333333333)/x;
    z=(x-0.5)*log(x)-x+0.918938533204673+z-log(x-1)-
	log(x-2)-log(x-3)-log(x-4)-log(x-5)-log(x-6);
    return z;
}


/*
 * argmax
 *
 */
inline int argmax(double* x, int n) {
    int i;
    double max = x[0];
    int argmax = 0;
    for (i = 1; i < n; i++)
    {
        if (x[i] > max)
        {
            max = x[i];
            argmax = i;
        }
    }
    return argmax;
}


} // namespace mmsb

#endif
