#include "pagmosimulationfullstate.h"
#include "utility.h"
#include "datacollector.h"
#include "odeint.h"
#include "modifiedcontrolledrungekutta.h"

PaGMOSimulationFullState::PaGMOSimulationFullState(const PaGMOSimulationFullState &other) {
    random_seed_ = other.random_seed_;
    simulation_time_ = other.simulation_time_;
    minimum_step_size_ = other.minimum_step_size_;
    fixed_step_size_ = other.fixed_step_size_;
    engine_noise_ = other.engine_noise_;
    perturbation_noise_ = other.perturbation_noise_;
    spacecraft_specific_impulse_ = other.spacecraft_specific_impulse_;
    sample_factory_ = other.sample_factory_;
    asteroid_ = other.asteroid_;
    if (other.sensor_simulator_ != NULL) {
        sensor_simulator_ = new SensorSimulatorFullState(sample_factory_, *other.sensor_simulator_);
    } else {
        sensor_simulator_ = NULL;
    }
    if (other.controller_ != NULL) {
        controller_ = new ControllerFullState(*other.controller_);
    } else {
        controller_ = NULL;
    }
    initial_system_state_ = other.initial_system_state_;
}

PaGMOSimulationFullState::PaGMOSimulationFullState(const unsigned int &random_seed, const double &simulation_time) : random_seed_(random_seed),  simulation_time_(simulation_time), sample_factory_(SampleFactory(random_seed)) {
    Init();
}

PaGMOSimulationFullState::PaGMOSimulationFullState(const unsigned int &random_seed, const double &simulation_time, const std::vector<double> &pid_coefficients) : random_seed_(random_seed),  simulation_time_(simulation_time), sample_factory_(SampleFactory(random_seed)) {
    Init();
    controller_->SetCoefficients(pid_coefficients[0], pid_coefficients[1], pid_coefficients[2]);
}

PaGMOSimulationFullState::~PaGMOSimulationFullState() {
    if (controller_ != NULL) {
        delete controller_;
    }
    if (sensor_simulator_ != NULL) {
        delete sensor_simulator_;
    }
}

PaGMOSimulationFullState& PaGMOSimulationFullState::operator=(const PaGMOSimulationFullState &other) {
    if (&other != this) {
        random_seed_ = other.random_seed_;
        simulation_time_ = other.simulation_time_;
        minimum_step_size_ = other.minimum_step_size_;
        fixed_step_size_ = other.fixed_step_size_;
        engine_noise_ = other.engine_noise_;
        perturbation_noise_ = other.perturbation_noise_;
        spacecraft_specific_impulse_ = other.spacecraft_specific_impulse_;
        sample_factory_ = other.sample_factory_;
        asteroid_ = other.asteroid_;
        if (sensor_simulator_ != NULL) {
            delete sensor_simulator_;
        }
        if (other.sensor_simulator_ != NULL) {
            sensor_simulator_ = new SensorSimulatorFullState(sample_factory_, *other.sensor_simulator_);
        } else {
            sensor_simulator_ = NULL;
        }
        if (controller_ != NULL) {
            delete controller_;
        }
        if (other.controller_ != NULL) {
            controller_ = new ControllerFullState(*other.controller_);
        } else {
            controller_ = NULL;
        }
        initial_system_state_ = other.initial_system_state_;
    }
    return *this;
}

boost::tuple<std::vector<double>, std::vector<double>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D> > PaGMOSimulationFullState::Evaluate() {
    sample_factory_.SetSeed(random_seed_);

    std::vector<double> time_points;
    std::vector<double> evaluated_masses;
    std::vector<Vector3D> evaluated_positions;
    std::vector<Vector3D> evaluated_velocities;
    std::vector<Vector3D> evaluated_heights;

    DataCollector collector(asteroid_, time_points, evaluated_masses, evaluated_positions, evaluated_heights, evaluated_velocities, false);
    SystemState system_state(initial_system_state_);

    typedef odeint::runge_kutta_cash_karp54<SystemState> ErrorStepper;
    typedef odeint::modified_controlled_runge_kutta<ErrorStepper> ControlledStepper;
    ControlledStepper controlled_stepper;

    ODESystem sys(sample_factory_, asteroid_, sensor_simulator_, controller_, spacecraft_specific_impulse_, perturbation_noise_, engine_noise_);

    try {
        integrate_adaptive(controlled_stepper , sys, system_state, 0.0, simulation_time_, minimum_step_size_, collector);
    } catch (const Asteroid::Exception &exception) {
        //std::cout << "The spacecraft crashed into the asteroid's surface." << std::endl;
    } catch (const ODESystem::Exception &exception) {
        //std::cout << "The spacecraft is out of fuel." << std::endl;
    }

    return boost::make_tuple(time_points, evaluated_masses, evaluated_positions, evaluated_heights, evaluated_velocities);
}

boost::tuple<std::vector<double>, std::vector<double>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D> > PaGMOSimulationFullState::EvaluateDetailed() {
    sample_factory_.SetSeed(random_seed_);

    std::vector<double> time_points;
    std::vector<double> evaluated_masses;
    std::vector<Vector3D> evaluated_positions;
    std::vector<Vector3D> evaluated_velocities;
    std::vector<Vector3D> evaluated_heights;

    DataCollector collector(asteroid_, time_points, evaluated_masses, evaluated_positions, evaluated_heights, evaluated_velocities);
    SystemState system_state(initial_system_state_);

    ODESystem sys(sample_factory_, asteroid_, sensor_simulator_, controller_, spacecraft_specific_impulse_, perturbation_noise_, engine_noise_);

    odeint::runge_kutta4<SystemState> stepper;
    try {
        integrate_const(stepper , sys, system_state, 0.0, simulation_time_, fixed_step_size_, collector);
    } catch (const Asteroid::Exception &exception) {
        //std::cout << "The spacecraft crashed into the asteroid's surface." << std::endl;
    } catch (const ODESystem::Exception &exception) {
        //std::cout << "The spacecraft is out of fuel." << std::endl;
    }

    return boost::make_tuple(time_points, evaluated_masses, evaluated_positions, evaluated_heights, evaluated_velocities);
}

double PaGMOSimulationFullState::FixedStepSize() const {
    return fixed_step_size_;
}

double PaGMOSimulationFullState::MinimumStepSize() const {
    return minimum_step_size_;
}

Asteroid& PaGMOSimulationFullState::AsteroidOfSystem() {
    return asteroid_;
}

void PaGMOSimulationFullState::Init() {
    minimum_step_size_ = 0.1;
    fixed_step_size_ = 0.1;

    const Vector3D semi_axis = {sample_factory_.SampleUniform(8000.0, 12000.0), sample_factory_.SampleUniform(4000.0, 7500.0), sample_factory_.SampleUniform(1000.0, 3500.0)};
    const double density = sample_factory_.SampleUniform(1500.0, 3000.0);
    Vector2D angular_velocity_xz = {sample_factory_.SampleSign() * sample_factory_.SampleUniform(0.0002, 0.0008), sample_factory_.SampleSign() * sample_factory_.SampleUniform(0.0002, 0.0008)};
    const double time_bias = sample_factory_.SampleUniform(0.0, 12.0 * 60 * 60);
    asteroid_ = Asteroid(semi_axis, density, angular_velocity_xz, time_bias);

    const double spacecraft_mass = sample_factory_.SampleUniform(450.0, 500.0);
    const double spacecraft_maximum_thrust = 21.0;
    spacecraft_specific_impulse_ = 200.0;


    // orbit
    //const Vector3D spacecraft_position = {sample_factory_.SampleUniform(4.0, 6.0) * semi_axis[0], 0.0, 0.0};

    // random
    const Vector3D spacecraft_position = sample_factory_.SamplePointOutSideEllipsoid(semi_axis, 1.1, 4.0);

    const Vector3D angular_velocity = boost::get<0>(asteroid_.AngularVelocityAndAccelerationAtTime(0.0));
    Vector3D spacecraft_velocity = VectorCrossProduct(angular_velocity, spacecraft_position);

    // orbit
    //const double norm_position = VectorNorm(spacecraft_position);
    //spacecraft_velocity[0] = -spacecraft_velocity[0]; spacecraft_velocity[1] = -spacecraft_velocity[1] + sqrt(asteroid_.MassGravitationalConstant() / norm_position); spacecraft_velocity[2] = -spacecraft_velocity[2];

    // no velocity
    spacecraft_velocity[0] *= -1; spacecraft_velocity[1] *= -1; spacecraft_velocity[2] *= -1;

    SensorNoiseConfiguration sensor_noise(SensorSimulatorFullState::kDimensions, 0.0);
    for (unsigned int i = 0; i < sensor_noise.size(); ++i) {
        sensor_noise[i] = 0.05;
    }
    sensor_simulator_ = new SensorSimulatorFullState(sample_factory_, asteroid_, sensor_noise);

    controller_ = new ControllerFullState(spacecraft_maximum_thrust, spacecraft_position);

    perturbation_noise_ = 1e-7;
    engine_noise_ = 0.05;

    for (unsigned int i = 0; i < 3; ++i) {
        initial_system_state_[i] = spacecraft_position[i];
        initial_system_state_[3+i] = spacecraft_velocity[i];
    }
    initial_system_state_[6] = spacecraft_mass;

    if (sensor_simulator_->Dimensions() != controller_->Dimensions()) {
        throw SizeMismatchException();
    }
}
