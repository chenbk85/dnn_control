#include "sensorsimulator.h"
#include "configuration.h"

#include <sstream>

const std::map<SensorSimulator::SensorType, std::pair<unsigned int, double> > SensorSimulator::SensorTypeConfigurations = {
    {SensorSimulator::SensorType::RelativePosition, {3, 0.05}},
    {SensorSimulator::SensorType::Velocity, {3, 0.05}},
    {SensorSimulator::SensorType::OpticalFlow, {6, 0.05}},
    {SensorSimulator::SensorType::ExternalAcceleration, {1, 0.05}},
    {SensorSimulator::SensorType::TotalAcceleration, {3, 0.05}},
    {SensorSimulator::SensorType::Height, {1, 0.05}},
    {SensorSimulator::SensorType::Mass, {1, 0.05}}
};

SensorSimulator::SensorSimulator(SampleFactory &sample_factory, const Asteroid &asteroid, const std::set<SensorType> &sensor_types_, const bool &enable_noise, const Vector3D &target_position, const std::map<SensorType, std::vector<std::pair<double, double> > > &sensor_value_transformations)
    : sample_factory_(sample_factory), asteroid_(asteroid), sensor_types_(sensor_types_), noise_enabled_(enable_noise), target_position_(target_position), sensor_value_transformations_(sensor_value_transformations) {
    dimensions_ = 0;
    for (auto t : sensor_types_) {
        dimensions_ += SensorTypeConfigurations.at(t).first;
    }
}

unsigned int SensorSimulator::Dimensions() const {
    return dimensions_;
}

double SensorSimulator::AddNoise(const double &sensor_value, const SensorType &type) {
    if (noise_enabled_) {
        const double &standard_deviation = SensorTypeConfigurations.at(type).second;
        return sensor_value + sensor_value * sample_factory_.SampleNormal(0.0, standard_deviation);
    } else {
        return sensor_value;
    }
}

std::vector<double> SensorSimulator::Simulate(const SystemState &state, const Vector3D &height, const Vector3D &perturbations_acceleration, const double &time, const Vector3D &thrust) {
    std::vector<double> sensor_data(dimensions_, 0.0);
    unsigned int offset = 0;

    for (auto t : sensor_types_) {
        if (t == RelativePosition) {
            const Vector3D &position = {state[0], state[1], state[2]};
            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset + i] = AddNoise(target_position_[i] - position[i], t);
            }
            if (sensor_value_transformations_.find(t) != sensor_value_transformations_.end()) {
                const std::vector<std::pair<double, double> > &transformations = sensor_value_transformations_.at(t);
                for (unsigned int i = 0; i < 3; ++i) {
                    sensor_data[offset + i] = TransformValue(sensor_data[offset + i], transformations.at(i));
                }
            }

        } else if (t == Velocity) {
            const Vector3D &velocity = {state[3], state[4], state[5]};
            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset + i] = AddNoise(velocity[i], t);
            }
            if (sensor_value_transformations_.find(t) != sensor_value_transformations_.end()) {
                const std::vector<std::pair<double, double> > &transformations = sensor_value_transformations_.at(t);
                for (unsigned int i = 0; i < 3; ++i) {
                    sensor_data[offset + i] = TransformValue(sensor_data[offset + i], transformations.at(i));
                }
            }

        } else if (t == OpticalFlow) {
            const double up_scale = 1000000.0;
            const Vector3D &velocity = {state[3], state[4], state[5]};

            const double coef_norm_height = 1.0 / VectorNorm(height);
            const Vector3D normalized_height = VectorMul(coef_norm_height, height);
            const double magn_velocity_parallel = VectorDotProduct(velocity, normalized_height);
            const Vector3D velocity_parallel = VectorMul(magn_velocity_parallel, normalized_height);
            const Vector3D velocity_perpendicular = VectorSub(velocity, velocity_parallel);

            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset + i] = AddNoise(up_scale * velocity_parallel[i] * coef_norm_height, t);
            }
            for (unsigned int i = 0; i < 3; ++i) {
                sensor_data[offset + 3 + i] = AddNoise(up_scale * velocity_perpendicular[i] * coef_norm_height, t);
            }

            if (sensor_value_transformations_.find(t) != sensor_value_transformations_.end()) {
                const std::vector<std::pair<double, double> > &transformations = sensor_value_transformations_.at(t);
                for (unsigned int i = 0; i < 6; ++i) {
                    sensor_data[offset + i] = TransformValue(sensor_data[offset + i], transformations.at(i));
                }
            }

        } else if (t == ExternalAcceleration || t == TotalAcceleration) {
            const double up_scale = 100000.0;
            const Vector3D &position = {state[0], state[1], state[2]};
            const Vector3D &velocity = {state[3], state[4], state[5]};
            const double &mass = state[6];

            const Vector3D gravity_acceleration = asteroid_.GravityAccelerationAtPosition(position);

            const boost::tuple<Vector3D, Vector3D> result_angular = asteroid_.AngularVelocityAndAccelerationAtTime(time);
            const Vector3D &angular_velocity = boost::get<0>(result_angular);
            const Vector3D &angular_acceleration = boost::get<1>(result_angular);

            const Vector3D euler_acceleration = VectorCrossProduct(angular_acceleration, position);
            const Vector3D centrifugal_acceleration = VectorCrossProduct(angular_velocity, VectorCrossProduct(angular_velocity, position));

            const Vector3D coriolis_acceleration = VectorCrossProduct(VectorMul(2.0, angular_velocity), velocity);

            Vector3D acceleration;
            for (unsigned int i = 0; i < 3; ++i) {
                double sensor_value = perturbations_acceleration[i]
                        + gravity_acceleration[i]
                        - coriolis_acceleration[i]
                        - euler_acceleration[i]
                        - centrifugal_acceleration[i];

                if (t == TotalAcceleration) {
                    sensor_value += thrust[i] / mass;
                }

                acceleration[i] = AddNoise(sensor_value, t);
            }

            const double coef_norm_height = 1.0 / VectorNorm(height);
            const Vector3D normalized_height = VectorMul(coef_norm_height, height);
            const double magn_acceleration_parallel = VectorDotProduct(acceleration, normalized_height);

            sensor_data[offset] = up_scale * (magn_acceleration_parallel < 0.0 ? -magn_acceleration_parallel : magn_acceleration_parallel);

            if (sensor_value_transformations_.find(t) != sensor_value_transformations_.end()) {
                const std::vector<std::pair<double, double> > &transformations = sensor_value_transformations_.at(t);
                for (unsigned int i = 0; i < 3; ++i) {
                    sensor_data[offset + i] = TransformValue(sensor_data[offset + i], transformations.at(i));
                }
            }

        } else if (t == Height) {
            sensor_data[offset] = AddNoise(VectorNorm(height), t);

        } else if (t == Mass) {
            const double &mass = state[6];

            sensor_data[offset] = AddNoise(mass, t);

        }
        offset += SensorTypeConfigurations.at(t).first;
    }

    return sensor_data;
}

double SensorSimulator::TransformValue(const double &sensor_value, const std::pair<double, double> &transformation_params) {
    return ((sensor_value - transformation_params.first) / transformation_params.second) * 0.25 + 0.5;
}
