#ifndef NEAT_POPULATION_H
#define NEAT_POPULATION_H

#include "neat.h"
#include "thirdparty/brain/brain/NEAT/neat_population.h"

class NeatPopulation : public Neat {

	GDCLASS(NeatPopulation, Neat);

	brain::NtPopulationSettings settings;
	brain::NtPopulation *population;

	static void _bind_methods();

public:
	NeatPopulation();

	uint64_t seed = 1;

	void set_learning_deviation(real_t p_learning_deviation) { settings.learning_deviation = p_learning_deviation; }
	real_t get_learning_deviation() const { return settings.learning_deviation; }

	void set_genetic_compatibility_threshold(real_t p_genetic_compatibility_threshold) { settings.genetic_compatibility_threshold = p_genetic_compatibility_threshold; }
	real_t get_genetic_compatibility_threshold() const { return settings.genetic_compatibility_threshold; }

	void set_genetic_disjoints_significance(real_t p_genetic_disjoints_significance) { settings.genetic_disjoints_significance = p_genetic_disjoints_significance; }
	real_t get_genetic_disjoints_significance() const { return settings.genetic_disjoints_significance; }

	void set_genetic_excesses_significance(real_t p_genetic_excesses_significance) { settings.genetic_excesses_significance = p_genetic_excesses_significance; }
	real_t get_genetic_excesses_significance() const { return settings.genetic_excesses_significance; }

	void set_genetic_weights_significance(real_t p_genetic_weights_significance) { settings.genetic_weights_significance = p_genetic_weights_significance; }
	real_t get_genetic_weights_significance() const { return settings.genetic_weights_significance; }

	void set_genetic_mate_prob(real_t p_genetic_mate_prob) { settings.genetic_mate_prob = p_genetic_mate_prob; }
	real_t get_genetic_mate_prob() const { return settings.genetic_mate_prob; }

	void set_genetic_mate_inside_species_threshold(real_t p_genetic_mate_inside_species_threshold) { settings.genetic_mate_inside_species_threshold = p_genetic_mate_inside_species_threshold; }
	real_t get_genetic_mate_inside_species_threshold() const { return settings.genetic_mate_inside_species_threshold; }

	void set_genetic_mate_multipoint_threshold(real_t p_genetic_mate_multipoint_threshold) { settings.genetic_mate_multipoint_threshold = p_genetic_mate_multipoint_threshold; }
	real_t get_genetic_mate_multipoint_threshold() const { return settings.genetic_mate_multipoint_threshold; }

	void set_genetic_mate_multipoint_avg_threshold(real_t p_genetic_mate_multipoint_avg_threshold) { settings.genetic_mate_multipoint_avg_threshold = p_genetic_mate_multipoint_avg_threshold; }
	real_t get_genetic_mate_multipoint_avg_threshold() const { return settings.genetic_mate_multipoint_avg_threshold; }

	void set_genetic_mate_singlepoint_threshold(real_t p_genetic_mate_singlepoint_threshold) { settings.genetic_mate_singlepoint_threshold = p_genetic_mate_singlepoint_threshold; }
	real_t get_genetic_mate_singlepoint_threshold() const { return settings.genetic_mate_singlepoint_threshold; }

	void set_genetic_mutate_add_link_porb(real_t p_genetic_mutate_add_link_porb) { settings.genetic_mutate_add_link_porb = p_genetic_mutate_add_link_porb; }
	real_t get_genetic_mutate_add_link_porb(real_t p_genetic_mutate_add_link_porb) const { return settings.genetic_mutate_add_link_porb; }

	void set_genetic_mutate_add_node_prob(real_t p_genetic_mutate_add_node_prob) { settings.genetic_mutate_add_node_prob = p_genetic_mutate_add_node_prob; }
	real_t get_genetic_mutate_add_node_prob() const { return settings.genetic_mutate_add_node_prob; }

	void set_genetic_mutate_link_weight_prob(real_t p_genetic_mutate_link_weight_prob) { settings.genetic_mutate_link_weight_prob = p_genetic_mutate_link_weight_prob; }
	real_t get_genetic_mutate_link_weight_prob() const { return settings.genetic_mutate_link_weight_prob; }

	void set_genetic_mutate_link_weight_uniform_prob(real_t p_genetic_mutate_link_weight_uniform_prob) { settings.genetic_mutate_link_weight_uniform_prob = p_genetic_mutate_link_weight_uniform_prob; }
	real_t get_genetic_mutate_link_weight_uniform_prob() const { return settings.genetic_mutate_link_weight_uniform_prob; }

	void set_genetic_mutate_toggle_link_enable_prob(real_t p_genetic_mutate_toggle_link_enable_prob) { settings.genetic_mutate_toggle_link_enable_prob = p_genetic_mutate_toggle_link_enable_prob; }
	real_t get_genetic_mutate_toggle_link_enable_prob() const { return settings.genetic_mutate_toggle_link_enable_prob; }

	void set_genetic_mutate_add_link_recurrent_prob(real_t p_genetic_mutate_add_link_recurrent_prob) { settings.genetic_mutate_add_link_recurrent_prob = p_genetic_mutate_add_link_recurrent_prob; }
	real_t get_genetic_mutate_add_link_recurrent_prob() const { return settings.genetic_mutate_add_link_recurrent_prob; }

	void set_species_youngness_age_threshold(real_t p_species_youngness_age_threshold) { settings.species_youngness_age_threshold = p_species_youngness_age_threshold; }
	int get_species_youngness_age_threshold() const { return settings.species_youngness_age_threshold; }

	void set_species_youngness_multiplier(real_t p_species_youngness_multiplier) { settings.species_youngness_multiplier = p_species_youngness_multiplier; }
	real_t get_species_youngness_multiplier() const { return settings.species_youngness_multiplier; }

	void set_species_stagnant_age_threshold(real_t p_species_stagnant_age_threshold) { settings.species_stagnant_age_threshold = p_species_stagnant_age_threshold; }
	int get_species_stagnant_age_threshold() const { return settings.species_stagnant_age_threshold; }

	void set_species_stagnant_multiplier(real_t p_species_stagnant_multiplier) { settings.species_stagnant_multiplier = p_species_stagnant_multiplier; }
	real_t get_species_stagnant_multiplier() const { return settings.species_stagnant_multiplier; }

	void set_species_survival_ratio(real_t p_species_survival_ratio) { settings.species_survival_ratio = p_species_survival_ratio; }
	real_t get_species_survival_ratio() const { return settings.species_survival_ratio; }

	void set_cribs_stealing(real_t p_cribs_stealing) { settings.cribs_stealing = p_cribs_stealing; }
	int get_cribs_stealing() const { return settings.cribs_stealing; }

	void set_cribs_stealing_limit(real_t p_cribs_stealing_limit) { settings.cribs_stealing_limit = p_cribs_stealing_limit; }
	int get_cribs_stealing_limit() const { return settings.cribs_stealing_limit; }

	void set_cribs_stealing_protection_age_threshold(real_t p_cribs_stealing_protection_age_threshold) { settings.cribs_stealing_protection_age_threshold = p_cribs_stealing_protection_age_threshold; }
	int get_cribs_stealing_protection_age_threshold() const { return settings.cribs_stealing_protection_age_threshold; }

	void set_population_stagnant_age_thresold(real_t p_population_stagnant_age_thresold) { settings.population_stagnant_age_thresold = p_population_stagnant_age_thresold; }
	int get_population_stagnant_age_thresold() const { return settings.population_stagnant_age_thresold; }
};

#endif // NEAT_POPULATION_H
