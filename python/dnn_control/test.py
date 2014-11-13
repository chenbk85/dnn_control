import matplotlib.pyplot as pyplot
from mpl_toolkits.mplot3d import Axes3D
from numpy import random
from numpy.linalg import norm
from asteroid import Asteroid
from scipy.integrate import odeint
from constants import PI
from math import fabs
from sys import float_info

'''
def w_dot(state, time, inertia_x, inertia_y, inertia_z):
    return [(inertia_y - inertia_z) * state[1] * state[2] / inertia_x, (inertia_z - inertia_x) * state[2] * state[0] / inertia_y, (inertia_x - inertia_y) * state[0] * state[1] / inertia_z]

INERTIA_Z = 4567.123  # [kg*m^2]
INERTIA_Y = 2345.3456  # [kg*m^2]
INERTIA_X = 1234.12  # [kg*m^2]
DENSITY = 2000.0  # [kg/m^3]
ANGULAR_VELOCITY = [0.0005, 0.0, 0.0003]  # [1/s]
TIME_BIAS = 0.0  # [s]
asteroid = Asteroid(INERTIA_X, INERTIA_Y, INERTIA_Z, DENSITY, ANGULAR_VELOCITY, TIME_BIAS)


test_time = 8000

result = odeint(
    w_dot, ANGULAR_VELOCITY, [0, test_time], (INERTIA_X, INERTIA_Y, INERTIA_Z))
omega_odeint = result[1][:]
omega_analytical = asteroid.angular_velocity_at_time(test_time)


print("{0:.10f} {1:.10f} {2:.10f}".format(omega_odeint[0],omega_odeint[1],omega_odeint[2]))
print("{0:.10f} {1:.10f} {2:.10f}".format(omega_analytical[0],omega_analytical[1],omega_analytical[2]))

'''


def w_dot(state, time, inertia_x, inertia_y, inertia_z):
    return [(inertia_y - inertia_z) * state[1] * state[2] / inertia_x, (inertia_z - inertia_x) * state[2] * state[0] / inertia_y, (inertia_x - inertia_y) * state[0] * state[1] / inertia_z]

min_error = float_info.max
max_error = float_info.min
avg_error = 0.0

for i in range(1000):
    print("Test run {0}".format(i + 1))

    test_time = random.uniform(0.0, 100.0)
    inertia_z = random.uniform(1000.0, 2000.0)
    inertia_y = random.uniform(inertia_z + 1.0, inertia_z + 4000.0)
    inertia_x = random.uniform(inertia_y + 1.0, inertia_y + 4000.0)
    density = 2000.0
    angular_velocity = [random.uniform(
        -2.0 * PI, 2.0 * PI), 0.0, random.uniform(-2.0 * PI, 2.0 * PI)]

    asteroid = Asteroid(
        inertia_x, inertia_y, inertia_z, density, angular_velocity, 0.0)

    result = odeint(
        w_dot, angular_velocity, [0, test_time], (inertia_x, inertia_y, inertia_z))
    omega_numerical = result[1][:]
    omega_analytical = asteroid.angular_velocity_at_time(test_time)

    error = norm(omega_numerical - omega_analytical)
    avg_error += error
    if error < min_error:
        min_error = error
    elif error > max_error:
        max_error = error

print(min_error)
print(max_error)
print(avg_error/1000.0)



'''
TIME = 100000.0
SAMPLING_FREQUENCY = 10.0
INERTIA_X = 4567.123  # [kg*m^2]
INERTIA_Y = 2345.3456  # [kg*m^2]
INERTIA_Z = 1234.12  # [kg*m^2]
DENSITY = 2000.0  # [kg/m^3]
ANGULAR_VELOCITY = [0.0005, 0.0007, -0.0003]  # [1/s]
TIME_BIAS = 0.0  # [s]

asteroid = Asteroid(INERTIA_X, INERTIA_Y, INERTIA_Z, DENSITY, ANGULAR_VELOCITY, TIME_BIAS)

iterations = int(TIME*SAMPLING_FREQUENCY)

angular_velocity = empty([iterations,3])
for i in range(iterations):
    time = i*1.0/SAMPLING_FREQUENCY
    omega = asteroid.angular_velocity_at_time(time)
    angular_velocity[i][:] = omega

# Visualize trajectory
fig = pyplot.figure()
ax = fig.gca(projection="3d")
ax.plot(angular_velocity[:,0],angular_velocity[:,1],angular_velocity[:,2], label="angular velocity")
pyplot.plot([angular_velocity[0][0]],[angular_velocity[0][1]],[angular_velocity[0][2]], 'rD', label="start")
pyplot.plot([angular_velocity[-1][0]],[angular_velocity[-1][1]],[angular_velocity[-1][2]], 'bD', label="end")
ax.legend()
ax.set_xlabel('Omega_x')
ax.set_ylabel('Omega_y')
ax.set_zlabel('Omega_z')
pyplot.show()

'''
