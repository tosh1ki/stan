#ifndef __STAN__PROB__DISTRIBUTIONS__MULTIVARIATE__CONTINUOUS__GAUSSIAN_DLM_HPP__
#define __STAN__PROB__DISTRIBUTIONS__MULTIVARIATE__CONTINUOUS__GAUSSIAN_DLM_HPP__

#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

#include <stan/math/matrix_error_handling.hpp>
#include <stan/math/error_handling.hpp>
#include <stan/math/error_handling/dom_err.hpp>
#include <stan/prob/constants.hpp>
#include <stan/prob/traits.hpp>
#include <stan/agrad/agrad.hpp>
#include <stan/meta/traits.hpp>
#include <stan/agrad/matrix.hpp>
#include <stan/math/matrix/log.hpp>
#include <stan/math/matrix/subtract.hpp>
#include <stan/math/matrix/sum.hpp>
#include <stan/math/matrix/multiply.hpp>
#include <stan/math/matrix/row.hpp>

// TODO: y as vector of vectors or matrix?

namespace stan {
  namespace prob {
    /**
     * The log of a multivariate Gaussian Process for the given y, Sigma, and
     * w.  y is a dxN matrix, where each column is a different observation and each
     * row is a different output dimension.  The Guassian Process is assumed to
     * have a scaled kernel matrix with a different scale for each output dimension.
     * This distribution is equivalent to, for \f$t = 1:N$,
     * \f{eqnarray*}{
     * y_t & \sim N(F \theta_t, V) \\
     * \theta_t & \sim N(G \theta_{t-1}, W) \\
     * \theta_0 & \sim N(0, diag(10^{6}))
     * }
     *
     * @param y A r x T matrix of observations.
     * @param F A r x n matrix. The design matrix.
     * @param G A n x n matrix. The transition matrix.
     * @param V A r x r matrix. The observation covariance matrix.
     * @param W A n x n matrix. The state covariance matrix.
     * @return The log of the joint density of the GDLM.
     * @throw std::domain_error if Sigma is not square, not symmetric, 
     * or not semi-positive definite.
     * @tparam T_y Type of scalar.
     * @tparam T_F Type of design matrix.
     * @tparam T_G Type of transition matrix.
     * @tparam T_V Type of observation covariance matrix.
     * @tparam T_W Type of state covariance matrix.
     */
    template <bool propto,
              typename T_y, 
              typename T_F, typename T_G,
              typename T_V, typename T_W
              >
    typename boost::math::tools::promote_args<T_y,T_F,T_G,T_V,T_W>::type
    gaussian_dlm_log(const Eigen::Matrix<T_y,Eigen::Dynamic,Eigen::Dynamic>& y,
            const Eigen::Matrix<T_F,Eigen::Dynamic,Eigen::Dynamic>& F,
            const Eigen::Matrix<T_G,Eigen::Dynamic,Eigen::Dynamic>& G,
            const Eigen::Matrix<T_V,Eigen::Dynamic,Eigen::Dynamic>& V,
            const Eigen::Matrix<T_W,Eigen::Dynamic,Eigen::Dynamic>& W) {
      static const char* function = "stan::prob::dlm_log(%1%)";
      // 
      typedef typename boost::math::tools::promote_args<T_y,T_F,T_G,T_V,T_W>::type T_lp;
      T_lp lp(0.0);
      
      using stan::math::log;
      using stan::math::sum;
      using stan::math::check_not_nan;
      using stan::math::check_size_match;
      using stan::math::check_positive;
      using stan::math::check_finite;
      using stan::math::check_symmetric;
      using stan::math::multiply;

      int r = y.rows(); // number of variables
      int T = y.cols(); // number of observations
      int n = G.rows(); // number of states

      // check F
      if (!check_size_match(function,
                            F.rows(), "rows of F",
                            y.rows(), "rows of y",
                            &lp))
        return lp;
      if (!check_size_match(function,
                            F.cols(), "columns of F",
                            G.rows(), "rows of G",
                            &lp))
        return lp;
      // check G
      if (!check_size_match(function,
                            G.rows(), "rows of G",
                            G.cols(), "columns of G",
                            &lp))
        return lp;
      // check V
      if (!check_symmetric(function, V, "V", &lp))
        return lp;
      // check W
      if (!check_symmetric(function, W, "W", &lp))
        return lp;

      if (y.cols() == 0 || y.rows() == 0)
        return lp;

      if (include_summand<propto>::value) {
        lp += NEG_LOG_SQRT_TWO_PI * r * T;
      }
      
      if (include_summand<propto,T_y,T_F,T_G,T_V,T_W>::value) {
        // TODO: make arguments
        Eigen::Matrix<typename 
                      boost::math::tools::promote_args<T_y,T_F,T_G,T_V,T_W>::type,
                      Eigen::Dynamic, 1> m(n);
        for (int i = 0; i < y.size(); i ++)
          m(i) = 0.0;
        Eigen::Matrix<typename 
                      boost::math::tools::promote_args<T_y,T_F,T_G,T_V,T_W>::type,
                      Eigen::Dynamic, Eigen::Dynamic> C(n, n);
        for (int i = 0; i < y.rows(); i ++)
          for (int j = 0; j < y.cols(); i ++)
            if (i == j)
              C(i, j) = 1.0;
            else
              C(i, j) = 0.0;

        Eigen::Matrix<typename 
                      boost::math::tools::promote_args<T_y,T_F,T_G,T_V,T_W>::type,
                      Eigen::Dynamic, 1> a(n);
        Eigen::Matrix<typename
                      boost::math::tools::promote_args<T_y,T_F,T_G,T_V,T_W>::type,
                      Eigen::Dynamic, Eigen::Dynamic> R(n, n);
        // Eigen::VectorXd f;
        // Eigen::VectorXd e;
        // Eigen::MatrixXd Q;
        // Eigen::MatrixXd A;

        for (int i = 0; i < n; ++i) {
          // Predict
          a = G * m;
          R = G * C * G.transpose() + W;
          // R = 0.5 * (R + transpose(R));
          // // one set ahead
          // f = F * a;          
          // Q = F * R * transpose(F) + V;
          // // filter
          // e = row(y, i + 1) - f;
          // A = R * transpose(F) * invserse(Q);
          // m = a + A * e;
          // C = R - A * Q * transpose(A);
          // C = 0.5 * (C + transpose(C));
          lp += 1;
        }
      }
      return lp;
    }

    template <typename T_y, 
              typename T_F, typename T_G,
              typename T_V, typename T_W
              >
    inline
    typename boost::math::tools::promote_args<T_y,T_F,T_G,T_V,T_W>::type
    gaussian_dlm_log(const Eigen::Matrix<T_y,Eigen::Dynamic,Eigen::Dynamic>& y,
            const Eigen::Matrix<T_F,Eigen::Dynamic,Eigen::Dynamic>& F,
            const Eigen::Matrix<T_G,Eigen::Dynamic,Eigen::Dynamic>& G,
            const Eigen::Matrix<T_V,Eigen::Dynamic,Eigen::Dynamic>& V,
            const Eigen::Matrix<T_W,Eigen::Dynamic,Eigen::Dynamic>& W) {
      return gaussian_dlm_log<false>(y, F, G, V, W);
    }
  }    
}

#endif
