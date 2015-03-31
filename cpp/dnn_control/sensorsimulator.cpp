#include "sensorsimulator.h"
#include "configuration.h"

#include <sstream>

const std::map<SensorSimulator::SensorType, std::pair<unsigned int, double> > SensorSimulator::SensorTypeConfigurations = {
    {SensorSimulator::SensorType::RelativePosition, {3, 0.05}},
    {SensorSimulator::SensorType::Velocity, {3, 0.05}},
    {SensorSimulator::SensorType::OpticalFlow, {6, 0.05}},
    {SensorSimulator::SensorType::Acceleration, {3, 0.05}}
};

SensorSimulator::SensorSimulator(SampleFactory &sample_factory, const Asteroid &asteroid, const std::set<SensorType> &sensor_types, const bool &enable_noise, const Vector3D &target_position, const std::vector<std::pair<double, double> > &sensor_value_transformations)
    : sample_factory_(sample_factory), asteroid_(asteroid), sensor_types(sensor_types), noise_enabled_(enable_noise), target_position_(target_position), sensor_value_transformations_(sensor_value_transformations) {
    dimensions_ = 0;
    for (auto t : sensor_types) {
        dimensions_ += SensorTypeConfigurations.at(t).first;
    }
}

unsigned int SensorSimulator::Dimensions() const {
    return dimensions_;
}

std::string SensorSimulator::SensorDataToString(const std::vector<double> &data) {
    std::stringstream result;
    if (data.size()) {
        result << data[0];
    }
    for (unsigned int i = 1; i < data.size(); ++i) {
        result << ",\t" << data[i];
    }
    return result.str();
}

double SensorSimulator::AddNoise(const double &sensor_value, const double &standard_deviation) {
    if (noise_enabled_) {
        return sensor_value + sensor_value * sample_factory_.SampleNormal(0.0, standard_deviation);
    } else {
        return sensor_value;
    }
}

std::vector<double> SensorSimulator::Simulate(const SystemState &state, const Vector3D &height, const Vector3D &perturbations_acceleration, const double &time) {
    std::vector<double> sensor_data(dimensions_, 0.0);
    unsigned int offset = 0;

    for (auto t : sensor_types) {
        if (t == RelativePosition) {
            const Vector3D &position = {state[0], state[1], state[2]};
            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset++] = AddNoise(target_position_[i] - position[i], SensorTypeConfigurations.at(t).second);
            }
        } else if (t == Velocity) {
            const Vector3D &velocity = {state[3], state[4], state[5]};
            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset++] = AddNoise(velocity[i], SensorTypeConfigurations.at(t).second);
            }
        } else if (t == OpticalFlow) {
            const double up_scale = 1000000.0;
            const Vector3D &velocity = {state[3], state[4], state[5]};

            const double coef_norm_height = 1.0 / VectorNorm(height);
            const Vector3D &normalized_height = {height[0] * coef_norm_height, height[1] * coef_norm_height, height[2] * coef_norm_height};
            const double magn_velocity_parallel = VectorDotProduct(velocity, normalized_height);
            const Vector3D &velocity_parallel = {magn_velocity_parallel * normalized_height[0], magn_velocity_parallel * normalized_height[1], magn_velocity_parallel * normalized_height[2]};
            const Vector3D velocity_perpendicular = VectorSub(velocity, velocity_parallel);

            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset++] = AddNoise(up_scale * velocity_parallel[i] * coef_norm_height, SensorTypeConfigurations.at(t).second);
            }
            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset++] = AddNoise(up_scale * velocity_perpendicular[i] * coef_norm_height, SensorTypeConfigurations.at(t).second);
            }
        } else if (t == Acceleration) {
            const double up_scale = 1000000.0;
            const Vector3D &position = {state[0], state[1], state[2]};
            const Vector3D &velocity = {state[3], state[4], state[5]};

            const Vector3D gravity_acceleration = asteroid_.GravityAccelerationAtPosition(position);

            const boost::tuple<Vector3D, Vector3D> result_angular = asteroid_.AngularVelocityAndAccelerationAtTime(time);
            const Vector3D &angular_velocity = boost::get<0>(result_angular);
            const Vector3D &angular_acceleration = boost::get<1>(result_angular);

            const Vector3D euler_acceleration = VectorCrossProduct(angular_acceleration, position);
            const Vector3D centrifugal_acceleration = VectorCrossProduct(angular_velocity, VectorCrossProduct(angular_velocity, position));

            const Vector3D coriolis_acceleration = VectorCrossProduct(VectorMul(2.0, angular_velocity), velocity);

            for (unsigned int i = 0; i < 3; ++i) {
                double sensor_value = perturbations_acceleration[i]
                        + gravity_acceleration[i]
                        - coriolis_acceleration[i]
                        - euler_acceleration[i]
                        - centrifugal_acceleration[i];
                sensor_value *= up_scale;

                sensor_data[offset++] = AddNoise(sensor_value, SensorTypeConfigurations.at(t).second);
            }
        }
    }

    if (sensor_value_transformations_.size()) {

    }
    return sensor_data;
}

double SensorSimulator::TransformValue(const double &sensor_value, const std::pair<double, double> &transformation_params) {

}
