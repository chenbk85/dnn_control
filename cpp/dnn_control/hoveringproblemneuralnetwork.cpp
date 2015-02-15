#include "hoveringproblemneuralnetwork.h"
#include "configuration.h"

namespace pagmo { namespace problem {

hovering_problem_neural_network::hovering_problem_neural_network(const unsigned int &seed, const unsigned int &n_evaluations, const double &simulation_time, const unsigned int &n_hidden_neurons)
    : base_stochastic(PaGMOSimulationNeuralNetwork(0, 0.0, n_hidden_neurons).ChromosomeSize(), seed),
      m_n_evaluations(n_evaluations), m_n_hidden_neurons(n_hidden_neurons), m_simulation_time(simulation_time) {

    set_lb(-1.0);
    set_ub(1.0);
}

hovering_problem_neural_network::hovering_problem_neural_network(const hovering_problem_neural_network &other)
    : base_stochastic(other) {
    m_n_evaluations = other.m_n_evaluations;
    m_simulation_time = other.m_simulation_time;
    m_n_hidden_neurons = other.m_n_hidden_neurons;
}

std::string hovering_problem_neural_network::get_name() const {
    return "Asteroid hovering - Neurocontroller Evolution";
}

base_ptr hovering_problem_neural_network::clone() const {
    return base_ptr(new hovering_problem_neural_network(*this));
}

fitness_vector hovering_problem_neural_network::objfun_seeded(const unsigned int &seed, const decision_vector &x) const {
    PaGMOSimulationNeuralNetwork simulation(seed, m_simulation_time, m_n_hidden_neurons, x);
    fitness_vector f(1);
    f[0] = single_fitness(simulation);
    return f;
}

void hovering_problem_neural_network::objfun_impl(fitness_vector &f, const decision_vector &x) const {
    f[0] = 0.0;

    // Make sure the pseudorandom sequence will always be the same
    m_urng.seed(m_seed);

    for (unsigned int count = 0; count < m_n_evaluations; count++) {

        // Creates the initial conditions at random, based on the current seed
#ifdef HP_FIXED_SEED
        const unsigned int current_seed = HP_FIXED_SEED;
#else
        const unsigned int current_seed = m_urng();
#endif

        // Neural Network simulation
        PaGMOSimulationNeuralNetwork simulation(current_seed, m_simulation_time, m_n_hidden_neurons, x);
        f[0] += single_fitness(simulation);
    }
    f[0] /= m_n_evaluations;
}

std::string hovering_problem_neural_network::human_readable_extra() const {
    std::ostringstream oss;
    oss << "\tSimulation Time: " << m_simulation_time << '\n';
    oss << "\tSeed: " << m_seed << '\n';
    oss << "\tSample Size: " << m_n_evaluations << '\n';
    oss << "\tHidden Neurons: " << m_n_hidden_neurons << '\n';
    return oss.str();
}

double hovering_problem_neural_network::single_fitness(PaGMOSimulationNeuralNetwork &simulation) const {
    double fitness = 0.0;

    const boost::tuple<std::vector<double>, std::vector<double>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D> > result = simulation.EvaluateAdaptive();
    const std::vector<double> &evaluated_times = boost::get<0>(result);
    const std::vector<double> &evaluated_masses = boost::get<1>(result);
    const std::vector<Vector3D> &evaluated_positions = boost::get<2>(result);
    const std::vector<Vector3D> &evaluated_heights = boost::get<3>(result);
    const std::vector<Vector3D> &evaluated_velocities = boost::get<4>(result);

    const unsigned int num_samples = evaluated_times.size();

#if HP_TARGET_TYPE == HP_TT_TARGET
    // The target position
    const Vector3D target_position = simulation.TargetPosition();
#endif

    // punish unfinished simulations (crash / out of fuel)
#if HP_OBJ_FUN_PUNISH_UNFINISHED_SIMULATIONS_ENABLED
    double time_diff = evaluated_times.back() - m_simulation_time;
    time_diff = (time_diff < 0.0 ? -time_diff : time_diff);
    if (time_diff > 0.1) {
        fitness += 1e30;
    }
#endif

#if HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_1
    // Method 1 : Compare start and ending position and velocity
    const Vector3D &position_end = evaluated_positions.back();
    const Vector3D &velocity_end = evaluated_velocities.back();
    fitness += VectorNorm(VectorSub(target_position, position_end)) + VectorNorm(velocity_end);

#elif HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_2
    // Method 2 : Compare mean distance to target point
    for (unsigned int i = 0; i < num_samples; ++i) {
        fitness += VectorNorm(VectorSub(target_position, evaluated_positions.at(i)));
    }
    fitness /= num_samples;

#elif HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_3
    // Method 3 : Compare mean distance to target point, also consider velocity
    for (unsigned int i = 0; i < num_samples; ++i) {
        fitness += VectorNorm(VectorSub(target_position, evaluated_positions.at(i))) + VectorNorm(evaluated_velocities.at(i));
    }
    fitness /= num_samples;

#elif HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_4
    // Method 4 : Compare mean distance to target point, but don't take into consideration some amount of starting positions
    const unsigned int start_index = num_samples * 0.01;
    for (unsigned int i = start_index; i < num_samples; ++i) {
        fitness += VectorNorm(VectorSub(target_position, evaluated_positions.at(i)));
    }
    fitness /= (num_samples - start_index);

#elif HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_5
    // Method 5 : Compare mean distance to target point, but don't take into consideration some amount of starting positions.
    // Additionally, take into consideration total fuel consumption
    const unsigned int start_index = num_samples * 0.01;
    for (unsigned int i = start_index; i < num_samples; ++i) {
        fitness += VectorNorm(VectorSub(target_position, evaluated_positions.at(i)));
    }
    fitness /= (num_samples - start_index);
    fitness += 1.0 / (evaluated_masses.back() - simulation.SpacecraftMinimumMass() + 0.001);

#elif HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_6
    // Method 6 : Compare mean distance to target point, also consider velocity, but don't take into consideration some amount of starting positions.
    const unsigned int start_index = num_samples * 0.01;
    for (unsigned int i = start_index; i < num_samples; ++i) {
        fitness += VectorNorm(VectorSub(target_position, evaluated_positions.at(i))) + VectorNorm(evaluated_velocities.at(i));
    }
    fitness /= num_samples - start_index;

#elif HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_7
    // Method 7 : Compare mean distance to target point, also consider velocity, punish later offsets more
    for (unsigned int i = 0; i < num_samples; ++i) {
        fitness += (i + 1) * (VectorNorm(VectorSub(target_position, evaluated_positions.at(i))) + VectorNorm(evaluated_velocities.at(i)));
    }
    fitness /= num_samples;

#elif HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_8
    // Method 8 : Compare height changes compared to initial height.
    const double height = VectorNorm(evaluated_heights.at(0));
    for (unsigned int i = 1; i < num_samples; ++i) {
        double cur_error = height - VectorNorm(evaluated_heights.at(i));
        cur_error = (cur_error < 0.0 ? -cur_error : cur_error);
        fitness += cur_error;
    }
    fitness /= num_samples - 1;
#endif

    return fitness;
}

double hovering_problem_neural_network::single_post_evaluation(PaGMOSimulationNeuralNetwork &simulation) const {
    double fitness = 0.0;

    const boost::tuple<std::vector<double>, std::vector<double>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D> > result = simulation.EvaluateAdaptive();
    const std::vector<double> &evaluated_times = boost::get<0>(result);
    const std::vector<Vector3D> &evaluated_positions = boost::get<2>(result);
    const std::vector<Vector3D> &evaluated_heights = boost::get<3>(result);

    const unsigned int num_samples = evaluated_times.size();

    // punish unfinished simulations (crash / out of fuel)
#if HP_OBJ_FUN_PUNISH_UNFINISHED_SIMULATIONS_ENABLED
    double time_diff = evaluated_times.back() - m_simulation_time;
    time_diff = (time_diff < 0.0 ? -time_diff : time_diff);
    if (time_diff > 0.1) {
        fitness += 1e30;
    }
#endif

#if HP_OBJECTIVE_FUNCTION_METHOD == HP_OBJ_FUN_METHOD_8
    const double height = VectorNorm(evaluated_heights.at(0));
    for (unsigned int i = 1; i < num_samples; ++i) {
        double cur_error = height - VectorNorm(evaluated_heights.at(i));
        cur_error = (cur_error < 0.0 ? -cur_error : cur_error);
        fitness += cur_error;
    }
    fitness /= num_samples - 1;
#else
    // The target position
    const Vector3D target_position = simulation.TargetPosition();
    const unsigned int start_index = num_samples * 0.01;
    for (unsigned int i = start_index; i < num_samples; ++i) {
        fitness += VectorNorm(VectorSub(target_position, evaluated_positions.at(i)));
    }

    fitness /= (num_samples - start_index);
#endif

    return fitness;
}

hovering_problem_neural_network::~hovering_problem_neural_network() {

}

boost::tuple<std::vector<double>, std::vector<unsigned int> > hovering_problem_neural_network::post_evaluate(const decision_vector &x, const unsigned int &start_seed, const std::vector<unsigned int> &random_seeds) const {
    unsigned int num_tests = random_seeds.size();
    std::vector<unsigned int> used_random_seeds;
    if (num_tests == 0) {
        m_urng.seed(start_seed);
        num_tests = 25000;
        for (unsigned int i = 0; i < num_tests; ++i) {
            used_random_seeds.push_back(m_urng());
        }
    } else {
        used_random_seeds = random_seeds;
    }

    std::vector<double> fitness(num_tests, 0.0);

    for (unsigned int i = 0; i < num_tests; ++i) {
        const unsigned int current_seed = used_random_seeds.at(i);
        PaGMOSimulationNeuralNetwork simulation(current_seed, m_simulation_time, m_n_hidden_neurons, x);
        fitness.at(i) = single_post_evaluation(simulation);
    }

    return boost::make_tuple(fitness, used_random_seeds);
}

}}
