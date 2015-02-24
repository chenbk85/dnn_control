#ifndef EVOLUTIONARYROBOTICS_H
#define EVOLUTIONARYROBOTICS_H

#include <pagmo/src/pagmo.h>

void TrainNeuralNetworkController();
void TestNeuralNetworkController(const unsigned int &random_seed);

void TrainProportionalDerivativeController();
void TestProportionalDerivativeController(const unsigned int &random_seed);

void TestNeuralNetworkVSFullStateController(const unsigned int &random_seed);

#endif // EVOLUTIONARYROBOTICS_H
