class Asteroid:

    """
    This class represents the dynamics and gravity of an asteroid shaped as a rigid ellipsoid having different
    rotational properties along its three principal inertia axis (Ix, Iy, Iz).

    For the angular velocity dynamics the analytical solution in terms of Jacobi elliptic functions is used (Implementation
    largely inspired from Landau Lifshitz Mechanics paragrpah 37). We thus assume w_y(-t_bias) = 0. So we only are free to specify two
    more initial conditions and we do so by assigning kinetic energy and angular momentum. As a result the user is not free to specify
    the initial angular velocity, but only the values of the two prime integrals. He does so by defining an angular velocity vector in
    the constructor, this is used only to compute kinetic energy and rotational momentum, the second of its components (w_y) will thus be disregarded
    as 0 will instead be used.

    For the asteroid gravity, we use the implementation of Dario Cersosimo whi kindly sent us his matlab files.
    """

    def __init__(self, semi_axis_a, semi_axis_b, semi_axis_c, density, angular_velocity, time_bias):

        """
        This constructs the asteroid object from its inertia properties and its prime integrals value

        USAGE: ast = Asteroid(0.1,0.2,0.3,2.3,[0.23,0.1,0.56],234)

        * semi_axis_a, semi_axis_b, semi_axis_c:        Semi Axis a,b,c  where a < b < c (UNITS)
        * density:                                      Asteroid density (UNITS)
        * angular_velocity:                             Asteroid hypothetical angular velocity (used to define prime integrals only)
        * time_bias:                                    A time bias applied so that w_y(-t_bias) = 0

        """

        from math import sqrt
        from constants import PI

        # SU controls (Stupid User)
        if (semi_axis_a == semi_axis_b) or (semi_axis_a == semi_axis_c) or (semi_axis_b == semi_axis_c):
            raise Exception("This function does not work for symmetric tops, make sure Ix, Iy and Iz are different")
        if sorted([semi_axis_a, semi_axis_b, semi_axis_c]) != [semi_axis_c, semi_axis_b, semi_axis_a]:
            raise Exception("This function assumes semi axis c < b < a, please make sure this is the case")
        if angular_velocity[0] == 0 or angular_velocity[2] == 0:
            raise Exception("You cannot define zero values for w_x or w_z as this "
                            "functions already assumes w_y=0 (thus the motion would be a rotation")

        self.semi_axis_a = float(semi_axis_a)
        self.semi_axis_a_pow2 = self.semi_axis_a ** 2
        self.semi_axis_b = float(semi_axis_b)
        self.semi_axis_b_pow2 = self.semi_axis_b ** 2
        self.semi_axis_c = float(semi_axis_c)
        self.semi_axis_c_pow2 = self.semi_axis_c ** 2
        self.density = float(density)
        self.mass = 4.0 / 3.0 * PI * self.semi_axis_a * self.semi_axis_b * self.semi_axis_c * self.density

        self.inertia_x = 0.2 * self.mass * (semi_axis_b ** 2 + semi_axis_c **2)
        self._inertia_x_pow2 = self.inertia_x ** 2
        self.inertia_y = 0.2 * self.mass * (semi_axis_a ** 2 + semi_axis_c **2)
        self._inertia_y_pow2 = self.inertia_y ** 2
        self.inertia_z = 0.2 * self.mass * (semi_axis_a ** 2 + semi_axis_b **2)
        self._inertia_z_pow2 = self.inertia_z ** 2
        self.time_bias = float(time_bias)

        self._angular_velocity = [float(val) for val in angular_velocity]

        sign_x = (-1.0, 1.0)[self._angular_velocity[0] > 0]
        sign_y = (-1.0, 1.0)[self._angular_velocity[0] * self._angular_velocity[2] > 0]
        sign_z = (-1.0, 1.0)[self._angular_velocity[2] > 0]

        self.energy_mul2 = self.inertia_x * self._angular_velocity[0] ** 2 \
                           + self.inertia_y * self._angular_velocity[1] ** 2 \
                           + self.inertia_z * self._angular_velocity[2] ** 2
        self.momentum_pow2 = self.inertia_x ** 2 * self._angular_velocity[0] ** 2 \
                             + self.inertia_y ** 2 * self._angular_velocity[1] ** 2\
                             + self.inertia_z ** 2 * self._angular_velocity[2] ** 2

        inertia_x = self.inertia_x
        inertia_y = self.inertia_y
        inertia_z = self.inertia_z

        self._inversion = False
        if self.momentum_pow2 < self.energy_mul2 * inertia_y:
            inertia_z = self.inertia_x
            inertia_x = self.inertia_z
            self._inversion = True
            sign_z = (-1.0, 1.0)[self._angular_velocity[0] > 0]
            sign_x = (-1.0, 1.0)[self._angular_velocity[2] > 0]


        # Lifshitz eq (37.10)
        self.elliptic_coef_angular_velocity_x = sign_x * sqrt((self.energy_mul2 * inertia_z - self.momentum_pow2)
                                                              / (inertia_x * (inertia_z - inertia_x)))
        self.elliptic_coef_angular_velocity_y = sign_y * sqrt((self.energy_mul2 * inertia_z - self.momentum_pow2)
                                                              / (inertia_y * (inertia_z - inertia_y)))
        self.elliptic_coef_angular_velocity_z = sign_z * sqrt((self.momentum_pow2 - self.energy_mul2 * inertia_x)

                                                              / (inertia_z * (inertia_z - inertia_x)))
        # Lifshitz eq (37.8)
        self.elliptic_tau = sqrt((inertia_z - inertia_y) * (self.momentum_pow2 - self.energy_mul2 * inertia_x)
                                 / (inertia_x * inertia_y * inertia_z))

        # Lifshitz eq (37.9)
        self.elliptic_modulus = (inertia_y - inertia_x) * (self.energy_mul2 * inertia_z - self.momentum_pow2) \
                                / ((inertia_z - inertia_y) * (self.momentum_pow2 - self.energy_mul2 * inertia_x))

    def __str__(self):
        result = ["Asteroid:"]
        keys = sorted([key for key in self.__dict__])
        for key in keys:
            result.append("{key}='{value}'".format(key=key, value=self.__dict__[key]))

        return "\n ".join(result)

    # Implementation of eq. (3.11a-c) from "EVALUATION OF NOVEL HOVERING STRATEGIES
    # TO IMPROVE GRAVITY-TRACTOR DEFLECTION MERITS" written by Dario Cersosimo,
    def gravity_at_position(self, position):
        from math import asin, sqrt
        from numpy import array, roots
        from constants import PI, GRAVITATIONAL_CONSTANT
        from scipy.special import ellipkinc, ellipeinc
        from sys import float_info

        gravity = [0, 0, 0]

        density = self.density
        axis_a = self.semi_axis_a
        axis_b = self.semi_axis_b
        axis_c = self.semi_axis_c
        axis_a_pow2 = self.semi_axis_a_pow2
        axis_b_pow2 = self.semi_axis_b_pow2
        axis_c_pow2 = self.semi_axis_c_pow2

        pos_x_pow2 = position[0] * position[0]
        pos_y_pow2 = position[1] * position[1]
        pos_z_pow2 = position[2] * position[2]

        coef_3 = 1.0
        coef_2 = -(pos_x_pow2 + pos_y_pow2 + pos_z_pow2 - axis_a_pow2 - axis_b_pow2 - axis_c_pow2)
        coef_1 = -(axis_b_pow2 * pos_x_pow2 + axis_c_pow2 * pos_x_pow2
                   + axis_a_pow2 * pos_y_pow2 + axis_c_pow2 * pos_y_pow2
                   + axis_a_pow2 * pos_z_pow2 + axis_b_pow2 * pos_z_pow2
                   - axis_a_pow2 * axis_c_pow2 - axis_b_pow2 * axis_c_pow2 - axis_a_pow2 * axis_b_pow2)
        coef_0 = -(axis_b_pow2 * axis_c_pow2 * pos_x_pow2
                   + axis_a_pow2 * axis_c_pow2 * pos_y_pow2
                   + axis_a_pow2 * axis_b_pow2 * pos_z_pow2
                   - axis_a_pow2 * axis_b_pow2 * axis_c_pow2)

        poly_coefs = array([coef_3, coef_2, coef_1, coef_0])
        kappa = float_info.min
        for root in roots(poly_coefs):
            if root > kappa:
                kappa = root

        val_sin = (axis_a_pow2 - axis_c_pow2) / (kappa + axis_a_pow2)
        if val_sin > 1.0:
            val_sin = 1.0
        elif val_sin < -1.0:
            val_sin = 1.0
        phi = asin(sqrt(val_sin))

        k = (axis_a_pow2 - axis_b_pow2) / (axis_a_pow2 - axis_c_pow2)

        integral_F = ellipkinc(phi, k)
        integral_E = ellipeinc(phi, k)

        gamma = 4.0 * PI * GRAVITATIONAL_CONSTANT * density * axis_a * axis_b * axis_c \
                / sqrt(axis_a_pow2 - axis_c_pow2)

        delta = sqrt((axis_a_pow2 - axis_c_pow2)
                     / ((axis_a_pow2 + kappa) * (axis_b_pow2 + kappa) * (axis_c_pow2 + kappa)))

        gravity[0] = gamma / (axis_a_pow2 - axis_b_pow2) * (integral_E - integral_F)
        gravity[1] = gamma * ((-axis_a_pow2 + axis_c_pow2) * integral_E
                                   / ((axis_a_pow2 - axis_b_pow2) * (axis_b_pow2 - axis_c_pow2))
                                   + integral_F / (axis_a_pow2 - axis_b_pow2)
                                   + delta * (axis_c_pow2 + kappa) / (axis_b_pow2 - axis_c_pow2))

        gravity[2] = gamma / (axis_c_pow2 - axis_b_pow2) * (-integral_E + (axis_b_pow2 + kappa) * delta)

        for i in range(3):
            gravity[i] *= position[i]

        return gravity

    # compute w
    def angular_velocity_and_acceleration_at_time(self, time):
        from scipy.special import ellipj

        # Add time bias
        time += self.time_bias

        # Multiply by tau
        time *= self.elliptic_tau

        # Get analytical solution
        sn_tau, cn_tau, dn_tau, _ = ellipj(time, self.elliptic_modulus)

        # Cache new values
        if self._inversion:
            angular_velocity = [self.elliptic_coef_angular_velocity_z * dn_tau,
                                self.elliptic_coef_angular_velocity_y * sn_tau,
                                self.elliptic_coef_angular_velocity_x * cn_tau]
        else:
            angular_velocity = [self.elliptic_coef_angular_velocity_x * cn_tau,
                                self.elliptic_coef_angular_velocity_y * sn_tau,
                                self.elliptic_coef_angular_velocity_z * dn_tau]

        inertia_x = self.inertia_x
        inertia_y = self.inertia_y
        inertia_z = self.inertia_z

        # Lifshitz eq (36.5)
        angular_acceleration = [(inertia_y - inertia_z) * angular_velocity[1] * angular_velocity[2] / inertia_x,
                                (inertia_z - inertia_x) * angular_velocity[2] * angular_velocity[0] / inertia_y,
                                (inertia_x - inertia_y) * angular_velocity[0] * angular_velocity[1] / inertia_z]

        return angular_velocity, angular_acceleration

    def _nearest_point_on_ellipse_first_quadrant(self, semi_axis, position):
        from math import sqrt
        from scipy.optimize import bisect

        solution = [0.0, 0.0]

        semi_axis_0 = semi_axis[0]
        semi_axis_1 = semi_axis[1]
        pos_0 = position[0]
        pos_1 = position[1]

        if pos_1 > 0.0:
            if pos_0 > 0.0:
                semi_axis_pow2 = [val * val for val in semi_axis]
                semi_axis_mul_pos = [semi_axis[0] * position[0], semi_axis[1] * position[1]]
                lower_boundary = 0.0
                upper_boundary = sqrt(semi_axis_mul_pos[0] * semi_axis_mul_pos[0]
                                      + semi_axis_mul_pos[1] * semi_axis_mul_pos[1])

                def fun(time):
                    cur_position = [semi_axis_mul_pos[0] / (time + semi_axis_pow2[0]),
                                    semi_axis_mul_pos[1] / (time + semi_axis_pow2[1])]
                    return cur_position[0] * cur_position[0] + cur_position[1] * cur_position[1] - 1.0

                time = bisect(fun, lower_boundary, upper_boundary)
                solution = [semi_axis_pow2[0] * position[0] / (time + semi_axis_pow2[0]),
                            semi_axis_pow2[1] * position[1] / (time + semi_axis_pow2[1])]
            else:
                solution[0] = 0.0
                solution[1] = semi_axis_1
        else:
            denominator = semi_axis_0 * semi_axis_0 - semi_axis_1 * semi_axis_1
            semi_axis_mul_pos = semi_axis_0 * pos_0
            if semi_axis_mul_pos < denominator:
                semi_axis_div_denom = semi_axis_mul_pos / denominator
                semi_axis_div_denom_pow2 = semi_axis_div_denom * semi_axis_div_denom
                solution[0] = semi_axis_0 * semi_axis_div_denom
                solution[1] = semi_axis_1 * sqrt(abs(1.0 - semi_axis_div_denom_pow2))
            else:
                solution[0] = semi_axis_0
                solution[1] = 0.0


        distance = sqrt((position[0] - solution[0]) * (position[0] - solution[0])
                        + (position[1] - solution[1]) * (position[1] - solution[1]))
        return distance, solution

    def _nearest_point_on_ellipsoid_at_position_first_quadrant(self, semi_axis, position):
        from math import sqrt
        from scipy.optimize import bisect

        solution = [0.0, 0.0, 0.0]

        semi_axis_0 = semi_axis[0]
        semi_axis_1 = semi_axis[1]
        semi_axis_2 = semi_axis[2]
        pos_0 = position[0]
        pos_1 = position[1]
        pos_2 = position[2]

        if pos_2 > 0.0:
            if pos_1 > 0.0:
                if pos_0 > 0.0:
                    semi_axis_pow2 = [val * val for val in semi_axis]
                    semi_axis_mul_pos = [semi_axis[0] * position[0],
                                         semi_axis[1] * position[1],
                                         semi_axis[2] * position[2]]
                    lower_boundary = 0.0
                    upper_boundary = sqrt(semi_axis_mul_pos[0] * semi_axis_mul_pos[0]
                                          + semi_axis_mul_pos[1] * semi_axis_mul_pos[1]
                                          + semi_axis_mul_pos[2] * semi_axis_mul_pos[2])

                    def fun(time):
                        cur_position = [semi_axis_mul_pos[0] / (time + semi_axis_pow2[0]),
                                        semi_axis_mul_pos[1] / (time + semi_axis_pow2[1]),
                                        semi_axis_mul_pos[2] / (time + semi_axis_pow2[2])]
                        return cur_position[0] * cur_position[0] + cur_position[1] * cur_position[1] \
                               + cur_position[2] * cur_position[2] - 1.0

                    time = bisect(fun, lower_boundary, upper_boundary)
                    solution = [semi_axis_pow2[0] * position[0] / (time + semi_axis_pow2[0]),
                                semi_axis_pow2[1] * position[1] / (time + semi_axis_pow2[1]),
                                semi_axis_pow2[2] * position[2] / (time + semi_axis_pow2[2])]
                else:
                    solution[0] = 0.0
                    semi_axis_2d = semi_axis[1:3]
                    position_2d = position[1:3]
                    _, solution_2d = self._nearest_point_on_ellipse_first_quadrant(semi_axis_2d, position_2d)
                    solution[1] = solution_2d[0]
                    solution[2] = solution_2d[1]
            else:
                solution[1] = 0.0
                if pos_0 > 0.0:
                    semi_axis_2d = [semi_axis_0, semi_axis_2]
                    position_2d = [pos_0, pos_2]
                    _, solution_2d = self._nearest_point_on_ellipse_first_quadrant(semi_axis_2d, position_2d)
                    solution[0] = solution_2d[0]
                    solution[2] = solution_2d[1]
                else:
                    solution[0] = 0.0
                    solution[2] = semi_axis_2
        else:
            denominator = [semi_axis_0 * semi_axis_0 - semi_axis_2 * semi_axis_2,
                           semi_axis_1 * semi_axis_1 - semi_axis_2 * semi_axis_2]
            semi_axis_mul_pos = [semi_axis[i] * position[i] for i in range(2)]
            if semi_axis_mul_pos[0] < denominator[0] and semi_axis_mul_pos[1] < denominator[1]:
                semi_axis_div_denom = [semi_axis_mul_pos[i] / denominator[i] for i in range(2)]
                semi_axis_div_denom_pow2 = [val*val for val in semi_axis_div_denom]
                discr = 1.0 - semi_axis_div_denom_pow2[0] - semi_axis_div_denom_pow2[1]
                if discr > 0.0:
                    solution[0] = semi_axis_0 * semi_axis_div_denom[0]
                    solution[1] = semi_axis_1 * semi_axis_div_denom[1]
                    solution[2] = semi_axis_2 * sqrt(discr)
                else:
                    solution[2] = 0.0
                    semi_axis_2d = [semi_axis_0, semi_axis_1]
                    position_2d = [pos_0, pos_1]
                    _, solution_2d = self._nearest_point_on_ellipse_first_quadrant(semi_axis_2d, position_2d)
                    solution[0] = solution_2d[0]
                    solution[1] = solution_2d[1]
            else:
                solution[2] = 0.0
                semi_axis_2d = [semi_axis_0, semi_axis_1]
                position_2d = [pos_0, pos_1]
                _, solution_2d = self._nearest_point_on_ellipse_first_quadrant(semi_axis_2d, position_2d)
                solution[0] = solution_2d[0]
                solution[1] = solution_2d[1]

        distance = sqrt((position[0] - solution[0]) * (position[0] - solution[0])
                        + (position[1] - solution[1]) * (position[1] - solution[1])
                        + (position[2] - solution[2]) * (position[2] - solution[2]))
        return distance, solution

    def distance_to_surface_at_position(self, position):
        '''
            Port of Implementation from "Distance from a Point to an Ellipse, an Ellipsoid, or a Hyperellipsoid" by David Eberly, 2013.
        '''
        from math import copysign

        semi_axis = [self.semi_axis_a, self.semi_axis_b, self.semi_axis_c]
        signs = [copysign(1.0, val) for val in position]

        abs_position = [abs(var) for var in position]

        distance, surface_position = self._nearest_point_on_ellipsoid_at_position_first_quadrant(semi_axis, abs_position)

        surface_position = [signs[0] * surface_position[0],
                            signs[1] * surface_position[1],
                            signs[2] * surface_position[2]]

        return distance, surface_position