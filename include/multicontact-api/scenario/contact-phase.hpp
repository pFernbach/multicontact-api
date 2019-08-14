// Copyright (c) 2015-2018, CNRS
// Authors: Justin Carpentier <jcarpent@laas.fr>

#ifndef __multicontact_api_scenario_contact_phase_hpp__
#define __multicontact_api_scenario_contact_phase_hpp__

#include "multicontact-api/scenario/fwd.hpp"
#include "multicontact-api/scenario/contact-patch.hpp"

#include "multicontact-api/serialization/archive.hpp"
#include "multicontact-api/serialization/eigen-matrix.hpp"
#include "multicontact-api/serialization/spatial.hpp"
#include "multicontact-api/geometry/second-order-cone.hpp"
#include "multicontact-api/geometry/linear-cone.hpp"
#include "multicontact-api/container/ref.hpp"

#include <curves/curve_abc.h>
#include <curves/piecewise_curve.h>
#include <curves/polynomial.h>
#include <curves/cubic_hermite_spline.h>

#include <map>
#include <string>
#include <sstream>

//#include <boost/array.hpp>
#include <boost/serialization/map.hpp>

namespace multicontact_api
{
  namespace scenario
  {

    template<typename _Scalar, int _dim>
    struct ContactPhaseTpl : public serialization::Serializable< ContactPhaseTpl<_Scalar,_dim> >
    {
      EIGEN_MAKE_ALIGNED_OPERATOR_NEW

      typedef _Scalar Scalar;
      typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> point_t;
      typedef curves::curve_abc<Scalar, Scalar, true, point_t> curve_abc_t;

      typedef ContactPatchTpl<Scalar> ContactPatch;
      typedef std::map< std::string, ContactPatch > ContactPatchMap;
      typedef std::map< std::string, curve_abc_t* > CurveMap;
      typedef std::vector< container::comparable_reference_wrapper<ContactPatch> > ContactPatchVector;
      //typedef std::vector< container::comparable_reference_wrapper<ContactPatch const> > ConstContactPatchVector; //???
      typedef geometry::SecondOrderCone<_Scalar,6> SOC6;
      typedef geometry::WrenchConeTpl<_Scalar> WrenchCone;
      typedef Eigen::DenseIndex Index;
      typedef typename ContactPatch::SE3 SE3;
      typedef Eigen::Matrix<double,6,Eigen::Dynamic> Matrix6x;
      typedef Eigen::Matrix<_Scalar,Eigen::Dynamic,1> ConfigurationVector;

      typedef curves::cubic_hermite_spline<Scalar,Scalar,3,true> CubicHermiteSpline3;
      typedef curves::cubic_hermite_spline<Scalar,Scalar,24,true> CubicHermiteSpline24;

      typedef Eigen::Matrix<Scalar,9,1> StateVector;

      enum { dim = _dim };


      /*Variables*/
      std::vector<std::string> m_effector_names;
      ContactPatchMap m_contact_patches;
      std::pair<double,double> m_time_interval;
      ConfigurationVector m_reference_configuration;

      curve_abc_t* m_q = NULL;
      curve_abc_t* m_dq = NULL;
      curve_abc_t* m_ddq = NULL;
      curve_abc_t* m_tau = NULL;
      curve_abc_t* m_c = NULL;
      curve_abc_t* m_dc = NULL;
      curve_abc_t* m_ddc = NULL;
      curve_abc_t* m_L = NULL;
      curve_abc_t* m_dL = NULL;
      curve_abc_t* m_wrench = NULL;
      curve_abc_t* m_zmp = NULL;

      CurveMap m_contact_forces;
      CurveMap m_contact_normal_force;
      CurveMap m_effector_trajectories;

      StateVector m_init_state;
      StateVector m_final_state;

      CubicHermiteSpline3 m_angular_momentum_ref;
      CubicHermiteSpline3 m_com_ref;
      CubicHermiteSpline3 m_vcom_ref;
      CubicHermiteSpline24 m_forces_ref;
      
      /*Variables*/


      /// \brief Default constructor
      ContactPhaseTpl()
      : m_sowc_placement(SE3::Identity())
      {}

      /// \brief Copy constructor
      template<typename S2>
      ContactPhaseTpl(const ContactPhaseTpl<S2,dim> & other)
      : m_contact_patches(other.m_contact_patches)
      , m_sowc(other.m_sowc)
      , m_sowc_placement(other.m_sowc_placement)
      , m_lwc(other.m_lwc)
      {}

      const ContactPatchMap & contact_patches() const { return m_contact_patches; }
      ContactPatchMap & contact_patches() { return m_contact_patches; }

      /// \Returns a reference of the second order wrench cone
      const SOC6 & sowc() const { return m_sowc; }
      SOC6 & sowc() { return m_sowc; }

      const SE3 & sowcPlacement() const { return m_sowc_placement; }
      SE3 & sowcPlacement() { return m_sowc_placement; }

      const Matrix6x & doubleDescription() const { return m_double_description; }
      Matrix6x & doubleDescription() { return m_double_description; }

      const WrenchCone & lwc() const { return m_lwc; }
      WrenchCone & lwc() { return m_lwc; }

      /// \returns the number of active patches
      Index numActivePatches() const
      {
        Index num_active = 0;
        for(typename ContactPatchMap::const_iterator it = m_contact_patches.begin();
            it != m_contact_patches.end(); ++it)
          if(it->second.active()) num_active++;

        return num_active;
      }

      /// \returns the number of inactive patches
      Index numInactivePatches() const
      { return dim - numActivePatches(); }

      template<typename S2>
      bool operator==(const ContactPhaseTpl<S2,dim> & other) const
      {
        return
        m_contact_patches == other.m_contact_patches
        && m_lwc == other.m_lwc
        && m_sowc == other.m_sowc
        && m_init_state == other.m_init_state
        && m_final_state == other.m_final_state
        ;
        // TO DO : Test equality on curves
      }

      template<typename S2>
      bool operator!=(const ContactPhaseTpl<S2,dim> & other) const
      { return !(*this == other); }


      ///
      /// \brief Returns the first active patch.
      ///
      /// \remark This method is useful one looks for the lonely active patch.
      ///
      /// \returns the first active patch.
      ///
      const ContactPatch & getActivePatch() const
      {
        for(typename ContactPatchMap::iterator it = m_contact_patches.begin();
            it != m_contact_patches.end(); ++it)
        {
          if(it->active()) return *it;
        }
      }
      ContactPatch & getActivePatch()
      { return const_cast<ContactPatch &>(static_cast<const ContactPhaseTpl*>(this)->getActivePatch()); }

      ContactPatchVector getActivePatches()
      {
        ContactPatchVector res; res.reserve((size_t)dim);
        for(typename ContactPatchMap::iterator it = m_contact_patches.begin();
            it != m_contact_patches.end(); ++it)
        {
          if(it->second.active()) res.push_back(typename ContactPatchVector::value_type(it->second));
        }
        return res;
      }

      /// \brief Check (assert) if all names used as key in all maps (CurveMap and ContactPatchMap) are in m_effector_names.
      ///
      void checkNameAllMaps()
      {
        checkNamesMapAndVector<std::string, ContactPatchMap, std::vector<std::string> >(
            m_contact_patches, m_effector_names
          );
        checkNamesMapAndVector<std::string, CurveMap, std::vector<std::string> >(
            m_contact_forces, m_effector_names
          );
        checkNamesMapAndVector<std::string, CurveMap, std::vector<std::string> >(
            m_contact_normal_force, m_effector_names
          );
        checkNamesMapAndVector<std::string, CurveMap, std::vector<std::string> >(
            m_effector_trajectories, m_effector_names
          );
      }

    protected:
      /*Attributes 2*/
      /// \brief Second Order Wrench Cone (SOWC) representing the Minkoski sum of the patch linear wrench cone.
      SOC6 m_sowc;
      SE3 m_sowc_placement;
      Matrix6x m_double_description;
      /// \brief Linear Wrench Cone (LWC) representing the Minkoski sum of the patch linear wrench cone.
      WrenchCone m_lwc;
      /*Attributes 2*/

      template <typename KeyType, typename MapType, typename VectorType>
      void checkNamesMapAndVector(const MapType m, const VectorType v)
      {
        bool haveName = true;
        KeyType keyNotFound;
        for(typename MapType::iterator it=m.begin(); it!=m.end(); ++it)
        {
          if(!isInList<std::string, std::vector<std::string> >(it->first,v))
          {
            haveName = false;
            keyNotFound = it->first;
            break;
          }
        }
        std::string msg;
        msg = std::string("Map contains a name not found => ") + to_string<KeyType>(keyNotFound);
        assert(haveName && msg.c_str());
      }

      template <typename ValueType, typename VectorType>
      bool isInList(const ValueType value, const VectorType v) {
        return std::find(v.begin(), v.end(), value) != v.end();
      }

      template <typename T>
      std::string to_string(T value)
      {
        std::ostringstream os;
        os << value;
        return os.str();
      }

    private:

      template <class Archive, typename Type >
      void serialize_pointer(Archive & ar, std::string name, Type * ptr) const
      {
        std::cout << "start ser "<<name<<std::endl;
        std::string name_boolean = name + std::string("_bool");
        bool isNull = false;
        std::cout<<"value ptr : "<<ptr<<std::endl;
        if (ptr != NULL)
        {
          std::cout << "ptr not NULL"<<std::endl;
          ar & boost::serialization::make_nvp(name_boolean.c_str(),isNull);
          //std::cout << "Here 2"<<std::endl;
          ar & boost::serialization::make_nvp(name.c_str(),ptr);
        }
        else
        {
          std::cout << "ptr NULL"<<std::endl;
          isNull = true;
          ar & boost::serialization::make_nvp(name_boolean.c_str(),isNull);
        }
        std::cout << "end"<<std::endl;
      }

      template <class Archive, typename Type >
      void deserialize_pointer(Archive & ar, std::string name, Type ** ptr) const
      {
        std::cout << "start deser "<<name<<std::endl;
        std::string name_boolean = name + std::string("_bool");
        bool isNull;
        ar >> boost::serialization::make_nvp(name_boolean.c_str(),isNull);
        if (!isNull)
        {
          std::cout << "ptr not NULL"<<std::endl;
          ar >> boost::serialization::make_nvp(name.c_str(),(*ptr));
        }
        else
        {
          std::cout << "ptr NULL"<<std::endl;
          (*ptr) = NULL;
        }
        std::cout << "end"<<std::endl;
      }

      // Serialization of the class
      friend class boost::serialization::access;

      template<class Archive>
      void save(Archive & ar, const unsigned int /*version*/) const
      {
        ar & boost::serialization::make_nvp("effector_names",m_effector_names);
        ar & boost::serialization::make_nvp("contact_patches",m_contact_patches);
        ar & boost::serialization::make_nvp("time_interval",m_time_interval);
        ar & boost::serialization::make_nvp("reference_configuration",m_reference_configuration);

        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_q"), m_q);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_dq"), m_dq);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_ddq"), m_ddq);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_tau"), m_tau);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_c"), m_c);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_dc"), m_dc);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_ddc"), m_ddc);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_L"), m_L);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_dL"), m_dL);
        serialize_pointer<Archive, curve_abc_t>(ar, std::string("m_wrench"), m_wrench);

        ar & boost::serialization::make_nvp("contact_forces",m_contact_forces);
        ar & boost::serialization::make_nvp("contact_normal_force",m_contact_normal_force);
        ar & boost::serialization::make_nvp("effector_trajectories",m_effector_trajectories);
        ar & boost::serialization::make_nvp("init_state",m_init_state);
        ar & boost::serialization::make_nvp("final_state",m_final_state);

        ar & boost::serialization::make_nvp("m_angular_momentum_ref", m_angular_momentum_ref);
        ar & boost::serialization::make_nvp("m_com_ref", m_com_ref);
        ar & boost::serialization::make_nvp("m_vcom_ref", m_vcom_ref);
        ar & boost::serialization::make_nvp("m_forces_ref", m_forces_ref);

        ar & boost::serialization::make_nvp("lwc",m_lwc);
        ar & boost::serialization::make_nvp("sowc",m_sowc);
        ar & boost::serialization::make_nvp("sowc_placement",m_sowc_placement);
        ar & boost::serialization::make_nvp("double_description",m_double_description);
      }

      template<class Archive>
      void load(Archive & ar, const unsigned int /*version*/)
      {
        ar >> boost::serialization::make_nvp("effector_names",m_effector_names);
        ar >> boost::serialization::make_nvp("contact_patches",m_contact_patches);
        ar >> boost::serialization::make_nvp("time_interval",m_time_interval);
        ar >> boost::serialization::make_nvp("reference_configuration",m_reference_configuration);

        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_q"), &m_q);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_dq"), &m_dq);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_ddq"), &m_ddq);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_tau"), &m_tau);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_c"), &m_c);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_dc"), &m_dc);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_ddc"), &m_ddc);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_L"), &m_L);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_dL"), &m_dL);
        deserialize_pointer<Archive, curve_abc_t>(ar, std::string("m_wrench"), &m_wrench);

        ar >> boost::serialization::make_nvp("contact_forces",m_contact_forces);
        ar >> boost::serialization::make_nvp("contact_normal_force",m_contact_normal_force);
        ar >> boost::serialization::make_nvp("effector_trajectories",m_effector_trajectories);
        ar >> boost::serialization::make_nvp("init_state",m_init_state);
        ar >> boost::serialization::make_nvp("final_state",m_final_state);

        ar >> boost::serialization::make_nvp("m_angular_momentum_ref", m_angular_momentum_ref);
        ar >> boost::serialization::make_nvp("m_com_ref", m_com_ref);
        ar >> boost::serialization::make_nvp("m_vcom_ref", m_vcom_ref);
        ar >> boost::serialization::make_nvp("m_forces_ref", m_forces_ref);

        ar >> boost::serialization::make_nvp("lwc",m_lwc);
        ar >> boost::serialization::make_nvp("sowc",m_sowc);
        ar >> boost::serialization::make_nvp("sowc_placement",m_sowc_placement);
        ar >> boost::serialization::make_nvp("double_description",m_double_description);
      }

      BOOST_SERIALIZATION_SPLIT_MEMBER()
    };
  }

  /// \returns the number of inactive patches
  Index numInactivePatches() const { return dim - numActivePatches(); }

  template <typename S2>
  bool operator==(const ContactPhaseTpl<S2, dim> &other) const {
    return m_contact_patches == other.m_contact_patches && m_lwc == other.m_lwc && m_sowc == other.m_sowc;
  }

  template <typename S2>
  bool operator!=(const ContactPhaseTpl<S2, dim> &other) const {
    return !(*this == other);
  }

  ContactPatchArray m_contact_patches;  // TODO: set protected

  ///
  /// \brief Returns the first active patch.
  ///
  /// \remark This method is useful one looks for the lonely active patch.
  ///
  /// \returns the first active patch.
  ///
  const ContactPatch &getActivePatch() const {
    for (typename ContactPatchArray::const_iterator it = m_contact_patches.begin(); it != m_contact_patches.end();
         ++it)
      if (it->active()) return *it;
  }
  ContactPatch &getActivePatch() {
    return const_cast<ContactPatch &>(static_cast<const ContactPhaseTpl *>(this)->getActivePatch());
  }

  ContactPatchVector getActivePatches() {
    ContactPatchVector res;
    res.reserve((size_t)dim);
    for (typename ContactPatchArray::iterator it = m_contact_patches.begin(); it != m_contact_patches.end(); ++it)
      if (it->active()) res.push_back(typename ContactPatchVector::value_type(*it));

    return res;
  }

 protected:
  /// \brief Second Order Wrench Cone (SOWC) representing the Minkoski sum of the patch linear wrench cone.
  SOC6 m_sowc;
  SE3 m_sowc_placement;

  Matrix6x m_double_description;

  /// \brief Linear Wrench Cone (LWC) representing the Minkoski sum of the patch linear wrench cone.
  WrenchCone m_lwc;

 private:
  // Serialization of the class
  friend class boost::serialization::access;

  template <class Archive>
  void save(Archive &ar, const unsigned int /*version*/) const {
    for (typename ContactPatchArray::const_iterator it = m_contact_patches.begin(); it != m_contact_patches.end();
         ++it)
      ar &boost::serialization::make_nvp("contact_patch", *it);

    ar &boost::serialization::make_nvp("lwc", m_lwc);
    ar &boost::serialization::make_nvp("sowc", m_sowc);
    ar &boost::serialization::make_nvp("sowc_placement", m_sowc_placement);
    ar &boost::serialization::make_nvp("double_description", m_double_description);
  }

  template <class Archive>
  void load(Archive &ar, const unsigned int /*version*/) {
    for (typename ContactPatchArray::iterator it = m_contact_patches.begin(); it != m_contact_patches.end(); ++it)
      ar >> boost::serialization::make_nvp("contact_patch", *it);

    ar >> boost::serialization::make_nvp("lwc", m_lwc);
    ar >> boost::serialization::make_nvp("sowc", m_sowc);
    ar >> boost::serialization::make_nvp("sowc_placement", m_sowc_placement);
    ar >> boost::serialization::make_nvp("double_description", m_double_description);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};
}  // namespace scenario
}  // namespace multicontact_api

#endif  // ifndef __multicontact_api_scenario_contact_phase_hpp__
