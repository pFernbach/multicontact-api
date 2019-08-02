// Copyright (c) 2015-2018, CNRS
// Authors: Justin Carpentier <jcarpent@laas.fr>

#ifndef __multicontact_api_python_scenario_contact_phase_humanoid_hpp__
#define __multicontact_api_python_scenario_contact_phase_humanoid_hpp__

#include <eigenpy/eigenpy.hpp>

#include "multicontact-api/scenario/contact-phase-humanoid.hpp"
#include "multicontact-api/scenario/contact-phase.hpp"
#include "multicontact-api/bindings/python/container/visitor.hpp"
#include "multicontact-api/bindings/python/container/array.hpp"

#include <pinocchio/bindings/python/utils/std-aligned-vector.hpp>

namespace multicontact_api
{
  namespace python
  {

    namespace bp = boost::python;

    template<typename ContactPhase>
    struct ContactPhaseHumanoidPythonVisitor
    : public boost::python::def_visitor< ContactPhaseHumanoidPythonVisitor<ContactPhase> >
    {
      typedef typename ContactPhase::Scalar Scalar;
      typedef typename ContactPhase::Base ContactPhase4;
      typedef typename ContactPhase::ContactPatch ContactPatch;

      typedef typename ContactPhase::StateVector StateVector;
      typedef typename ContactPhase::ControlVector ControlVector;
      typedef typename ContactPhase::ConfigurationVector ConfigurationVector;

      typedef typename ContactPhase::VectorStateVector VectorStateVector;
      typedef typename ContactPhase::VectorControlVector VectorControlVector;
      typedef typename ContactPhase::VectorConfigurationVector VectorConfigurationVector;

      typedef typename ContactPhase::VectorForce VectorForce;
      typedef typename ContactPhase::Force Force;

      template<class PyClass>
      void visit(PyClass & cl) const
      {
        cl
        .def(bp::init<>(bp::arg(""),"Default constructor."))
        .def(bp::init<ContactPhase>(bp::args("other"),"Copy contructor."))
        .add_property("RF_patch",
                      bp::make_function(&getRFpatch,bp::return_internal_reference<>()),
                      bp::make_function(&setRFpatch)
                      )
        .add_property("LF_patch",
                      bp::make_function(&getLFpatch,bp::return_internal_reference<>()),
                      bp::make_function(&setLFpatch)
                      )
        .add_property("RH_patch",
                      bp::make_function(&getRHpatch,bp::return_internal_reference<>()),
                      bp::make_function(&setRHpatch)
                      )
        .add_property("LH_patch",
                      bp::make_function(&getLHpatch,bp::return_internal_reference<>()),
                      bp::make_function(&setLHpatch)
                      )

        .add_property("init_state",&getInitState,&setInitState,"Initial state of the phase.")
        .add_property("final_state",&getFinalState,&setFinalState,"Final state of the phase.")

        .add_property("state_trajectory",
                      bp::make_getter(&ContactPhase::m_state_trajectory,bp::return_internal_reference<>()))
        .add_property("dot_state_trajectory",
                      bp::make_getter(&ContactPhase::m_dot_state_trajectory,bp::return_internal_reference<>()))
        .add_property("control_trajectory",
                      bp::make_getter(&ContactPhase::m_control_trajectory,bp::return_internal_reference<>()))
        .add_property("time_trajectory",
                      bp::make_getter(&ContactPhase::m_time_trajectory,bp::return_internal_reference<>()))
        .add_property("objective_trajectory",
                      bp::make_getter(&ContactPhase::m_objective_trajectory,bp::return_internal_reference<>()))

        .def_readwrite("reference_configurations",&ContactPhase::m_reference_configurations)
        .def_readwrite("raw_control_trajectory",&ContactPhase::m_raw_control_trajectory)
        .def_readwrite("angular_momentum_ref",&ContactPhase::m_angular_momentum_ref)
        .def_readwrite("com_ref",&ContactPhase::m_com_ref)
        .def_readwrite("vcom_ref",&ContactPhase::m_vcom_ref)
        .def_readwrite("forces_ref",&ContactPhase::m_forces_ref)
        .add_property("contact_forces_trajectories", bp::make_array(&ContactPhase::m_contact_forces_trajectories))

        .add_property("RF_trajectory",
                      bp::make_getter(&ContactPhase::m_RF_trajectory,bp::return_internal_reference<>()))
        .add_property("LF_trajectory",
                      bp::make_getter(&ContactPhase::m_LF_trajectory,bp::return_internal_reference<>()))
        .add_property("RH_trajectory",
                      bp::make_getter(&ContactPhase::m_RH_trajectory,bp::return_internal_reference<>()))
        .add_property("LH_trajectory",
                      bp::make_getter(&ContactPhase::m_LH_trajectory,bp::return_internal_reference<>()))

        .def(bp::self == bp::self)
        .def(bp::self != bp::self)

        .def("copy",&copy,"Returns a copy of *this.")
        ;
      }

      static void expose(const std::string & class_name)
      {
        std::string doc = "Contact Phase for a Humanoid robot definied by its 4 end effectors.";
        bp::class_< ContactPhase,bp::bases<ContactPhase4> >(class_name.c_str(),
                                                            doc.c_str(),
                                                            bp::no_init)
        .def(ContactPhaseHumanoidPythonVisitor<ContactPhase>())
        ;

        // Expose related quantities
        eigenpy::enableEigenPySpecific<StateVector,StateVector>();
        eigenpy::enableEigenPySpecific<ConfigurationVector,ConfigurationVector>();

        VectorPythonVisitor<VectorStateVector,true>::expose("VectorStateVector");
        VectorPythonVisitor<VectorControlVector,true>::expose("VectorControlVector");
        VectorPythonVisitor<VectorConfigurationVector,true>::expose("VectorConfigurationVector");
        VectorPythonVisitor<VectorForce,true>::expose("VectorForce");

      }

    protected:

      static ContactPatch & getRFpatch(ContactPhase & self) { return self.RF_patch; }
      static void setRFpatch(ContactPhase & self, const ContactPatch & contact_patch)
      { self.RF_patch = contact_patch; }

      static ContactPatch & getLFpatch(ContactPhase & self) { return self.LF_patch; }
      static void setLFpatch(ContactPhase & self, const ContactPatch & contact_patch)
      { self.LF_patch = contact_patch; }


      static ContactPatch & getRHpatch(ContactPhase & self) { return self.RH_patch; }
      static void setRHpatch(ContactPhase & self, const ContactPatch & contact_patch)
      { self.RH_patch = contact_patch; }


      static ContactPatch & getLHpatch(ContactPhase & self) { return self.LH_patch; }
      static void setLHpatch(ContactPhase & self, const ContactPatch & contact_patch)
      { self.LH_patch = contact_patch; }

      static StateVector getInitState(const ContactPhase & self) { return self.m_init_state; }
      static void setInitState(ContactPhase & self, const StateVector & init_state)
      { self.m_init_state = init_state; }

      static StateVector getFinalState(const ContactPhase & self) { return self.m_final_state; }
      static void setFinalState(ContactPhase & self, const StateVector & final_state)
      { self.m_final_state = final_state; }

      static ContactPhase copy(const ContactPhase & self) { return ContactPhase(self); }
    };
  }
}


#endif // ifndef __multicontact_api_python_scenario_contact_phase_humanoid_hpp__
