'''

Simulator state:
0 : x
1 : y
2 : z
3 : dx
4 : dy
5 : dz
6 : m

spacecraft_controller thrust
0 : T_x
1 : T_y
2 : T_z

'''


class Simulator:
    # Constructor
    def __init__(self, asteroid, spacecraft_position, spacecraft_velocity, spacecraft_mass, spacecraft_specific_impulse,
                 sensor_simulator, spacecraft_controller, control_frequency):
        from numpy import array
        from constants import EARTH_ACCELERATION

        self.sensor_simulator = sensor_simulator
        self.spacecraft_controller = spacecraft_controller
        self.control_interval = 1.0 / control_frequency

        self.asteroid = asteroid

        self.spacecraft_specific_impulse = float(spacecraft_specific_impulse)
        self.spacecraft_state = array([float(spacecraft_position[0]),
                                       float(spacecraft_position[1]),
                                       float(spacecraft_position[2]),
                                       float(spacecraft_velocity[0]),
                                       float(spacecraft_velocity[1]),
                                       float(spacecraft_velocity[2]),
                                       float(spacecraft_mass)])

        self.earth_acceleration_mul_spacecraft_specific_impulse = EARTH_ACCELERATION * \
                                                                  self.spacecraft_specific_impulse


    def __str__(self):
        result = ["Simulator:"]
        keys = sorted([key for key in self.__dict__])
        for key in keys:
            result.append("{key}='{value}'".format(key=key, value=self.__dict__[key]))

        return "\n ".join(result)

    # Perform the simulation for time seconds
    def run(self, time, collect_positions=False):
        from numpy import empty

        print("Run for {0} time with following configuration:".format(time))
        print(self)

        control_interval = self.control_interval
        iterations = int(time / self.control_interval)
        positions = []

        if collect_positions:
            positions = empty([iterations, 3])

        for i in range(iterations):
            sensor_data = self.sensor_simulator.simulate(self.spacecraft_state)
            thrust = self.spacecraft_controller.get_thrust(sensor_data)
            perturbations_acceleration = self.simulate_perturbations()

            if collect_positions:
                positions[i][:] = self.spacecraft_state[0:3]

            self.simulate_dynamics(perturbations_acceleration, thrust, i * control_interval,
                                   (i + 1) * control_interval)

        return positions

    # Integrate the system from start_time to end_time
    def simulate_dynamics(self, perturbations_acceleration, thrust, start_time, end_time):
        from scipy.integrate import odeint
        from math import isnan, isinf

        result = odeint(self.dynamics, self.spacecraft_state, [start_time, end_time],
                        (perturbations_acceleration, thrust))
        self.spacecraft_state = result[1][:]

        for val in self.spacecraft_state:
            if isnan(val) or isinf(val):
                raise Exception(self)

    # Simulator dynamics from "Control of Hovering Spacecraft Using Altimetry"
    # eq (69) and "Robust Spacecraft Hovering Near Small Bodies in
    # Environments with Unknown Dynamics using Reinforcement Learning" eq (6)
    def dynamics(self, state, time, perturbations_acceleration, thrust):
        from utility import cross_product
        from math import sqrt

        position = state[0:3]
        velocity = state[3:6]
        mass = state[6]

        gravity = self.asteroid.gravity_at_position(state[0:3])
        gravity_acceleration = [val / mass for val in gravity]
        thrust_acceleration = [val / mass for val in thrust]

        angular_velocity = self.asteroid.angular_velocity_at_time(time)
        angular_velocity_mul2 = [2.0 * val for val in angular_velocity]
        angular_acceleration = self.asteroid.angular_acceleration_at_time(time)

        centrifugal_acceleration = cross_product(
            angular_velocity, cross_product(angular_velocity, position))
        coriolis_acceleration = cross_product(angular_velocity_mul2, velocity)
        euler_acceleration = cross_product(angular_acceleration, position)

        d_dt_state = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]

        # d/dt r
        d_dt_state[0] = velocity[0]
        d_dt_state[1] = velocity[1]
        d_dt_state[2] = velocity[2]

        # d/dt v
        d_dt_state[3] = perturbations_acceleration[0] \
                        + gravity_acceleration[0] \
                        + thrust_acceleration[0] \
                        - coriolis_acceleration[0] \
                        - euler_acceleration[0] \
                        - centrifugal_acceleration[0]
        d_dt_state[4] = perturbations_acceleration[1] \
                        + gravity_acceleration[1] \
                        + thrust_acceleration[1] \
                        - coriolis_acceleration[1] \
                        - euler_acceleration[1] \
                        - centrifugal_acceleration[1]
        d_dt_state[5] = perturbations_acceleration[2] \
                        + gravity_acceleration[2] \
                        + thrust_acceleration[2] \
                        - coriolis_acceleration[2] \
                        - euler_acceleration[2] \
                        - centrifugal_acceleration[2]

        # d/dt m
        d_dt_state[6] = sqrt(thrust[0] ** 2 + thrust[1] ** 2 + thrust[2]
                             ** 2) / self.earth_acceleration_mul_spacecraft_specific_impulse
        return d_dt_state

    # External perturbations acceleration
    def simulate_perturbations(self):
        from numpy import random

        mean = 0.0
        variance = 1e-6
        spacecraft_mass = self.spacecraft_state[6]
        return [spacecraft_mass * random.normal(mean, variance),
                spacecraft_mass * random.normal(mean, variance),
                spacecraft_mass * random.normal(mean, variance)]

