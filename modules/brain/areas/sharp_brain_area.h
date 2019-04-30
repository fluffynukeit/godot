#ifndef SHARP_BRAIN_AREA_H
#define SHARP_BRAIN_AREA_H

#include "brain_area.h"

#include "thirdparty/brain/brain/NEAT/neat_genome.h"
#include "thirdparty/brain/brain/brain_areas/sharp_brain_area.h"

class SharpBrainAreaStructure : public Resource {
	GDCLASS(SharpBrainAreaStructure, Resource);

public:
	virtual void make_brain_area(brain::SharpBrainArea &r_area) = 0;
	virtual void make_brain_area(brain::NtGenome &r_genome) = 0;
};

class SharpBrainAreaStructureRuntime : public Resource {
	GDCLASS(SharpBrainAreaStructureRuntime, Resource);

public:
	brain::SharpBrainArea area;

	virtual void make_brain_area(brain::SharpBrainArea &r_area);
	virtual void make_brain_area(brain::NtGenome &r_genome);
};

class SharpBrainAreaStructureEditable : public SharpBrainAreaStructure {
	GDCLASS(SharpBrainAreaStructureEditable, SharpBrainAreaStructure);

	enum NeuronType {
		NEURON_TYPE_INPUT,
		NEURON_TYPE_HIDDEN,
		NEURON_TYPE_OUTPUT
	};

	struct Neuron {
		NeuronType type = NEURON_TYPE_INPUT;
		BrainArea::Activation activation = BrainArea::ACTIVATION_SIGMOID;
	};

	struct Link {
		int neuron_parent_id = 0;
		int neuron_child_id = 0;
		real_t weight = 0.5;
		bool recurrent = false;
	};

	int neuron_count;
	int link_count;

	Vector<Neuron> neurons;
	Vector<Link> links;

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	static void _bind_methods();

public:
	SharpBrainAreaStructureEditable();

	void set_neuron_count(int p_neuron_count);
	int get_neuron_count() const;

	void set_link_count(int p_link_count);
	int get_link_count() const;

	virtual void make_brain_area(brain::SharpBrainArea &r_area);
	virtual void make_brain_area(brain::NtGenome &r_genome);
};

class SharpBrainAreaStructureAncestor : public SharpBrainAreaStructure {
	GDCLASS(SharpBrainAreaStructureAncestor, SharpBrainAreaStructure);

	int input_count;
	int output_count;
	bool randomize_weights;
	BrainArea::Activation input_activation_func;
	BrainArea::Activation output_activation_func;

	static void _bind_methods();

public:
	SharpBrainAreaStructureAncestor();

	void set_input_count(int p_input_count) {
		input_count = p_input_count;
		emit_changed();
	}
	int get_input_count() const {
		return input_count;
	}
	void set_output_count(int p_output_count) {
		output_count = p_output_count;
		emit_changed();
	}
	int get_output_count() const {
		return output_count;
	}
	void set_randomize_weights(bool p_randomize_weights) {
		randomize_weights = p_randomize_weights;
		emit_changed();
	}
	bool get_randomize_weights() const {
		return randomize_weights;
	}
	void set_input_activation_func(BrainArea::Activation p_input_activation_func) {
		input_activation_func = p_input_activation_func;
		emit_changed();
	}
	BrainArea::Activation get_input_activation_func() const {
		return input_activation_func;
	}
	void set_output_activation_func(BrainArea::Activation p_output_activation_func) {
		output_activation_func = p_output_activation_func;
		emit_changed();
	}
	BrainArea::Activation get_output_activation_func() const {
		return output_activation_func;
	}

	virtual void make_brain_area(brain::SharpBrainArea &r_area);
	virtual void make_brain_area(brain::NtGenome &r_genome);
};

class SharpBrainArea : public BrainArea {
	GDCLASS(SharpBrainArea, BrainArea);

	friend class NeatPopulation;

private:
	Ref<SharpBrainAreaStructure> structure;
	brain::SharpBrainArea brain_area;

	static void _bind_methods();

public:
	SharpBrainArea();

	virtual brain::BrainArea &get_internal_brain() {
		return brain_area;
	}

	void set_structure(Ref<SharpBrainAreaStructure> p_struct);
	Ref<SharpBrainAreaStructure> get_structure() const;

	String description() const;

	virtual bool guess(
			Ref<SynapticTerminals> p_input,
			Ref<SynapticTerminals> r_result);

private:
	void update_shape_area();
};

#endif // SHARP_BRAIN_AREA_H
