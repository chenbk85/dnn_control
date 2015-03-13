#ifndef SENSORSIMULATOR_H
#define SENSORSIMULATOR_H

#include "asteroid.h"
#include "systemstate.h"
#include "samplefactory.h"

#include <vector>

class SensorSimulator {
    /*
     * This abstract class generates the artificial sensor data required for a controller.
     */
public:
    SensorSimulator(const unsigned int &dimensions, SampleFactory &sample_factory, const Asteroid &asteroid);

    virtual ~SensorSimulator();

    // Generates (simulates) sensor data based on the current spacecraft state "state" and time "time"
    virtual std::vector<double> Simulate(const SystemState &state, const Vector3D &height, const Vector3D &perturbations_acceleration, const double &time) = 0;

    // The number of sensor data dimensions produced by the SensorSimulator
    unsigned int Dimensions() const;

    // SensorSimulator can throw the following exceptions
    class Exception {};

    // Returns sensor data in a string representation
    static std::string SensorDataToString(const std::vector<double> &data);

protected:
    // How large is the sensor data space
    unsigned int dimensions_;

    // The underlying random sample factory
    SampleFactory &sample_factory_;

    // The system's asteroid
    const Asteroid &asteroid_;

    // The noise configuration for every sensor dimension
    std::vector<double> noise_configurations_;
};

#endif // SENSORSIMULATOR_H
