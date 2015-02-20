#ifndef SENSORSIMULATORNEURALNETWORK_H
#define SENSORSIMULATORNEURALNETWORK_H

#include "sensorsimulator.h"

class SensorSimulatorNeuralNetwork : public SensorSimulator {
	/*
    * This class generates the artificial sensor data required for a neural network controller. The data produced by this sensor simulator contains optical flow and accelerometer data.
    */
public:
	// The number of output dimensions the sensor simulator will generate
    static const unsigned int kDimensions;

    SensorSimulatorNeuralNetwork(SampleFactory &sample_factory, const Asteroid &asteroid);

    virtual ~SensorSimulatorNeuralNetwork();

    // Generates (simulates) sensor data based on the current spacecraft state "state" and time "time"
    virtual SensorData Simulate(const SystemState &state, const Vector3D &height, const Vector3D &perturbations_acceleration, const double &time);

    // SensorSimulatorNeuralNetwork can throw the following exceptions
    class RangeMalConfigurationException : public Exception {};

private:
    // Sensor data values will be normalized between [0,1] by dividing them by the maximum range
    std::vector<double> sensor_maximum_absolute_ranges_;
};

#endif // SENSORSIMULATORNEURALNETWORK_H
