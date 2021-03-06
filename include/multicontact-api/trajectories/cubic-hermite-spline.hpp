// Copyright (c) 2015-2018, CNRS
// Authors: Justin Carpentier <jcarpent@laas.fr>

#ifndef __multicontact_api_trajectories_cubic_hermite_spline_hpp__
#define __multicontact_api_trajectories_cubic_hermite_spline_hpp__

#include "multicontact-api/trajectories/fwd.hpp"
#include "multicontact-api/serialization/eigen-matrix.hpp"
#include "multicontact-api/serialization/archive.hpp"

#include <Eigen/Dense>

namespace multicontact_api
{
  namespace trajectories
  {
    namespace detail
    {
//      template<typename _Scalar, int _dim>
    }

    template<typename _Scalar, int _dim>
    struct CubicHermiteSplineTpl
    : public serialization::Serializable< CubicHermiteSplineTpl<_Scalar,_dim> >
    {
      EIGEN_MAKE_ALIGNED_OPERATOR_NEW

      typedef _Scalar Scalar;
      enum { dim = _dim };
      typedef Eigen::Matrix<Scalar,dim,Eigen::Dynamic> MatrixDx;
      typedef Eigen::Matrix<Scalar,Eigen::Dynamic,1> VectorX;
      typedef Eigen::Matrix<Scalar,dim,1> VectorD;
      typedef Eigen::DenseIndex Index;

      CubicHermiteSplineTpl()
      : m_absicca()
      , m_points(_dim,0)
      , m_derivatives(_dim,0)
      , m_ds()
      {}

      CubicHermiteSplineTpl(const Index size)
      : m_absicca(size)
      , m_points(_dim,size)
      , m_derivatives(_dim,size)
      , m_ds((size-1>0)?size-1:0)
      {}

      template<typename VectorDerived, typename MatrixDerived>
      CubicHermiteSplineTpl(const Eigen::MatrixBase<VectorDerived> & absicca,
                            const Eigen::MatrixBase<MatrixDerived> & points,
                            const Eigen::MatrixBase<MatrixDerived> & derivatives
                            )
      : m_absicca(absicca)
      , m_points(points)
      , m_derivatives(derivatives)
      , m_ds(absicca.size()-1)
      {
        //EIGEN_STATIC_ASSERT_SAME_MATRIX_SIZE(MatrixDerived,MatrixDx);
        //EIGEN_STATIC_ASSERT_SAME_VECTOR_SIZE(VectorDerived,VectorX);

        assert(points.cols() == derivatives.cols() && "Points and derivatives must have the same number of columns.");
        assert(points.cols() == absicca.size() && "Points and times must have the same dimension.");

        compute();
        assert(check());
      }

      template<typename OtherScalar>
      CubicHermiteSplineTpl(const CubicHermiteSplineTpl<OtherScalar,dim> & other)
      : m_absicca(other.m_absicca)
      , m_points(other.m_points)
      , m_derivatives(other.m_derivatives)
      {
        compute();
      }

      static CubicHermiteSplineTpl Constant(const VectorD & point)
      {
        MatrixDx points(point.size(),2), derivatives(MatrixDx::Zero(point.size(),2));
        points << point, point;

        VectorX absicca(2); absicca << 0., 1.;

        return CubicHermiteSplineTpl(absicca,points,derivatives);
      }

      template<typename OtherScalar>
      bool operator==(const CubicHermiteSplineTpl<OtherScalar,dim> & other) const
      {
        return
        m_absicca == other.m_absicca
        && m_points == other.m_points
        && m_derivatives == other.m_derivatives
        && m_ds == other.m_ds
        ;
      }

      template<typename OtherScalar>
      CubicHermiteSplineTpl operator+(const CubicHermiteSplineTpl<OtherScalar,dim> & other) const
      {
        assert(dimension() == other.dimension());
        assert(m_absicca.isApprox(other.m_absicca));
        return CubicHermiteSplineTpl(m_absicca, m_points+other.m_points,
                                     m_derivatives+other.m_derivatives);
      }


      template<typename OtherScalar>
      CubicHermiteSplineTpl operator-(const CubicHermiteSplineTpl<OtherScalar,dim> & other) const
      {
        assert(dimension() == other.dimension());
        assert(m_absicca.isApprox(other.m_absicca));
        return CubicHermiteSplineTpl(m_absicca, m_points-other.m_points,
                                     m_derivatives-other.m_derivatives);
      }

      template<typename OtherScalar>
      CubicHermiteSplineTpl& operator+=(const CubicHermiteSplineTpl<OtherScalar,dim> & other)
      {
        assert(dimension() == other.dimension());
        assert(m_absicca.isApprox(other.m_absicca));
        m_points += other.m_points;
        m_derivatives += other.m_derivatives;
        return *this;
      }


      template<typename OtherScalar>
      CubicHermiteSplineTpl& operator-=(const CubicHermiteSplineTpl<OtherScalar,dim> & other)
      {
        assert(dimension() == other.dimension());
        assert(m_absicca.isApprox(other.m_absicca));
        m_points-=other.m_points;
        m_derivatives-=other.m_derivatives;
        return *this;
      }

      template<typename OtherScalar>
      bool operator!=(const CubicHermiteSplineTpl<OtherScalar,dim> & other) const
      { return !(*this != other); }

      const VectorX & absicca() const { return m_absicca; }
      void setAbsicca(const VectorX & absicca)
      {
        assert(absicca.size() == size());
        m_absicca = absicca;
        compute();
        assert(check());
      }

      const MatrixDx & points() const { return m_points; }
      MatrixDx & points() { return m_points; }

      const MatrixDx & derivatives() const { return m_derivatives; }
      MatrixDx & derivatives() { return m_derivatives; }

      ///
      /// \brief Returns the number of points contained in the trajectory.
      ///
      Index size() const { return m_points.cols(); }

      ///
      /// \brief Resize the trajectory with the given input size.
      ///
      void resize(const Index size)
      {
        assert(size >= 0);
        m_points.conservativeResize(dimension(),size);
        m_derivatives.conservativeResize(dimension(),size);
        m_absicca.conservativeResize(size);
        m_ds.conservativeResize(size-1);
      }

      ///
      /// \brief Returns the dimension of the trajectory.
      ///
      Index dimension() const { return m_points.rows(); }

      ///
      /// \brief Returns the number of intervals contained in the trajectory.
      ///
      Index numIntervals() const { return size()-1; }

      ///
      /// \brief Eval the spline at value s.
      ///
      template<typename Derived1, typename Derived2>
      void eval(const Scalar t,
                const Eigen::MatrixBase<Derived1> & p,
                const Eigen::MatrixBase<Derived2> & m) const
      {
        //EIGEN_STATIC_ASSERT_SAME_MATRIX_SIZE(Derived1,VectorD)
        //EIGEN_STATIC_ASSERT_SAME_MATRIX_SIZE(Derived2,VectorD)

        const Index id = findInterval(t);
        Eigen::MatrixBase<Derived1> & p_ = const_cast<Eigen::MatrixBase<Derived1> &>(p);
        Eigen::MatrixBase<Derived2> & m_ = const_cast<Eigen::MatrixBase<Derived2> &>(m);
        if(id == size()-1)
        {
          p_ = m_points.template rightCols<1>();
          m_ = m_derivatives.template rightCols<1>();
          return;
        }

        const typename MatrixDx::ConstColXpr p0 = m_points.col(id);
        const typename MatrixDx::ConstColXpr p1 = m_points.col(id+1);

        const typename MatrixDx::ConstColXpr m0 = m_derivatives.col(id);
        const typename MatrixDx::ConstColXpr m1 = m_derivatives.col(id+1);

        const Scalar & t0 = m_absicca[id];
        const Scalar & t1 = m_absicca[id+1];

        const Scalar dt = (t1-t0);
        const Scalar alpha = (t - t0)/dt;

        assert(0. <= alpha <= 1. && "alpha must be in [0,1]");

        Scalar h00, h10, h01, h11;
        evalCoeffs(alpha,h00,h10,h01,h11);
        h10 *= dt; h11 *= dt;

        p_ = (h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1);

        m_ = (1.-alpha) * m0 + alpha * m1;
      }

    protected:

      static void evalCoeffs(const Scalar t,
                             Scalar & h00, Scalar & h10, Scalar & h01, Scalar & h11)
      {
        h00 = 1.; h10 = t; h01 = 0.; h11 = 0.;

        Scalar t_pow = t;

        // t^2
        t_pow = t*t;
        h00 -= 3*t_pow;
        h10 -= 2*t_pow;
        h01 += 3*t_pow;
        h11 -= t_pow;

        // t^3
        t_pow *= t;
        h00 += 2*t_pow;
        h10 += t_pow;
        h01 -= 2*t_pow;
        h11 += t_pow;
      }

      ///
      /// \brief Internal check. It checks if the abissica is monotone.
      ///
      bool check() const
      {
        return (m_ds.array() > 0.).all();
      }

      ///
      /// \brief Internal computation
      ///
      void compute()
      {
        assert(numIntervals() >= 1 && "The number of intervals must be greater or equal to 1.");
        m_ds = m_absicca.tail(numIntervals()) - m_absicca.head(numIntervals());
      }

      ///
      /// \brief Returns the index of the interval for the interpolation.
      ///
      Index findInterval(const Scalar t) const
      {
        if(t < m_absicca[0]) return 0;
        if(t > m_absicca[size()-1]) return numIntervals()-1;

        Index left_id = 0;
        Index right_id = size()-1;

        while(left_id <= right_id)
        {
          const Index middle_id = left_id + (right_id - left_id)/2;
          if(m_absicca[middle_id] < t)
            left_id = middle_id+1;
          else if(m_absicca[middle_id] > t)
            right_id = middle_id-1;
          else
            return middle_id;
        }

        return left_id-1;
      }

      VectorX m_absicca;
      MatrixDx m_points;
      MatrixDx m_derivatives;
      VectorX m_ds;

      // Serialization of the class
      friend class boost::serialization::access;

      template<class Archive>
      void save(Archive & ar, const unsigned int /*version*/) const
      {
        const Index m_size = size();
        ar & boost::serialization::make_nvp("size",m_size);
        ar & boost::serialization::make_nvp("absicca",m_absicca);
        ar & boost::serialization::make_nvp("points",m_points);
        ar & boost::serialization::make_nvp("derivatives",m_derivatives);
      }

      template<class Archive>
      void load(Archive & ar, const unsigned int /*version*/)
      {
        Index m_size;
        ar & boost::serialization::make_nvp("size",m_size);
        resize(m_size);

        ar & boost::serialization::make_nvp("absicca",m_absicca);
        ar & boost::serialization::make_nvp("points",m_points);
        ar & boost::serialization::make_nvp("derivatives",m_derivatives);

        compute();
      }

      BOOST_SERIALIZATION_SPLIT_MEMBER()

    };

    template<typename Scalar, int dim> CubicHermiteSplineTpl<Scalar,dim>
    createHermiteSplineAtAbsicca(const CubicHermiteSplineTpl<Scalar,dim>& spline,
                                    const typename
                                    CubicHermiteSplineTpl<Scalar,dim>::VectorX& absicca)
    {
      typedef CubicHermiteSplineTpl<Scalar,dim> SplineType;
      typedef typename SplineType::VectorX VectorX;
      typedef typename SplineType::MatrixDx MatrixDx;
      const VectorX& t0 = spline.absicca();

      //Assert that the final and initial point is the same
      assert(absicca[0] == t0[0]);
      assert(absicca[absicca.size()-1] == t0[t0.size()-1]);
      //Assert that the new resolution is higher.
      //Otherwise, there might be loss of information.
      assert(absicca.size() >= spline.size());

      MatrixDx p_new(dim, absicca.size());
      MatrixDx m_new(dim, absicca.size());

      for (int k=0;k<absicca.size();k++)
      {
        typename MatrixDx::ColXpr p = p_new.col(k);
        typename MatrixDx::ColXpr m = m_new.col(k);
        spline.eval(absicca[k], p, m);
      }

      return SplineType(absicca, p_new, m_new);
    }
  }
}

#endif // ifndef __multicontact_api_trajectories_cubic_hermite_spline_hpp__
