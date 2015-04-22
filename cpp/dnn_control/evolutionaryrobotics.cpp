#include "evolutionaryrobotics.h"
#include "hoveringproblemneuralnetwork.h"
#include "samplefactory.h"
#include "filewriter.h"
#include "configuration.h"
#include <sstream>

// Training configuration
static const unsigned int kNumGenerations = ER_NUM_GENERATIONS;
static const unsigned int kPopulationSize = ER_POPULATION_SIZE;
static const unsigned int kNumIslands = ER_NUM_ISLANDS;
#ifdef ER_SIMULATION_TIME
static const double kSimulationTime = ER_SIMULATION_TIME;
#else
static const double kSimulationTime = 0.0;
#endif
static const unsigned int kNumEvaluations = ER_EVALUATIONS;
static const unsigned int kNumHiddenNeurons = ER_NUM_HIDDEN_NODES;
static const unsigned int kEarlyStoppingTestInterval = 10;
static const unsigned int kNumEarlyStoppingTests = 100;
static const unsigned int kNumEarlyStoppingDelay = 5;


static unsigned int ArchipelagoChampionID(pagmo::archipelago archi) {
    double min = archi.get_island(0)->get_population().champion().f[0];
    unsigned int idx = 0;
    for (unsigned int i = 1; i < archi.get_size(); ++i) {
        double cur = archi.get_island(i)->get_population().champion().f[0];
        if (cur < min) {
            min = cur;
            idx = i;
        }
    }
    return idx;
}

static void ArchipelagoEvolve(pagmo::archipelago &archi, const pagmo::problem::hovering_problem_neural_network &prob, const unsigned int &num_generations, const unsigned int &early_stopping_test_interval, const unsigned int &num_early_stopping_tests, const unsigned int &early_stopping_delay) {
    // Buffer
    std::vector<double> buff;

    //Evolution is here started on the archipelago

    unsigned int generations = 0;
    double avg_error = std::numeric_limits<double>::max();
    unsigned int worse = 0;
    pagmo::decision_vector champion;
    for (unsigned int i = 0; i< num_generations; ++i){
        const unsigned int idx = ArchipelagoChampionID(archi);
        double best_f = archi.get_island(idx)->get_population().champion().f[0];

        if (i<50) {
            buff.push_back(best_f);
        }
        else {
            (buff[i%50] = best_f);
        }
        double mean = 0.0;
        mean = std::accumulate(buff.begin(),buff.end(),mean);
        mean /= (double)buff.size();

        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        std::cout << std::endl << asctime(timeinfo) << "generation: "<< std::setw(20) << i << std::setw(20) <<
                     best_f << std::setw(20) <<
                     archi.get_island(idx)->get_population().mean_velocity() << std::setw(20) <<
                     mean <<	 std::endl << "[";
        const pagmo::decision_vector x = archi.get_island(idx)->get_population().champion().x;
        for (unsigned int i = 0; i < x.size() -1; ++i) {
            std::cout << x[i] << ", ";
        }
        std::cout << x.back() << "]" << std::endl;
        fflush(stdout);
        archi.evolve(1);

        generations++;
        if (generations == early_stopping_test_interval) {
            generations = 0;
            std::cout << std::endl << ">> early stopping test ... ";
            fflush(stdout);
            std::vector<unsigned int> random_seeds;
            for (unsigned int j = 0; j < num_early_stopping_tests; ++j) {
                random_seeds.push_back(rand());
            }

            double cur_avg_error = 0.0;
            for (unsigned int j = 0; j < num_early_stopping_tests; ++j) {
                cur_avg_error += prob.objfun_seeded(random_seeds.at(j), x)[0];
            }
            cur_avg_error /= num_early_stopping_tests;
            std::cout << "average error: " << cur_avg_error << "/" << avg_error << " patience: ";
            fflush(stdout);
            if (cur_avg_error > avg_error) {
                worse++;
            } else {
                avg_error = cur_avg_error;
                worse = 0;
                champion = x;
            }

            std::cout << worse << "/" << early_stopping_delay << std::endl;
            if (worse == early_stopping_delay) {
                break;
            }
        }
    }

    std::cout << std::endl << "And the winner is ......" << std::endl << "[";
    for (unsigned int i = 0; i < champion.size() -1; ++i) {
        std::cout << champion[i] << ", ";
    }
    std::cout << champion.back() << "]" << std::endl;


    std::cout << std::endl << "Copy this into C++ code:" << std::endl << "{";
    for (unsigned int i = 0; i < champion.size() -1; ++i) {
        std::cout << champion[i] << ", ";
    }
    std::cout << champion.back() << "};" << std::endl;
}

void TrainNeuralNetworkController() {
    ConfigurationPaGMO();

    srand(time(0));

    std::cout << std::setprecision(10);

    // We instantiate a PSO algorithm capable of coping with stochastic prolems
    pagmo::algorithm::pso_generational algo(1,0.7298,2.05,2.05,0.05);

    std::cout << "Initializing NN controller evolution ....";

    pagmo::archipelago archi = pagmo::archipelago(pagmo::topology::fully_connected());

    for (unsigned int j = 0;j < kNumIslands; ++j) {
        std::cout << " [" << j;
        fflush(stdout);
        pagmo::problem::hovering_problem_neural_network prob(rand(), kNumEvaluations, kSimulationTime, kNumHiddenNeurons);

        // This instantiates a population within the original bounds (-1,1)
        pagmo::population pop_temp(prob, kPopulationSize);

        // We make the bounds larger to allow neurons weights to grow
        prob.set_bounds(-30.0,30.0);

        // We create an empty population on the new prolem (-10,10)
        pagmo::population pop(prob);

        // And we fill it up with (-1,1) individuals having zero velocities
        pagmo::decision_vector v(prob.get_dimension(),0);
        for (unsigned int i = 0; i < kPopulationSize; ++i) {
            pop.push_back(pop_temp.get_individual(i).cur_x);
            pop.set_v(i,v);
        }
        archi.push_back(pagmo::island(algo,pop));
        std::cout << "]";
        fflush(stdout);
    }

    std::cout << " done" << std::endl << "Evolving ..." << std::endl;
    fflush(stdout);

    pagmo::problem::hovering_problem_neural_network prob(rand(), kNumEvaluations, kSimulationTime, kNumHiddenNeurons);
    ArchipelagoEvolve(archi, prob, kNumGenerations, kEarlyStoppingTestInterval, kNumEarlyStoppingTests, kNumEarlyStoppingDelay);
}

static void ConvexityCheck(pagmo::problem::hovering_problem_neural_network &problem, const unsigned &random_seed, const pagmo::decision_vector &x) {
    const double d_range = 0.01;
    const double range = 5.0;

    std::cout << "Checking NN controller convexity... " << std::endl;

    for (unsigned int dimension = 0; dimension < x.size(); ++dimension) {
        std::vector<std::pair<double,double> > fitness;
        pagmo::decision_vector x_copy(x);
        double weight = -range;
        while (weight <= range) {
            std::cout << weight << std::endl;
            x_copy.at(dimension) = weight;
            fitness.push_back(std::make_pair(weight, problem.objfun_seeded(random_seed, x_copy)[0]));
            weight += d_range;
        }
        std::cout << "Writing convexity file ... ";
        std::string path(PATH_TO_NEURO_CONVEXITY_PATH);
        std::stringstream ss;
        ss << "dim_" << dimension << ".txt";
        path += ss.str();
        FileWriter writer(path);
        writer.CreateConvexityFile(random_seed, dimension, fitness);
        std::cout << "done." << std::endl;
    }

    std::cout << "done." << std::endl;
}

void TestNeuralNetworkController(const unsigned int &random_seed) {
    ConfigurationPaGMO();

    const unsigned int worst_case_seed = random_seed;

    const pagmo::decision_vector &solution = {-0.01236501794, 1.373674969, -3.197310572, -2.490554383, 2.453852009, -4.972314813, -6.598451767, -5.188087775, -2.722995107, 2.232155309, 3.661709141, -9.902712683, -0.3154268727, 1.398515551, -8.685090773, -9.374793163, -11.17103997, -4.751219269, -4.701791396, -4.794371852, -1.525313484, -3.015671809, -3.560177245, -6.163869436, 2.497946839, -3.477202092, 1.403686443, 0.6162124201, -4.004739968, -0.1206160496, 1.399136044, 6.906672731, -0.695947657, -3.780636076, -3.071320222, -9.12713488, -3.589677596, -5.016644352, -7.851863408, -0.0004941009082, -3.70432747, -1.114502301, -1.693606465, -2.329674187, -0.2206568153, -2.717581552, -3.035086962, -4.730028639, 0.3308702739, -8.797471172, -1.271785732, 4.334976447, -5.6959147, -3.515380831, 3.201445998, -0.002169785776, -0.2491572599, 4.739965454, -6.463084083, 3.964758199, 5.681357428, -4.352092371, 5.267536242, 0.002403957107, -1.796462134, 1.732403205, -3.758573536, -7.370266028, 1.172091337, -0.2512033314, -5.893892145, -6.86598965, -1.36281577, 0.6503813815, 0.6440064148, -3.785136653, 1.223503159, -1.290324742, -7.581969458, -5.963972729, -2.691818473, -3.435996685, 0.998554493, 1.584152781, 2.479707462, 0.1051838902, 0.3846289467, 0.4391482297, -0.1517717938, -0.9817775973, -0.497143465, 0.3770336102, 4.394750232, -6.396495348, 6.624984575, -0.7378955139, 0.301536429, 1.219418319, 0.02567380436, 0.4751797856, 4.855908526, 1.817925182, -0.4669428669, 2.327927113, 4.216168658, 7.326928324, 0.6066953586, 0.276441439, -1.500775995, -0.292159858, -0.1855277052, 2.581046452, 4.221784404};

    std::cout << std::setprecision(10);

    pagmo::problem::hovering_problem_neural_network prob(random_seed, kNumEvaluations, kSimulationTime, kNumHiddenNeurons);

    std::cout << "Checking NN controller fitness... ";
    const double fitness = prob.objfun_seeded(random_seed, solution)[0];
    std::cout << fitness << std::endl;

    std::cout << "Simulating NN controller ... ";
    PaGMOSimulationNeuralNetwork simulation(worst_case_seed, kNumHiddenNeurons, solution);
    simulation.SetSimulationTime(2.0 * 86400.0);
    const boost::tuple<std::vector<double>, std::vector<double>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<Vector3D>, std::vector<std::vector<double> > > result = simulation.EvaluateAdaptive();
    const std::vector<double> &times = boost::get<0>(result);
    const std::vector<Vector3D> &positions = boost::get<2>(result);
    const std::vector<Vector3D> &heights = boost::get<3>(result);
    const std::vector<Vector3D> &velocities = boost::get<4>(result);
    const std::vector<Vector3D> &thrusts = boost::get<5>(result);
    std::cout << "done." << std::endl;

    std::cout << "Writing visualization file ... ";
    FileWriter writer_visualization(PATH_TO_NEURO_TRAJECTORY_FILE);
    writer_visualization.CreateTrajectoryFile(simulation.ControlFrequency(), simulation.AsteroidOfSystem(), positions, heights);
    std::cout << "done." << std::endl;

    std::cout << "Writing evaluation file ... ";
    FileWriter writer_evaluation(PATH_TO_NEURO_EVALUATION_FILE);
    writer_evaluation.CreateEvaluationFile(random_seed, simulation.TargetPosition(), simulation.AsteroidOfSystem(), times, positions, velocities, thrusts);
    std::cout << "done." << std::endl;

    std::cout << "Performing post evaluation ... ";
    const boost::tuple<std::vector<unsigned int>, std::vector<double>, std::vector<std::pair<double, double> > > post_evaluation = prob.post_evaluate(solution, random_seed);
    const std::vector<unsigned int> &random_seeds = boost::get<0>(post_evaluation);
    const std::vector<double> &mean_errors = boost::get<1>(post_evaluation);
    const std::vector<std::pair<double, double> > &min_max_errors = boost::get<2>(post_evaluation);

    std::cout << "done." << std::endl << "Writing post evaluation file ... ";
    FileWriter writer_post_evaluation(PATH_TO_NEURO_POST_EVALUATION_FILE);
    writer_post_evaluation.CreatePostEvaluationFile(random_seeds, mean_errors, min_max_errors);
    std::cout << "done." << std::endl;
}
