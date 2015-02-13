#include "controllerfullstate.h"

const unsigned int ControllerFullState::kDimensions = 7;

ControllerFullState::ControllerFullState(const double &maximum_thrust, const Vector3D &target_position)
#if CFS_ENABLE_INTEGRATION
    : Controller(kDimensions, (kDimensions - 1) * 3, maximum_thrust) {
#else
    : Controller(kDimensions, (kDimensions - 1) * 2, maximum_thrust) {
#endif

    target_state_ = std::vector<double>(dimensions_ - 1, 0.0);
    for (unsigned int i  = 0; i < 3; ++i) {
        target_state_[i] = target_position[i];
    }

    previous_error_ = std::vector<double>(dimensions_ - 1, 0.0);
#if CFS_ENABLE_INTEGRATION
    integral_ = std::vector<double>(dimensions_ - 1, 0.0);
#endif
    pid_coefficients_ = std::vector<double>(number_of_parameters_, 0.0);

    latest_control_action_ = 0.0;
}

ControllerFullState::~ControllerFullState() {

}

void ControllerFullState::SetCoefficients(const std::vector<double> &pid_coefficients) {
    if (pid_coefficients.size() != number_of_parameters_) {
        throw SizeMismatchException();
    }
    pid_coefficients_ = pid_coefficients;
}

Vector3D ControllerFullState::GetThrustForSensorData(const SensorData &sensor_data) {
    Vector3D thrust = {0.0, 0.0, 0.0};
    const double control_interval = sensor_data[dimensions_ - 1] - latest_control_action_;

    for (unsigned int i = 0; i < dimensions_ - 1; ++i) {
        const double error = target_state_[i] - sensor_data[i];
        double derivative = 0.0;
        if (control_interval > 0.0) {
            derivative = (error - previous_error_[i]) / control_interval;

#if CFS_ENABLE_INTEGRATION
            integral_[i] += error * control_interval;
#endif

        }

#if CFS_ENABLE_INTEGRATION
        const unsigned int base = i * 3;
        thrust[i % 3] += pid_coefficients_.at(base) * error + pid_coefficients_.at(base + 1) * integral_[i] + pid_coefficients_.at(base + 2) * derivative;
#else
        const unsigned int base = i * 2;
        thrust[i % 3] += pid_coefficients_.at(base) * error + pid_coefficients_.at(base + 1) * derivative;
#endif
        previous_error_[i] = error;
    }

    for (unsigned int i = 0; i < 3; ++i) {
        double &t = thrust[i];
        if (t > maximum_thrust_) {
            t = maximum_thrust_;
        } else if (t < -maximum_thrust_) {
            t = -maximum_thrust_;
        }
    }
    latest_control_action_ = sensor_data[dimensions_ - 1];

    return thrust;
}
