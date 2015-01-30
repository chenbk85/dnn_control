#ifndef FULLSTATECONTROLLER_H
#define FULLSTATECONTROLLER_H

#include "controller.h"

class ControllerFullState : public Controller {
public:
    static const unsigned int kDimensions;

    ControllerFullState(const double &maximum_thrust, const Vector3D &target_position);
    virtual ~ControllerFullState();

    // thrust = F(sensor_data), whereas F can be eg., a PD controller, some RL solution, a NN, ...
    virtual Vector3D GetThrustForSensorData(const SensorData &sensor_data);

    void SetCoefficients(const std::vector<double> &pid_coefficients);

private:
    double latest_control_action_;

    double coef_proportional_;

    double coef_derivative_;

    double coef_integral_;

    Vector3D target_position_;

    Vector3D previous_error_;

    Vector3D integral_;
};

#endif // FULLSTATECONTROLLER_H
