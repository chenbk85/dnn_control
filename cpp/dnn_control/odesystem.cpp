#include "odesystem.h"
#include "constants.h"
#include "configuration.h"

ODESystem::ODESystem(const Asteroid &asteroid, const Vector3D &perturbations_acceleration, const Vector3D &thrust, const double &spacecraft_specific_impulse, const double &spacecraft_minimum_mass, const double &engine_noise, const bool &fuel_usage_enabled)
    : asteroid_(asteroid) {
    fuel_usage_enabled_ = fuel_usage_enabled;
    perturbations_acceleration_ = perturbations_acceleration;
    thrust_ = thrust;
    spacecraft_specific_impulse_ = spacecraft_specific_impulse;
    spacecraft_minimum_mass_ = spacecraft_minimum_mass;
    engine_noise_ = engine_noise;
}

ODESystem::ODESystem(const ODESystem &other)
    : asteroid_(other.asteroid_) {
    fuel_usage_enabled_ = other.fuel_usage_enabled_;
    perturbations_acceleration_ = other.perturbations_acceleration_;
    thrust_ = other.thrust_;
    spacecraft_specific_impulse_ = other.spacecraft_specific_impulse_;
    spacecraft_minimum_mass_ = other.spacecraft_minimum_mass_;
    engine_noise_ = other.engine_noise_;
}

void ODESystem::operator ()(const SystemState &state, SystemState &d_state_dt, const double &time) {
    const double mass = state[6];
    // check if spacecraft is out of fuel
    if (mass <= spacecraft_minimum_mass_) {
        throw OutOfFuelException();
    }

    // 1/m
    const double coef_mass = 1.0 / mass;

    const Vector3D &position = {state[0], state[1], state[2]};
    const Vector3D &velocity = {state[3], state[4], state[5]};

    // Fg
    const Vector3D gravity_acceleration = asteroid_.GravityAccelerationAtPosition(position);

    // w, w'
    const boost::tuple<Vector3D, Vector3D> result = asteroid_.AngularVelocityAndAccelerationAtTime(time);
    const Vector3D &angular_velocity = boost::get<0>(result);
    const Vector3D &angular_acceleration = boost::get<1>(result);

    // Fc
    const Vector3D thrust_acceleration = VectorMul(coef_mass, thrust_);

    // w' x r
    const Vector3D euler_acceleration = VectorCrossProduct(angular_acceleration, position);

    // w x (w x r)
    const Vector3D centrifugal_acceleration = VectorCrossProduct(angular_velocity, VectorCrossProduct(angular_velocity, position));

    // 2w x r'
    const Vector3D coriolis_acceleration = VectorCrossProduct(VectorMul(2.0, angular_velocity), velocity);

    for (unsigned int i = 0; i < 3 ;++i) {
        d_state_dt[i] = state[3+i];
        d_state_dt[3+i] = perturbations_acceleration_[i]
                + gravity_acceleration[i]
                + thrust_acceleration[i]
                - coriolis_acceleration[i]
                - euler_acceleration[i]
                - centrifugal_acceleration[i];
    }

    if (fuel_usage_enabled_) {
        d_state_dt[6] = -VectorNorm(thrust_) / ((spacecraft_specific_impulse_ + spacecraft_specific_impulse_ * engine_noise_) * kEarthAcceleration);
    } else {
        d_state_dt[6] = 0.0;
    }
}

