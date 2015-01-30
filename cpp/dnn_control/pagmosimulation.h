#ifndef PAGMOSIMULATION_H
#define PAGMOSIMULATION_H

#include "vector.h"
#include "asteroid.h"
#include "systemstate.h"

#include <boost/tuple/tuple.hpp>

#define PS_TEST_FOR_ORBIT   0

class PaGMOSimulation {
public:
    PaGMOSimulation(const unsigned int &random_seed, const double &simulation_time);
    virtual ~PaGMOSimulation();

    virtual boost::tuple<std::vector<double>, std::vector<double>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D> > EvaluateAdaptive() = 0;

    virtual boost::tuple<std::vector<double>, std::vector<double>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D> > EvaluateFixed() = 0;


    double FixedStepSize() const;

    double MinimumStepSize() const;

    double InteractionInterval() const;

    Asteroid& AsteroidOfSystem();

    double SpacecraftMass() const;

    double SpacecraftMinimumMass() const;

    class Exception {};
    class SizeMismatchException : public Exception {};

protected:
    class Observer {
    public:
        Observer(double &time) : time_(time){}
        void operator () (const SystemState &, const double &current_time) {
            time_ = current_time;
        }
    private:
        double &time_;
    };

    void Init();

    unsigned int random_seed_;

    double simulation_time_;

    double interaction_interval_;

    double minimum_step_size_;

    double fixed_step_size_;

    double spacecraft_engine_noise_;

    double spacecraft_specific_impulse_;

    double spacecraft_mass_;

    double spacecraft_minimum_mass_;

    double spacecraft_maximum_thrust_;

    double perturbation_mean_;

    double perturbation_noise_;

    Asteroid asteroid_;

    SystemState initial_system_state_;

    Vector3D target_position_;

    std::vector<double> simulation_parameters_;
};

#endif // PAGMOSIMULATION_H