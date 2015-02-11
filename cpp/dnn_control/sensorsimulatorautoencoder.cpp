#include "sensorsimulatorautoencoder.h"
#include "configuration.h"

const unsigned int SensorSimulatorAutoencoder::kDimensions = SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER * SSA_DATA_HISTORY;

SensorSimulatorAutoencoder::SensorSimulatorAutoencoder(SampleFactory &sample_factory, const Asteroid &asteroid)
    : SensorSimulator(kDimensions, sample_factory, asteroid) {
    sensor_maximum_absolute_ranges_ = {10.0, 10.0, 10.0, 0.025, 0.025, 0.025};

    if (sensor_maximum_absolute_ranges_.size() != SSA_DATA_DIMENSIONS) {
        throw RangeMalConfigurationException();
    }

    noise_configurations_ = std::vector<double>(SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER, 0.05);
    sensor_values_cache_ = std::vector<double>(SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER * SSA_DATA_HISTORY, 0.0);
    cache_index_ = 0;
    first_simulation_ = true;
}

SensorSimulatorAutoencoder::~SensorSimulatorAutoencoder() {

}

void SensorSimulatorAutoencoder::Reset() {
    cache_index_ = 0;
    first_simulation_ = true;
}


SensorData SensorSimulatorAutoencoder::Simulate(const SystemState &state, const Vector3D &height, const Vector3D &perturbations_acceleration, const double &time) {
    SensorData sensor_data(dimensions_, 0.0);

    std::vector<double> sensor_seed(SSA_DATA_DIMENSIONS, 0.0);

    const Vector3D position = {state[0], state[1], state[2]};
    const Vector3D velocity = {state[3], state[4], state[5]};

    /*
    const double norm_height_pow2 = VectorDotProduct(height, height);
    const double coef_norm_height = 1.0 / sqrt(norm_height_pow2);

    const double vel_dot_height = VectorDotProduct(velocity, height);

    const double coef_projection = vel_dot_height / norm_height_pow2;

    const Vector3D normalized_height = {height[0] * coef_norm_height, height[1] * coef_norm_height, height[2] * coef_norm_height};
    const Vector3D velocity_vertical = {coef_projection * height[0], coef_projection * height[1], coef_projection * height[2]};
    const Vector3D velocity_remaining = {velocity[0] - velocity_vertical[0],
                                         velocity[1] - velocity_vertical[1],
                                         velocity[2] - velocity_vertical[2]};

    const double norm_vel_vert = VectorNorm(velocity_vertical);
    const double norm_vel_rem = VectorNorm(velocity_remaining);

    Vector3D normalized_velocity_remaining;
    if (norm_vel_rem) {
        const double coef_vel_rem = 1.0 / norm_vel_rem;
        normalized_velocity_remaining[0] = velocity_remaining[0] * coef_vel_rem;
        normalized_velocity_remaining[1] = velocity_remaining[1] * coef_vel_rem;
        normalized_velocity_remaining[2] = velocity_remaining[2] * coef_vel_rem;
    }

    const double coef_vert_height = norm_vel_vert * coef_norm_height;
    const double coef_rem_height = norm_vel_rem * coef_norm_height;

    for (unsigned int i = 0; i < 3; ++i) {
        sensor_seed[i] = coef_vert_height * normalized_height[i];
        sensor_seed[3+i] = coef_rem_height * normalized_velocity_remaining[i];
    }
    */

    sensor_seed[0] = velocity[0];
    sensor_seed[1] = velocity[1];
    sensor_seed[2] = velocity[2];

    const Vector3D gravity_acceleration = asteroid_.GravityAccelerationAtPosition(position);

    const boost::tuple<Vector3D, Vector3D> result_angular = asteroid_.AngularVelocityAndAccelerationAtTime(time);
    const Vector3D angular_velocity = boost::get<0>(result_angular);
    const Vector3D angular_acceleration = boost::get<1>(result_angular);

    const Vector3D euler_acceleration = VectorCrossProduct(angular_acceleration, position);
    const Vector3D centrifugal_acceleration = VectorCrossProduct(angular_velocity, VectorCrossProduct(angular_velocity, position));

    Vector3D tmp;
    for (unsigned int i = 0; i < 3; ++i) {
        tmp[i] = angular_velocity[i] * 2.0;
    }

    const Vector3D coriolis_acceleration = VectorCrossProduct(tmp, velocity);

    for (unsigned int i = 0; i < 3; ++i) {
        sensor_seed[3+i] = perturbations_acceleration[i]
                + gravity_acceleration[i]
                - coriolis_acceleration[i]
                - euler_acceleration[i]
                - centrifugal_acceleration[i];
    }

    // Every initial sensor value gets tripled and added with normal distributed noise
    std::vector<double> sensor_duplicates(SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER, 0.0);
    for (unsigned int i = 0; i < SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER; ++i) {
        const unsigned int sensor_data_index = i % SSA_DATA_DIMENSIONS;
        sensor_duplicates[i] = sensor_seed[sensor_data_index] + sensor_seed[sensor_data_index] * sample_factory_.SampleNormal(0.0, noise_configurations_.at(i));

        // Normalize between [0,1]
        const double sensor_maximum_absolute_range = sensor_maximum_absolute_ranges_.at(sensor_data_index);
        if (sensor_duplicates[i] > sensor_maximum_absolute_range) {
            sensor_duplicates[i] = sensor_maximum_absolute_range;
        } else if (sensor_duplicates[i] < -sensor_maximum_absolute_range) {
            sensor_duplicates[i] = -sensor_maximum_absolute_range;
        }
        sensor_duplicates[i] = sensor_duplicates[i] / (2.0 * sensor_maximum_absolute_range) + 0.5;
    }

    if (first_simulation_) {
        first_simulation_ = false;
        // Assume spacecraft was standing still at current location with no sensor noise...
        for (unsigned int i = 0; i < SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER * SSA_DATA_HISTORY; ++i) {
            sensor_values_cache_[i] = sensor_duplicates[i % (SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER)];
        }
    } else {
       // write new sensor values at correct place
        const int cache_index = cache_index_;
        int start_index = cache_index - (SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER);
        if (start_index < 0) {
            start_index = (SSA_DATA_DIMENSIONS*SSA_DATA_MULTIPLIER*SSA_DATA_HISTORY) - (SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER);
        }
        for (unsigned int i = 0; i < SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER; ++i) {
            sensor_values_cache_[start_index +i] = sensor_duplicates[i];
        }
    }

    for (unsigned int i = 0; i < SSA_DATA_DIMENSIONS*SSA_DATA_MULTIPLIER*SSA_DATA_HISTORY; ++i) {
        sensor_data[(SSA_DATA_DIMENSIONS*SSA_DATA_MULTIPLIER*SSA_DATA_HISTORY - 1) - i] = sensor_values_cache_[(cache_index_ + i) % (SSA_DATA_DIMENSIONS*SSA_DATA_MULTIPLIER*SSA_DATA_HISTORY)];
    }

    cache_index_ += SSA_DATA_DIMENSIONS*SSA_DATA_MULTIPLIER;
    if (cache_index_ == (SSA_DATA_DIMENSIONS*SSA_DATA_MULTIPLIER*SSA_DATA_HISTORY)) {
        cache_index_ = 0;
    }

    return sensor_data;
}
