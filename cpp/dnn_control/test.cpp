#include "test.h"
#include "asteroid.h"
#include "utility.h"
#include "odesystem.h"
#include "simulator.h"
#include "sensorsimulator5d.h"
#include "controller5d.h"
#include "odeint.h"

#include <iostream>
#include <iomanip>
#include <ctime>

#define PATH_TO_FILE    "../result.txt"

typedef double AngularVelocityState[3];
struct AngularVelocitySystem {
    Vector3D inertia;
    void operator()(const AngularVelocityState &state, AngularVelocityState &d_state_dt, double time) const {
        d_state_dt[0] = (inertia[1] - inertia[2]) * state[1] * state[2] / inertia[0];
        d_state_dt[1] = (inertia[2] - inertia[0]) * state[2] * state[0] / inertia[1];
        d_state_dt[2] = (inertia[0] - inertia[1]) * state[0] * state[1] / inertia[2];
    };
    AngularVelocitySystem(const Vector3D &para_inertia) {
        for (unsigned int i = 0; i < 3; ++i) {
            inertia[i] = para_inertia[i];
        }
    };
};

void UnitTestAngularVelocity() {
    using namespace boost::numeric::odeint;

    const Vector3D semi_axis = {10000.0, 6000.0, 4000.0};
    const double density = 2215.0;
    const Vector2D angular_velocity_xz = {-0.0002, -0.0008};
    const double time_bias = 0.0;

    Asteroid asteroid(semi_axis, density, angular_velocity_xz, time_bias);

    const Vector3D inertia = asteroid.Inertia();
    AngularVelocitySystem sys(inertia);

    const int num_test_cases = 10000;
    double min_error = 1e20;
    double max_error = 1e-20;
    double avg_error = 0.0;
    for (unsigned int i = 0; i < num_test_cases; ++i) {
        const double time = SampleUniform(0.0, 24.0*60.0*60.0);
        const boost::tuple<Vector3D, Vector3D> result = asteroid.AngularVelocityAndAccelerationAtTime(time);
        Vector3D omega_analytical = boost::get<0>(result);

        AngularVelocityState omega_numerical = {angular_velocity_xz[0], 0.0, angular_velocity_xz[1]};

        runge_kutta4<AngularVelocityState> integrator;
        const double dt = 0.1;
        integrate_const(integrator, sys, omega_numerical, 0.0, time, dt);

        double error = 0.0;
        for (unsigned int i = 0; i < 3; ++i) {
            error += (omega_analytical[i] - omega_numerical[i]) * (omega_analytical[i] - omega_numerical[i]);
        }
        error = sqrt(error);

        if (error < min_error) {
            min_error = error;
        } else {
            max_error = error;
        }
        avg_error += error;
    }

    std::cout << std::setprecision(10) << "min: " << min_error << std::endl << "max: " << max_error << std::endl << "avg: " << avg_error/(double)num_test_cases << std::endl;
}

void UnitTestTrajectory() {
    std::cout << "looking for strange physics..." << std::setprecision(10) << std::endl;

    const double time = 1.0 * 60.0 * 60.0;

    const Vector3D semi_axis = {10000.0, 6000.0, 4000.0};
    const double density = 2215.0;
    const Vector2D angular_velocity_xz = {-0.0002, 0.0008};
    const double time_bias = 0.0;

    const double band_width_scaling = 4.0;

    const double spacecraft_specific_impulse = 200.0;
    const double spacecraft_mass = 1000.0;
    const double spacecraft_maximum_thrust = 21.0;

    const double control_frequency = 10.0;

    const double sensor_noise = 0.05;
    const double perturbation_noise = 0.0;

    Asteroid asteroid(semi_axis,density, angular_velocity_xz, time_bias);
    SensorSimulator5D *sensor_simulator = new SensorSimulator5D(asteroid, sensor_noise);
    Controller5D *spacecraft_controller = new Controller5D(spacecraft_maximum_thrust);
    /*Simulator simulator(asteroid, sensor_simulator, spacecraft_controller, control_frequency, perturbation_noise);
    simulator.InitSpacecraftSpecificImpulse(spacecraft_specific_impulse);

    const int num_test_cases = 1000000;
    for (unsigned int i = 0; i < num_test_cases; ++i) {
        const Vector3D position = SamplePointOutSideEllipsoid(semi_axis, band_width_scaling);

        Vector3D velocity = CrossProduct(angular_velocity, position);
        velocity[0] *= -1;
        velocity[1] *= -1;
        velocity[2] *= -1;

        State state;
        for(unsigned int i = 0; i < 3; ++i) {
            state[i] = position[i];
            state[3+i] = velocity[i];
        }
        state[6] = spacecraft_mass;

        State next_state;
        const Vector3D thrust = {0.0, 0.0, 0.0};
        simulator.NextState(state, thrust, 0.0, next_state);

        const Vector3D next_position = {next_state[0], next_state[1], next_state[2]};
        double norm_pos = 0.0;
        double norm_next_pos = 0.0;
        for (unsigned int i = 0; i < 3; ++i) {
            norm_pos += position[i] * position[i];
            norm_next_pos += next_position[i] * next_position[i];
        }
        norm_pos = sqrt(norm_pos);
        norm_next_pos = sqrt(norm_next_pos);
        if(norm_pos < norm_next_pos) {
            Vector3D gravity = asteroid.GravityAtPosition(position);
            gravity[0] /= state[6];
            gravity[1] /= state[6];
            gravity[2] /= state[6];

            const boost::tuple<Vector3D, Vector3D> result = asteroid.AngularVelocityAndAccelerationAtTime(time);
            Vector3D angular_velocity = boost::get<0>(result);

            const Vector3D centrifugal_acceleration = CrossProduct(angular_velocity, CrossProduct(angular_velocity, position));

            std::cout << "gravity at position: (" << gravity[0] << "," << gravity[1] << "," << gravity[2] << ")" << std::endl;
            std::cout << "centrifugal force at position: (" << centrifugal_acceleration[0] << "," << centrifugal_acceleration[1] << "," << centrifugal_acceleration[2] << ")" << std::endl;

            simulator.InitSpacecraft(position, velocity, spacecraft_mass, spacecraft_specific_impulse);
            //simulator.Run(time, true, false);
            //simulator.FlushLoggedStatesToFile(PATH_TO_FILE);

            std::cout << "-> check the result.txt file..." << std::endl;
            break;
        }
    }
    */
    delete sensor_simulator;
    std::cout << "done." << std::endl;
}


void UnitTestAny() {
    const Vector3D semi_axis = {1103.1670527778024, 466.18400577010857, 293.35148306268576};
    const double density = 2215.0;
    const Vector2D angular_velocity_xz = {-0.0002, 0.0008};
    const double time_bias = 0.0;

    Asteroid asteroid(semi_axis,density, angular_velocity_xz, time_bias);
    const Vector3D position = {816.5726055517212, 0.9933720217425616, -716.1995078171824};

    const boost::tuple<Vector3D, double> result = asteroid.NearestPointOnSurfaceToPosition(position);
    const double distance = boost::get<1>(result);
    std::cout << distance << std::endl;
}


void UnitTestGravity()
{
    const Vector3D semi_axis = {1103.1670527778024, 466.18400577010857, 293.35148306268576};
    const double density = 2215.0;
    const Vector2D angular_velocity_xz = {-0.0002, 0.0008};
    const double time_bias = 0.0;

    Asteroid asteroid(semi_axis,density, angular_velocity_xz, time_bias);

    double avg_error = 0.0;
    double avg_time_legendre = 0.0;
    double avg_time_carlson = 0.0;
    const int num_tests = 1000000;
    for (unsigned int j = 0; j < num_tests; ++j) {

    }
    avg_error /= num_tests;
    avg_time_legendre = avg_time_legendre / num_tests;
    avg_time_carlson = avg_time_carlson / num_tests;
    std::cout << "error : " << avg_error << std::endl;
    std::cout << "time cersosimo : " << avg_time_legendre / CLOCKS_PER_SEC << std::endl;
    std::cout << "time izzo : " << avg_time_carlson / CLOCKS_PER_SEC << std::endl;
}
