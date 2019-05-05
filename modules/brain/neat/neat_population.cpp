#include "neat_population.h"

#include "core/engine.h"
#include "core/os/os.h"

void NeatPopulation::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_population_size", "population_size"), &NeatPopulation::set_population_size);
	ClassDB::bind_method(D_METHOD("get_population_size"), &NeatPopulation::get_population_size);

	ClassDB::bind_method(D_METHOD("set_random_seed", "actice"), &NeatPopulation::set_random_seed);
	ClassDB::bind_method(D_METHOD("get_random_seed"), &NeatPopulation::get_random_seed);

	ClassDB::bind_method(D_METHOD("set_ancestor", "ancestor"), &NeatPopulation::set_ancestor);
	ClassDB::bind_method(D_METHOD("get_ancestor"), &NeatPopulation::get_ancestor);

	ClassDB::bind_method(D_METHOD("set_seed", "seed"), &NeatPopulation::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &NeatPopulation::get_seed);
	ClassDB::bind_method(D_METHOD("set_learning_deviation", "learning_deviation"), &NeatPopulation::set_learning_deviation);
	ClassDB::bind_method(D_METHOD("get_learning_deviation"), &NeatPopulation::get_learning_deviation);
	ClassDB::bind_method(D_METHOD("set_genetic_compatibility_threshold", "genetic_compatibility_threshold"), &NeatPopulation::set_genetic_compatibility_threshold);
	ClassDB::bind_method(D_METHOD("get_genetic_compatibility_threshold"), &NeatPopulation::get_genetic_compatibility_threshold);
	ClassDB::bind_method(D_METHOD("set_genetic_disjoints_significance", "genetic_disjoints_significance"), &NeatPopulation::set_genetic_disjoints_significance);
	ClassDB::bind_method(D_METHOD("get_genetic_disjoints_significance"), &NeatPopulation::get_genetic_disjoints_significance);
	ClassDB::bind_method(D_METHOD("set_genetic_excesses_significance", "genetic_excesses_significance"), &NeatPopulation::set_genetic_excesses_significance);
	ClassDB::bind_method(D_METHOD("get_genetic_excesses_significance"), &NeatPopulation::get_genetic_excesses_significance);
	ClassDB::bind_method(D_METHOD("set_genetic_weights_significance", "genetic_weights_significance"), &NeatPopulation::set_genetic_weights_significance);
	ClassDB::bind_method(D_METHOD("get_genetic_weights_significance"), &NeatPopulation::get_genetic_weights_significance);
	ClassDB::bind_method(D_METHOD("set_genetic_mate_prob", "genetic_mate_prob"), &NeatPopulation::set_genetic_mate_prob);
	ClassDB::bind_method(D_METHOD("get_genetic_mate_prob"), &NeatPopulation::get_genetic_mate_prob);
	ClassDB::bind_method(D_METHOD("set_genetic_mate_inside_species_threshold", "genetic_mate_inside_species_threshold"), &NeatPopulation::set_genetic_mate_inside_species_threshold);
	ClassDB::bind_method(D_METHOD("get_genetic_mate_inside_species_threshold"), &NeatPopulation::get_genetic_mate_inside_species_threshold);
	ClassDB::bind_method(D_METHOD("set_genetic_mate_multipoint_threshold", "genetic_mate_multipoint_threshold"), &NeatPopulation::set_genetic_mate_multipoint_threshold);
	ClassDB::bind_method(D_METHOD("get_genetic_mate_multipoint_threshold"), &NeatPopulation::get_genetic_mate_multipoint_threshold);
	ClassDB::bind_method(D_METHOD("set_genetic_mate_multipoint_avg_threshold", "genetic_mate_multipoint_avg_threshold"), &NeatPopulation::set_genetic_mate_multipoint_avg_threshold);
	ClassDB::bind_method(D_METHOD("get_genetic_mate_multipoint_avg_threshold"), &NeatPopulation::get_genetic_mate_multipoint_avg_threshold);
	ClassDB::bind_method(D_METHOD("set_genetic_mate_singlepoint_threshold", "genetic_mate_singlepoint_threshold"), &NeatPopulation::set_genetic_mate_singlepoint_threshold);
	ClassDB::bind_method(D_METHOD("get_genetic_mate_singlepoint_threshold"), &NeatPopulation::get_genetic_mate_singlepoint_threshold);
	ClassDB::bind_method(D_METHOD("set_genetic_mutate_add_link_porb", "genetic_mutate_add_link_porb"), &NeatPopulation::set_genetic_mutate_add_link_porb);
	ClassDB::bind_method(D_METHOD("get_genetic_mutate_add_link_porb"), &NeatPopulation::get_genetic_mutate_add_link_porb);
	ClassDB::bind_method(D_METHOD("set_genetic_mutate_add_node_prob", "genetic_mutate_add_node_prob"), &NeatPopulation::set_genetic_mutate_add_node_prob);
	ClassDB::bind_method(D_METHOD("get_genetic_mutate_add_node_prob"), &NeatPopulation::get_genetic_mutate_add_node_prob);
	ClassDB::bind_method(D_METHOD("set_genetic_mutate_link_weight_prob", "genetic_mutate_link_weight_prob"), &NeatPopulation::set_genetic_mutate_link_weight_prob);
	ClassDB::bind_method(D_METHOD("get_genetic_mutate_link_weight_prob"), &NeatPopulation::get_genetic_mutate_link_weight_prob);
	ClassDB::bind_method(D_METHOD("set_genetic_mutate_link_weight_uniform_prob", "genetic_mutate_link_weight_uniform_prob"), &NeatPopulation::set_genetic_mutate_link_weight_uniform_prob);
	ClassDB::bind_method(D_METHOD("get_genetic_mutate_link_weight_uniform_prob"), &NeatPopulation::get_genetic_mutate_link_weight_uniform_prob);
	ClassDB::bind_method(D_METHOD("set_genetic_mutate_toggle_link_enable_prob", "genetic_mutate_toggle_link_enable_prob"), &NeatPopulation::set_genetic_mutate_toggle_link_enable_prob);
	ClassDB::bind_method(D_METHOD("get_genetic_mutate_toggle_link_enable_prob"), &NeatPopulation::get_genetic_mutate_toggle_link_enable_prob);
	ClassDB::bind_method(D_METHOD("set_genetic_mutate_add_link_recurrent_prob", "genetic_mutate_add_link_recurrent_prob"), &NeatPopulation::set_genetic_mutate_add_link_recurrent_prob);
	ClassDB::bind_method(D_METHOD("get_genetic_mutate_add_link_recurrent_prob"), &NeatPopulation::get_genetic_mutate_add_link_recurrent_prob);
	ClassDB::bind_method(D_METHOD("set_species_youngness_age_threshold", "species_youngness_age_threshold"), &NeatPopulation::set_species_youngness_age_threshold);
	ClassDB::bind_method(D_METHOD("get_species_youngness_age_threshold"), &NeatPopulation::get_species_youngness_age_threshold);
	ClassDB::bind_method(D_METHOD("set_species_youngness_multiplier", "species_youngness_multiplier"), &NeatPopulation::set_species_youngness_multiplier);
	ClassDB::bind_method(D_METHOD("get_species_youngness_multiplier"), &NeatPopulation::get_species_youngness_multiplier);
	ClassDB::bind_method(D_METHOD("set_species_stagnant_age_threshold", "species_stagnant_age_threshold"), &NeatPopulation::set_species_stagnant_age_threshold);
	ClassDB::bind_method(D_METHOD("get_species_stagnant_age_threshold"), &NeatPopulation::get_species_stagnant_age_threshold);
	ClassDB::bind_method(D_METHOD("set_species_stagnant_multiplier", "species_stagnant_multiplier"), &NeatPopulation::set_species_stagnant_multiplier);
	ClassDB::bind_method(D_METHOD("get_species_stagnant_multiplier"), &NeatPopulation::get_species_stagnant_multiplier);
	ClassDB::bind_method(D_METHOD("set_species_survival_ratio", "species_survival_ratio"), &NeatPopulation::set_species_survival_ratio);
	ClassDB::bind_method(D_METHOD("get_species_survival_ratio"), &NeatPopulation::get_species_survival_ratio);
	ClassDB::bind_method(D_METHOD("set_cribs_stealing", "cribs_stealing"), &NeatPopulation::set_cribs_stealing);
	ClassDB::bind_method(D_METHOD("get_cribs_stealing"), &NeatPopulation::get_cribs_stealing);
	ClassDB::bind_method(D_METHOD("set_cribs_stealing_limit", "cribs_stealing_limit"), &NeatPopulation::set_cribs_stealing_limit);
	ClassDB::bind_method(D_METHOD("get_cribs_stealing_limit"), &NeatPopulation::get_cribs_stealing_limit);
	ClassDB::bind_method(D_METHOD("set_cribs_stealing_protection_age_threshold", "cribs_stealing_protection_age_threshold"), &NeatPopulation::set_cribs_stealing_protection_age_threshold);
	ClassDB::bind_method(D_METHOD("get_cribs_stealing_protection_age_threshold"), &NeatPopulation::get_cribs_stealing_protection_age_threshold);
	ClassDB::bind_method(D_METHOD("set_population_stagnant_age_thresold", "population_stagnant_age_thresold"), &NeatPopulation::set_population_stagnant_age_thresold);
	ClassDB::bind_method(D_METHOD("get_population_stagnant_age_thresold"), &NeatPopulation::get_population_stagnant_age_thresold);

	ClassDB::bind_method(D_METHOD("organism_get_brain_area", "organism_id", "sharp_brain_area"), &NeatPopulation::organism_get_brain_area);
	ClassDB::bind_method(D_METHOD("organism_get_brain_area_structure", "organism_id"), &NeatPopulation::organism_get_brain_area_structure);

	ClassDB::bind_method(D_METHOD("organism_set_fitness", "organism_id", "fitness"), &NeatPopulation::organism_set_fitness);
	ClassDB::bind_method(D_METHOD("organism_get_fitness", "organism_id"), &NeatPopulation::organism_get_fitness);

	ClassDB::bind_method(D_METHOD("epoch_advance"), &NeatPopulation::epoch_advance);

	ClassDB::bind_method(D_METHOD("get_epoch"), &NeatPopulation::get_epoch);
	ClassDB::bind_method(D_METHOD("get_best_fitness_ever"), &NeatPopulation::get_best_fitness_ever);
	ClassDB::bind_method(D_METHOD("get_champion_brain_area"), &NeatPopulation::get_champion_brain_area);

	ClassDB::bind_method(D_METHOD("get_statistic", "key"), &NeatPopulation::get_statistic);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "population_size"), "set_population_size", "get_population_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "random_seed"), "set_random_seed", "get_random_seed");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "ancestor", PROPERTY_HINT_RESOURCE_TYPE, "SharpBrainAreaStructure"), "set_ancestor", "get_ancestor");

	ADD_GROUP("Settings", "settings_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "settings_seed"), "set_seed", "get_seed");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_learning_deviation"), "set_learning_deviation", "get_learning_deviation");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_compatibility_threshold"), "set_genetic_compatibility_threshold", "get_genetic_compatibility_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_disjoints_significance"), "set_genetic_disjoints_significance", "get_genetic_disjoints_significance");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_excesses_significance"), "set_genetic_excesses_significance", "get_genetic_excesses_significance");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_weights_significance"), "set_genetic_weights_significance", "get_genetic_weights_significance");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mate_prob"), "set_genetic_mate_prob", "get_genetic_mate_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mate_inside_species_threshold"), "set_genetic_mate_inside_species_threshold", "get_genetic_mate_inside_species_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mate_multipoint_threshold"), "set_genetic_mate_multipoint_threshold", "get_genetic_mate_multipoint_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mate_multipoint_avg_threshold"), "set_genetic_mate_multipoint_avg_threshold", "get_genetic_mate_multipoint_avg_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mate_singlepoint_threshold"), "set_genetic_mate_singlepoint_threshold", "get_genetic_mate_singlepoint_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mutate_add_link_porb"), "set_genetic_mutate_add_link_porb", "get_genetic_mutate_add_link_porb");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mutate_add_node_prob"), "set_genetic_mutate_add_node_prob", "get_genetic_mutate_add_node_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mutate_link_weight_prob"), "set_genetic_mutate_link_weight_prob", "get_genetic_mutate_link_weight_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mutate_link_weight_uniform_prob"), "set_genetic_mutate_link_weight_uniform_prob", "get_genetic_mutate_link_weight_uniform_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mutate_toggle_link_enable_prob"), "set_genetic_mutate_toggle_link_enable_prob", "get_genetic_mutate_toggle_link_enable_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_genetic_mutate_add_link_recurrent_prob"), "set_genetic_mutate_add_link_recurrent_prob", "get_genetic_mutate_add_link_recurrent_prob");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "settings_species_youngness_age_threshold"), "set_species_youngness_age_threshold", "get_species_youngness_age_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_species_youngness_multiplier"), "set_species_youngness_multiplier", "get_species_youngness_multiplier");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "settings_species_stagnant_age_threshold"), "set_species_stagnant_age_threshold", "get_species_stagnant_age_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_species_stagnant_multiplier"), "set_species_stagnant_multiplier", "get_species_stagnant_multiplier");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "settings_species_survival_ratio"), "set_species_survival_ratio", "get_species_survival_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "settings_cribs_stealing"), "set_cribs_stealing", "get_cribs_stealing");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "settings_cribs_stealing_limit"), "set_cribs_stealing_limit", "get_cribs_stealing_limit");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "settings_cribs_stealing_protection_age_threshold"), "set_cribs_stealing_protection_age_threshold", "get_cribs_stealing_protection_age_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "settings_population_stagnant_age_thresold"), "set_population_stagnant_age_thresold", "get_population_stagnant_age_thresold");

	BIND_ENUM_CONSTANT(STATISTIC_EPOCH);
	BIND_ENUM_CONSTANT(STATISTIC_IS_EPOCH_ADVANCED);
	BIND_ENUM_CONSTANT(STATISTIC_POP_CHAMPION_FITNESS);
	BIND_ENUM_CONSTANT(STATISTIC_POP_CHAMPION_SPECIES_ID);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_COUNT);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_YOUNG_COUNT);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_STAGNANT_COUNT);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_AVG_AGES);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_BEST_ID);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_BEST_AGE);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_BEST_OFFSPRING_PRE_STEAL);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_BEST_OFFSPRING);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_BEST_CHAMPION_OFFSPRING);
	BIND_ENUM_CONSTANT(STATISTIC_SPECIES_BEST_IS_DIED);
	BIND_ENUM_CONSTANT(STATISTIC_POP_AVG_FITNESS);
	BIND_ENUM_CONSTANT(STATISTIC_POP_IS_STAGNANT);
	BIND_ENUM_CONSTANT(STATISTIC_POP_EPOCH_LAST_IMPROVEMENT);
	BIND_ENUM_CONSTANT(STATISTIC_POP_STOLEN_CRIBS);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_CHAMPION_MUTATE_WEIGHTS);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_CHAMPION_ADD_RANDOM_LINK);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_MATE_MULTIPOINT);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_MATE_MULTIPOINT_AVG);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_MATE_SINGLEPOINT);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_MUTATE_ADD_RANDOM_LINK);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_MUTATE_ADD_RANDOM_NEURON);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_MUTATE_WEIGHTS);
	BIND_ENUM_CONSTANT(STATISTIC_REPRODUCTION_MUTATE_TOGGLE_LINK_ACTIVATION);
}

void NeatPopulation::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {

			if (Engine::get_singleton()->is_editor_hint())
				break;

			init_population();
			break;
		}
	}
}

NeatPopulation::NeatPopulation() :
		Neat(),
		population_size(100),
		random_seed(true),
		population(nullptr) {
}

void NeatPopulation::set_ancestor(Ref<SharpBrainAreaStructure> p_ancestor) {
	ancestor = p_ancestor;
}

void NeatPopulation::organism_get_brain_area(int p_organism_id, Object *r_brain_area) const {

	SharpBrainArea *brain_area = Object::cast_to<SharpBrainArea>(r_brain_area);
	ERR_FAIL_COND(!brain_area);
	brain_area->set_structure(Ref<SharpBrainAreaStructure>());
	brain_area->brain_area = *population->organism_get_network(p_organism_id);
}

Ref<SharpBrainAreaStructureRuntime> NeatPopulation::organism_get_brain_area_structure(int p_organism_id) const {

	Ref<SharpBrainAreaStructureRuntime> r;
	r.instance();
	r->area = *population->organism_get_network(p_organism_id);
	return r;
}

void NeatPopulation::organism_set_fitness(int p_organism_id, real_t p_fitness) {
	population->organism_set_fitness(p_organism_id, p_fitness);
}

real_t NeatPopulation::organism_get_fitness(int p_organism_id) const {
	return population->organism_get_fitness(p_organism_id);
}

bool NeatPopulation::epoch_advance() {
	const bool res = population->epoch_advance();
	statistics = population->get_epoch_statistics();
	return res;
}

int NeatPopulation::get_epoch() const {
	return population->get_epoch();
}

real_t NeatPopulation::get_best_fitness_ever() const {
	return population->get_best_personal_fitness();
}

Ref<SharpBrainAreaStructureRuntime> NeatPopulation::get_champion_brain_area() const {
	Ref<SharpBrainAreaStructureRuntime> r;
	r.instance();
	population->get_champion_network(r->area);
	return r;
}

Variant NeatPopulation::get_statistic(StatisticKeys p_what) const {

	if (p_what == STATISTIC_EPOCH) {
		return statistics.epoch;
	}
	if (p_what == STATISTIC_IS_EPOCH_ADVANCED) {
		return statistics.is_epoch_advanced;
	}
	if (p_what == STATISTIC_POP_CHAMPION_FITNESS) {
		return statistics.pop_champion_fitness;
	}
	if (p_what == STATISTIC_POP_CHAMPION_SPECIES_ID) {
		return statistics.pop_champion_species_id;
	}
	if (p_what == STATISTIC_SPECIES_COUNT) {
		return statistics.species_count;
	}
	if (p_what == STATISTIC_SPECIES_YOUNG_COUNT) {
		return statistics.species_young_count;
	}
	if (p_what == STATISTIC_SPECIES_STAGNANT_COUNT) {
		return statistics.species_stagnant_count;
	}
	if (p_what == STATISTIC_SPECIES_AVG_AGES) {
		return statistics.species_avg_ages;
	}
	if (p_what == STATISTIC_SPECIES_BEST_ID) {
		return statistics.species_best_id;
	}
	if (p_what == STATISTIC_SPECIES_BEST_AGE) {
		return statistics.species_best_age;
	}
	if (p_what == STATISTIC_SPECIES_BEST_OFFSPRING_PRE_STEAL) {
		return statistics.species_best_offspring_pre_steal;
	}
	if (p_what == STATISTIC_SPECIES_BEST_OFFSPRING) {
		return statistics.species_best_offspring;
	}
	if (p_what == STATISTIC_SPECIES_BEST_CHAMPION_OFFSPRING) {
		return statistics.species_best_champion_offspring;
	}
	if (p_what == STATISTIC_SPECIES_BEST_IS_DIED) {
		return statistics.species_best_is_died;
	}
	if (p_what == STATISTIC_POP_AVG_FITNESS) {
		return statistics.pop_avg_fitness;
	}
	if (p_what == STATISTIC_POP_IS_STAGNANT) {
		return statistics.pop_is_stagnant;
	}
	if (p_what == STATISTIC_POP_EPOCH_LAST_IMPROVEMENT) {
		return statistics.pop_epoch_last_improvement;
	}
	if (p_what == STATISTIC_POP_STOLEN_CRIBS) {
		return statistics.pop_stolen_cribs;
	}
	if (p_what == STATISTIC_REPRODUCTION_CHAMPION_MUTATE_WEIGHTS) {
		return statistics.reproduction_champion_mutate_weights;
	}
	if (p_what == STATISTIC_REPRODUCTION_CHAMPION_ADD_RANDOM_LINK) {
		return statistics.reproduction_champion_add_random_link;
	}
	if (p_what == STATISTIC_REPRODUCTION_MATE_MULTIPOINT) {
		return statistics.reproduction_mate_multipoint;
	}
	if (p_what == STATISTIC_REPRODUCTION_MATE_MULTIPOINT_AVG) {
		return statistics.reproduction_mate_multipoint_avg;
	}
	if (p_what == STATISTIC_REPRODUCTION_MATE_SINGLEPOINT) {
		return statistics.reproduction_mate_singlepoint;
	}
	if (p_what == STATISTIC_REPRODUCTION_MUTATE_ADD_RANDOM_LINK) {
		return statistics.reproduction_mutate_add_random_link;
	}
	if (p_what == STATISTIC_REPRODUCTION_MUTATE_ADD_RANDOM_NEURON) {
		return statistics.reproduction_mutate_add_random_neuron;
	}
	if (p_what == STATISTIC_REPRODUCTION_MUTATE_WEIGHTS) {
		return statistics.reproduction_mutate_weights;
	}
	if (p_what == STATISTIC_REPRODUCTION_MUTATE_TOGGLE_LINK_ACTIVATION) {
		return statistics.reproduction_mutate_toggle_link_activation;
	}

	ERR_EXPLAIN("The what value, is not a valid parameter");
	ERR_FAIL_V(Variant());
}

void NeatPopulation::init_population() {

	if (random_seed)
		settings.seed = OS::get_singleton()->get_ticks_usec();

	ERR_FAIL_COND(ancestor.is_null());

	brain::NtGenome ancestor_genome;
	ancestor->make_brain_area(ancestor_genome);

	population = new brain::NtPopulation(
			ancestor_genome,
			population_size,
			settings);
}
