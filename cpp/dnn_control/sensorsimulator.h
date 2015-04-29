#ifndef SENSORSIMULATOR_H
#define SENSORSIMULATOR_H

#include "asteroid.h"
#include "systemstate.h"
#include "samplefactory.h"

#include <vector>
#include <set>
#include <map>

class SensorSimulator {
    /*
     * This class generates the artificial sensor data for a controller.
     */
public:
    // The number of output dimensions the sensor simulator will generate
    enum SensorType {
        RelativePosition,
        Velocity,
        OpticalFlow,
        ExternalAcceleration,
        TotalAcceleration,
        Height,
        Mass
    };

    const static std::map<SensorType, std::pair<unsigned int, double> > SensorTypeConfigurations;

    SensorSimulator(SampleFactory &sample_factory, const Asteroid &asteroid);

    void SetSensorTypes(const std::set<SensorType> &sensor_types);

    void SetNoiseEnabled(const bool &enable_noise);

    void SetTargetPosition(const Vector3D &target_position);

    void SetSensorValueTransformations(const std::map<SensorType, std::vector<std::pair<double, double> > > &sensor_value_transformations);


    // Generates (simulates) sensor data based on the current spacecraft state "state" and time "time"
    virtual std::vector<double> Simulate(const SystemState &state, const Vector3D &height, const Vector3D &perturbations_acceleration, const double &time, const Vector3D &thrust);

    // The number of sensor data dimensions produced by the SensorSimulator
    unsigned int Dimensions() const;

    // SensorSimulator can throw the following exceptions
    class Exception {};


protected:
    static double TransformValue(const double &sensor_value, const std::pair<double, double> &transformation_params);

    double AddNoise(const double &sensor_value, const SensorType &type);

    // How large is the sensor data space
    unsigned int dimensions_;

    // The underlying random sample factory
    SampleFactory &sample_factory_;

    // The system's asteroid
    const Asteroid &asteroid_;

    // The active sensor types
    std::set<SensorType> sensor_types_;

    // Is noise enabled
    bool noise_enabled_;

    // The target position the spacecraft is supposed to hover over, if required.
    Vector3D target_position_;

    // Transform sensor variables. Allow two parameters
    std::map<SensorType, std::vector<std::pair<double, double> > > sensor_value_transformations_;
};

#endif // SENSORSIMULATOR_H
