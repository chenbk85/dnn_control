#include "odesystem.h"
#include "utility.h"
#include <math.h>

ODESystem::ODESystem(Asteroid &asteroid) : asteroid_(asteroid) {
    coef_earth_acceleration_mul_specific_impulse_ = 0.0;
    for (int i = 0; i < 3; ++i) {
        thrust_[i] = 0.0;
        perturbations_acceleration_[i] = 0.0;
        state_[i] = 0.0;
        state_[3+i] = 0.0;
    }
    coef_earth_acceleration_mul_specific_impulse_ = 0.0;
}

void ODESystem::operator()(const State &state, State &d_state_dt, double time) const
{
    double gravity_acceleration[3];
    double thrust_acceleration[3];
    double euler_acceleration[3];
    double centrifugal_acceleration[3];
    double coriolis_acceleration[3];
    double angular_velocity[3];
    double angular_acceleration[3];
    double tmp[3];

    const double coef_mass = 1.0 / state[6];

    double position[3];
    double velocity[3];
    for(int i = 0; i < 3; ++i) {
        position[i] = state[i];
        velocity[i] = state[3+i];
    }

    asteroid_.GravityAtPosition(position,gravity_acceleration);
    asteroid_.AngularVelocityAndAccelerationAtTime(time, angular_velocity, angular_acceleration);

    for(int i = 0; i < 3; ++i) {
        gravity_acceleration[i] *= coef_mass;
        thrust_acceleration[i] = thrust_[i] * coef_mass;
    }


    CrossProduct(angular_acceleration, position, euler_acceleration);
    CrossProduct(angular_velocity, position, tmp);
    CrossProduct(angular_velocity, tmp, centrifugal_acceleration);

    for(int i = 0; i < 3; ++i) {
        tmp[i] = angular_velocity[i] * 2.0;
    }

    CrossProduct(tmp, velocity, coriolis_acceleration);

    for (int i = 0; i < 3 ;++i) {
        d_state_dt[i] = state[3+i];
        d_state_dt[3+i] = perturbations_acceleration_[i] + gravity_acceleration[i] + thrust_acceleration[i]
                - coriolis_acceleration[i] - euler_acceleration[i] - centrifugal_acceleration[i];
    }

    d_state_dt[6] = sqrt(thrust_[0] * thrust_[0] + thrust_[1] * thrust_[1] + thrust_[2] * thrust_[2]) * coef_earth_acceleration_mul_specific_impulse_;
}
