def cross_product(vector_a, vector_b):
    vector_c = [vector_a[1] * vector_b[2] - vector_a[2] * vector_b[1], vector_a[2] *
                vector_b[0] - vector_a[0] * vector_b[2],
                vector_a[0] * vector_b[1] - vector_a[1] * vector_b[0]]
    return vector_c


def sample_position_outside_ellipse(semi_axis_a, semi_axis_b, band_width, num_samples):
    from numpy import random
    from math import cos, sin
    from constants import PI

    samples = []

    for i in range(num_samples):
        theta = random.uniform(0.0, 2.0*PI)
        x = random.uniform(semi_axis_a, semi_axis_a + band_width) * cos(theta)
        y = random.uniform(semi_axis_b, semi_axis_b + band_width) * sin(theta)
        samples.append([x, y])

    return samples


def sample_position_outside_ellipsoid(semi_axis_a, semi_axis_b, semi_axis_c, band_width, num_samples):
    from numpy import random
    from math import cos, sin
    from constants import PI

    samples = []

    for i in range(num_samples):
        u = random.uniform(0.0, 2.0*PI)
        v = random.uniform(0.0, PI)
        x = random.uniform(semi_axis_a, semi_axis_a + band_width) * cos(u) * sin(v)
        y = random.uniform(semi_axis_b, semi_axis_b + band_width) * sin(u) * sin(v)
        z = random.uniform(semi_axis_c, semi_axis_c + band_width) * cos(v)
        samples.append([x, y, z])

    return samples