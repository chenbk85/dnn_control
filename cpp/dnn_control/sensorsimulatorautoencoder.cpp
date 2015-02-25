#include "sensorsimulatorautoencoder.h"
#include "configuration.h"

const unsigned int SensorSimulatorAutoencoder::kDimensions = SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER * (SSA_DATA_HISTORY + 1);

SensorSimulatorAutoencoder::SensorSimulatorAutoencoder(SampleFactory &sample_factory, const Asteroid &asteroid)
    : SensorSimulator(kDimensions, sample_factory, asteroid) {

    noise_configurations_ = std::vector<double>(SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER, 0.05);

    sensor_values_cache_ = std::vector<double>(SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER * (SSA_DATA_HISTORY + 1), 0.0);
    cache_index_ = 0;
    num_simulations_ = 0;
}

SensorSimulatorAutoencoder::~SensorSimulatorAutoencoder() {

}

void SensorSimulatorAutoencoder::Reset() {
    cache_index_ = 0;
    num_simulations_ = 0;
}


SensorData SensorSimulatorAutoencoder::Simulate(const SystemState &state, const Vector3D &height, const Vector3D &perturbations_acceleration, const double &time) {
    SensorData sensor_data(dimensions_, 0.0);

    std::vector<double> sensor_seed(SSA_DATA_DIMENSIONS, 0.0);

    const Vector3D &velocity = {state[3], state[4], state[5]};

    const double norm_height_pow2 = VectorDotProduct(height, height);
    const double norm_height = sqrt(norm_height_pow2);
    const double coef_norm_height = sqrt(3) / norm_height;

    const double velocity_dot_height = VectorDotProduct(velocity, height);
    const double scaling = velocity_dot_height / norm_height_pow2;

    const Vector3D &velocity_vertical = {scaling * height[0], scaling * height[1], scaling * height[2]};
    const Vector3D velocity_horizontal = VectorSub(velocity, velocity_vertical);

    for (unsigned int i = 0; i < 3; ++i) {
        sensor_seed[i] = velocity_vertical[i] * coef_norm_height;
        sensor_seed[3+i] = velocity_horizontal[i] * coef_norm_height;
    }

    // Every initial sensor value gets tripled and added with normal distributed noise
    std::vector<double> sensor_duplicates(SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER, 0.0);
    for (unsigned int i = 0; i < SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER; ++i) {
        const unsigned int sensor_data_index = i % SSA_DATA_DIMENSIONS;
        sensor_duplicates[i] = sensor_seed[sensor_data_index];

#if SSA_DATA_WITH_NOISE
        sensor_duplicates[i] += sensor_seed[sensor_data_index] * sample_factory_.SampleNormal(0.0, noise_configurations_.at(i));
#endif

        // Normalize between [0,1]
        const double sensor_maximum_absolute_range = 1.0;
        if (sensor_duplicates[i] > sensor_maximum_absolute_range) {
            sensor_duplicates[i] = sensor_maximum_absolute_range;
        } else if (sensor_duplicates[i] < -sensor_maximum_absolute_range) {
            sensor_duplicates[i] = -sensor_maximum_absolute_range;
        }
        sensor_duplicates[i] = 0.5 * sensor_duplicates[i] / sensor_maximum_absolute_range + 0.5;
    }

       // write new sensor values at correct place
        const int cache_index = cache_index_;
        int start_index = cache_index - (SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER);
        if (start_index < 0) {
            start_index = dimensions_ - (SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER);
        }
        for (unsigned int i = 0; i < SSA_DATA_DIMENSIONS * SSA_DATA_MULTIPLIER; ++i) {
            sensor_values_cache_[start_index +i] = sensor_duplicates[i];
        }

    for (unsigned int i = 0; i < dimensions_; ++i) {
        sensor_data[(dimensions_ - 1) - i] = sensor_values_cache_[(cache_index_ + i) % dimensions_];
    }

    cache_index_ += SSA_DATA_DIMENSIONS*SSA_DATA_MULTIPLIER;
    if (cache_index_ == dimensions_) {
        cache_index_ = 0;
    }

    if (num_simulations_ < SSA_DATA_HISTORY) {
        num_simulations_++;
        return SensorData(dimensions_, 0.0);
    }
    return sensor_data;
}
