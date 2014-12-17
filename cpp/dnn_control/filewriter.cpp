#include "filewriter.h"
#include <iomanip>

FileWriter::FileWriter() {
    file_ << std::setprecision(10);
}

void FileWriter::CreateVisualizationFile(const std::string &path_to_file, const double &control_frequency, const Asteroid &asteroid, const std::vector<Vector3D> &positions, const std::vector<Vector3D> &heights) {
    file_.open(path_to_file.c_str());
    file_ << asteroid.SemiAxis(0) << ",\t" << asteroid.SemiAxis(1) << ",\t" << asteroid.SemiAxis(2) << ",\t" << control_frequency << std::endl;
    for (unsigned int i = 0; i < positions.size(); ++i) {
        const Vector3D &pos = positions.at(i);
        const Vector3D &height = heights.at(i);
        file_ << pos[0] << ",\t" << pos[1] << ",\t" << pos[2] << ",\t" << height[0] << ",\t" << height[1] << ",\t" << height[2] << " \n";
    }
    file_.close();
}

void FileWriter::CreateSensorDataFile(const std::string &path_to_file, const double &control_frequency, const double &time, const Asteroid &asteroid, const Vector3D &spacecraft_position, const Vector3D spacecraft_velocity, const double &spacecraft_mass, const double &spacecraft_specific_impulse, const Vector3D &target_position, const std::vector<SensorData> &sensor_data)
{
    const Vector3D angular_velocity = asteroid.InitialAngularVelocity();
    file_.open(path_to_file.c_str());
    file_ << "# target position: " << target_position[0] << ", " << target_position[1] << ", " << target_position[2] << " m" << std::endl;
    file_ << "#" << std::endl;
    file_ << "# control frequency: " << control_frequency << " Hz" << std::endl;
    file_ << "# simulation time: " << time << " s" << std::endl;
    file_ << "#" << std::endl;
    file_ << "# asteroid:" << std::endl;
    file_ << "#  density: " << asteroid.Density() << " kg/m^3" << std::endl;
    file_ << "#  time bias: " << asteroid.TimeBias() << " s" << std::endl;
    file_ << "#  semi axis: " << asteroid.SemiAxis(0) << ", " << asteroid.SemiAxis(1) << ", " << asteroid.SemiAxis(2) << " m" << std::endl;
    file_ << "#  angular velocity: " << angular_velocity[0] << ", " << angular_velocity[1] << ", " << angular_velocity[2] << " 1/s" << std::endl;
    file_ << "#" << std::endl;
    file_ << "# spacecraft:" << std::endl;
    file_ << "#  mass: " << spacecraft_mass << " kg" << std::endl;
    file_ << "#  specific impulse: " << spacecraft_specific_impulse << " s" << std::endl;
    file_ << "#  position: " << spacecraft_position[0] << ", " << spacecraft_position[1] << ", " << spacecraft_position[2] << " m" << std::endl;
    file_ << "#  velocity: " << spacecraft_velocity[0] << ", " << spacecraft_velocity[1] << ", " << spacecraft_velocity[2] << " m/s" << std::endl;
    file_ << "#" << std::endl;
    for (unsigned int i = 0; i < sensor_data.size(); ++i) {
        const SensorData &data = sensor_data.at(i);
        file_ << data[0];
        for (unsigned int j = 1; j < data.size(); ++j) {
            file_ << ", " << data[j];
        }
        file_ << std::endl;
    }
    file_.close();
}


