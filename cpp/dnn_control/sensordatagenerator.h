#ifndef SENSORDATAGENERATOR_H
#define SENSORDATAGENERATOR_H

#include <string>
#include <vector>

class SensorDataGenerator {
	/*
	* This class generates and creates files for sensor data streams generated by random simulations.
	*/
public:
    SensorDataGenerator(const std::string &path_to_output_folder, const double &data_set_time);

    // Generates the sensor data stream files
    void Generate(const unsigned int &num_datasets, const unsigned int &random_seed, const std::vector<double> &neural_network_weights=std::vector<double>());

private:
    // The time for which the sensor data stream will be produced
    double data_set_time_;

    // The path to the output folder
    std::string path_to_output_folder_;
};

#endif // SENSORDATAGENERATOR_H
