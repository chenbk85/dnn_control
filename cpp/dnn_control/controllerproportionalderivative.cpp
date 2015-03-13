#include "controllerproportionalderivative.h"
#include "configuration.h"

#if PGMOS_ENABLE_ODOMETRY
const unsigned int ControllerProportionalDerivative::kDimensions = 6;
#else
const unsigned int ControllerProportionalDerivative::kDimensions = PGMOS_ENABLE_OPTICAL_FLOW * 3 + PGMOS_ENABLE_DIRECTION_SENSOR * 3 + PGMOS_ENABLE_ACCELEROMETER * 3;
#endif

ControllerProportionalDerivative::ControllerProportionalDerivative(const double &maximum_thrust)
    : Controller(kDimensions, maximum_thrust), neural_network_(kDimensions, false, {boost::make_tuple(3, false, NeuralNetwork::ActivationFunctionType::Linear)}) {
    number_of_parameters_ = neural_network_.Size();
}

ControllerProportionalDerivative::ControllerProportionalDerivative(const double &maximum_thrust, const std::vector<double> &pd_coefficients)
    : Controller(kDimensions, maximum_thrust), neural_network_(kDimensions, false, {boost::make_tuple(3, false, NeuralNetwork::ActivationFunctionType::Linear)}) {
    number_of_parameters_ = neural_network_.Size();
    SetCoefficients(pd_coefficients);
}

void ControllerProportionalDerivative::SetCoefficients(const std::vector<double> &pd_coefficients) {
    if (pd_coefficients.size() == number_of_parameters_) {
        neural_network_.SetWeights(pd_coefficients);
    } else {
        throw SizeMismatchException();
    }
}

Vector3D ControllerProportionalDerivative::GetThrustForSensorData(const std::vector<double> &sensor_data) {
#if CPD_ENABLE_CORRECT_THRUST_OUTPUT
    const std::vector<double> unboxed_thrust = neural_network_.Evaluate(sensor_data);
    Vector3D thrust = {unboxed_thrust[0], unboxed_thrust[1], unboxed_thrust[2]};
    const double norm = VectorNorm(thrust);
    if (norm > maximum_thrust_) {
        const double coef_norm = 1.0 / norm;
        for (unsigned int i = 0; i < 3; ++i) {
            thrust[i] *= coef_norm * maximum_thrust_;
        }
    }
    return thrust;
#else
    const std::vector<double> unboxed_thrust = neural_network_.Evaluate(sensor_data);
    Vector3D thrust;
    for (unsigned int i = 0; i < 3; ++i) {
        double t = unboxed_thrust[i];
        if (t > maximum_thrust_) {
            t = maximum_thrust_;
        } else if (t < -maximum_thrust_) {
            t = -maximum_thrust_;
        }
        thrust[i] = t;
    }
    return thrust;
#endif
}
