#include "neat_population.h"

void NeatPopulation::_bind_methods() {

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

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "learning_deviation"), "set_learning_deviation", "get_learning_deviation");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_compatibility_threshold"), "set_genetic_compatibility_threshold", "get_genetic_compatibility_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_disjoints_significance"), "set_genetic_disjoints_significance", "get_genetic_disjoints_significance");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_excesses_significance"), "set_genetic_excesses_significance", "get_genetic_excesses_significance");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_weights_significance"), "set_genetic_weights_significance", "get_genetic_weights_significance");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mate_prob"), "set_genetic_mate_prob", "get_genetic_mate_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mate_inside_species_threshold"), "set_genetic_mate_inside_species_threshold", "get_genetic_mate_inside_species_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mate_multipoint_threshold"), "set_genetic_mate_multipoint_threshold", "get_genetic_mate_multipoint_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mate_multipoint_avg_threshold"), "set_genetic_mate_multipoint_avg_threshold", "get_genetic_mate_multipoint_avg_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mate_singlepoint_threshold"), "set_genetic_mate_singlepoint_threshold", "get_genetic_mate_singlepoint_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mutate_add_link_porb"), "set_genetic_mutate_add_link_porb", "get_genetic_mutate_add_link_porb");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mutate_add_node_prob"), "set_genetic_mutate_add_node_prob", "get_genetic_mutate_add_node_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mutate_link_weight_prob"), "set_genetic_mutate_link_weight_prob", "get_genetic_mutate_link_weight_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mutate_link_weight_uniform_prob"), "set_genetic_mutate_link_weight_uniform_prob", "get_genetic_mutate_link_weight_uniform_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mutate_toggle_link_enable_prob"), "set_genetic_mutate_toggle_link_enable_prob", "get_genetic_mutate_toggle_link_enable_prob");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "genetic_mutate_add_link_recurrent_prob"), "set_genetic_mutate_add_link_recurrent_prob", "get_genetic_mutate_add_link_recurrent_prob");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "species_youngness_age_threshold"), "set_species_youngness_age_threshold", "get_species_youngness_age_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "species_youngness_multiplier"), "set_species_youngness_multiplier", "get_species_youngness_multiplier");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "species_stagnant_age_threshold"), "set_species_stagnant_age_threshold", "get_species_stagnant_age_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "species_stagnant_multiplier"), "set_species_stagnant_multiplier", "get_species_stagnant_multiplier");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "species_survival_ratio"), "set_species_survival_ratio", "get_species_survival_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cribs_stealing"), "set_cribs_stealing", "get_cribs_stealing");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cribs_stealing_limit"), "set_cribs_stealing_limit", "get_cribs_stealing_limit");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cribs_stealing_protection_age_threshold"), "set_cribs_stealing_protection_age_threshold", "get_cribs_stealing_protection_age_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "population_stagnant_age_thresold"), "set_population_stagnant_age_thresold", "get_population_stagnant_age_thresold");
}

NeatPopulation::NeatPopulation() :
		Neat() {
}
