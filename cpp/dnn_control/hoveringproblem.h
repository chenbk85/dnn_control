#ifndef HOVERINGPROBLEM_H
#define HOVERINGPROBLEM_H

#include <pagmo/src/problem/base_stochastic.h>
#include <boost/serialization/access.hpp>

namespace pagmo { namespace problem {

class __PAGMO_VISIBLE hovering_problem : public base_stochastic {
public:
    // Make sure it matches the sensor's class "kDimensions" constant
    static const unsigned int n_sensor_dimensions = 3;

    // Make sure it machtes the output of the controller's "GetThrustForSensorData" function
    static const unsigned int n_control_dimensions = 3;

    hovering_problem(const unsigned int &seed=0, const unsigned int &n_evaluations=5, const double &simulation_time=60.0, const unsigned int &n_hidden_neurons=10);

    hovering_problem(const hovering_problem &other);

    ~hovering_problem();

    std::string get_name() const;

    base_ptr clone() const;

protected:
    void objfun_impl(fitness_vector &f, const decision_vector &x) const;

    std::string human_readable_extra() const;

private:
    unsigned int m_n_evaluations;
    unsigned int m_n_hidden_neurons;
    double m_simulation_time;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int) {
        ar & boost::serialization::base_object<base_stochastic>(*this);
        ar & m_n_evaluations;
        ar & m_n_hidden_neurons;
        ar & m_simulation_time;
    }
};

}} //namespaces

BOOST_CLASS_EXPORT_KEY(pagmo::problem::hovering_problem)

#endif // HOVERINGPROBLEM_H