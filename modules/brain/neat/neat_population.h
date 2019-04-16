#ifndef NEAT_POPULATION_H
#define NEAT_POPULATION_H

#include "neat.h"
#include "thirdparty/brain/brain/NEAT/neat_population.h"

class NeatPopulation : public Neat {

	GDCLASS(NeatPopulation, Neat);

	brain::NtPopulationSettings settings;
	brain::NtPopulation *population;

public:
	NeatPopulation();
};

#endif // NEAT_POPULATION_H
