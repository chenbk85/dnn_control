#include "controllerneuralnetwork.h"
#include "constants.h"
#include "configuration.h"

#include "samplefactory.h"

#if PGMOS_ENABLE_ODOMETRY
const unsigned int ControllerNeuralNetwork::kDimensions = 6;
#else
const unsigned int ControllerNeuralNetwork::kDimensions = PGMOS_ENABLE_OPTICAL_FLOW * 6 + PGMOS_ENABLE_VELOCITY * 3 + PGMOS_ENABLE_VELOCITY_OVER_HEIGHT * 3 + PGMOS_ENABLE_DIRECTION_SENSOR * 3 + PGMOS_ENABLE_ACCELEROMETER * 3;
#endif

ControllerNeuralNetwork::ControllerNeuralNetwork(const double &maximum_thrust, const unsigned int &num_hidden)
    : Controller(kDimensions, maximum_thrust), neural_network_(kDimensions, true, 3, NeuralNetwork::ActivationFunctionType::Linear, {boost::make_tuple(num_hidden, true, NeuralNetwork::ActivationFunctionType::Sigmoid)}) {
    number_of_parameters_ = neural_network_.Size();
}

ControllerNeuralNetwork::ControllerNeuralNetwork(const double &maximum_thrust, const unsigned int &num_hidden, const std::vector<double> &weights)
    : Controller(kDimensions, maximum_thrust), neural_network_(kDimensions, true, 3, NeuralNetwork::ActivationFunctionType::Linear, {boost::make_tuple(num_hidden, true, NeuralNetwork::ActivationFunctionType::Sigmoid)}) {
    number_of_parameters_ = neural_network_.Size();
    SetWeights(weights);
}

void ControllerNeuralNetwork::SetWeights(const std::vector<double> &weights) {
    if (weights.size() == number_of_parameters_) {
        neural_network_.SetWeights(weights);
    } else {
        throw SizeMismatchException();
    }
}

static SampleFactory sf(rand());
Vector3D ControllerNeuralNetwork::GetThrustForSensorData(const std::vector<double> &sensor_data) {
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

    for (unsigned int i = 0; i < 3; ++i) {
        thrust[i] = sf.SampleUniform(-maximum_thrust_, maximum_thrust_);
    }
    return thrust;
}
