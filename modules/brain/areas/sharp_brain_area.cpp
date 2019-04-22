#include "sharp_brain_area.h"

void SharpBrainAreaStructureRuntime::make_brain_area(brain::SharpBrainArea &r_area) {
	r_area = area;
}

void SharpBrainAreaStructureRuntime::make_brain_area(brain::NtGenome &r_genome) {
	ERR_EXPLAIN("This function is not implemented");
	ERR_FAIL();
}

bool SharpBrainAreaStructureEditable::_set(const StringName &p_name, const Variant &p_value) {

	String name(p_name);

	if (name.begins_with("neurons")) {

		const int neuron_id = name
									  .get_slicec('/', 1)
									  .get_slicec('_', 1)
									  .to_int();

		ERR_FAIL_INDEX_V(neuron_id, neurons.size(), false);

		String what = name.get_slicec('/', 2);

		if (what == "type") {

			neurons.write[neuron_id].type = NeuronType(int(p_value));
		} else if (what == "activation") {

			neurons.write[neuron_id].activation = BrainArea::Activation(int(p_value));
		} else {
			return false;
		}

		emit_changed();
		return true;

	} else if (name.begins_with("links")) {

		const int link_id = name
									.get_slicec('/', 1)
									.get_slicec('_', 1)
									.to_int();

		ERR_FAIL_INDEX_V(link_id, links.size(), false);

		String what = name.get_slicec('/', 2);

		if (what == "parent") {

			links.write[link_id].neuron_parent_id = p_value;

		} else if (what == "child") {

			links.write[link_id].neuron_child_id = p_value;

		} else if (what == "weight") {

			links.write[link_id].weight = p_value;

		} else if (what == "recurrent") {

			links.write[link_id].recurrent = p_value;

		} else {
			return false;
		}

		emit_changed();
		return true;
	}

	return false;
}

bool SharpBrainAreaStructureEditable::_get(const StringName &p_name, Variant &r_ret) const {
	String name(p_name);

	if (name.begins_with("neurons")) {

		const int neuron_id = name
									  .get_slicec('/', 1)
									  .get_slicec('_', 1)
									  .to_int();

		ERR_FAIL_INDEX_V(neuron_id, neurons.size(), false);

		String what = name.get_slicec('/', 2);

		if (what == "type") {

			r_ret = neurons[neuron_id].type;
		} else if (what == "activation") {

			r_ret = neurons[neuron_id].activation;
		} else {
			return false;
		}

		return true;

	} else if (name.begins_with("links")) {

		const int link_id = name
									.get_slicec('/', 1)
									.get_slicec('_', 1)
									.to_int();

		ERR_FAIL_INDEX_V(link_id, links.size(), false);

		String what = name.get_slicec('/', 2);

		if (what == "parent") {

			r_ret = links[link_id].neuron_parent_id;

		} else if (what == "child") {

			r_ret = links[link_id].neuron_child_id;

		} else if (what == "weight") {

			r_ret = links[link_id].weight;

		} else if (what == "recurrent") {

			r_ret = links[link_id].recurrent;

		} else {
			return false;
		}

		return true;
	}

	return false;
}

void SharpBrainAreaStructureEditable::_get_property_list(List<PropertyInfo> *p_list) const {

	String neurons_enum_hint("");
	for (int i(0); i < neuron_count; ++i) {

		p_list->push_back(PropertyInfo(
				Variant::INT,
				"neurons/neuron_" + itos(i) + "/type",
				PROPERTY_HINT_ENUM,
				"Input,Hidden,Output"));

		p_list->push_back(PropertyInfo(
				Variant::INT,
				"neurons/neuron_" + itos(i) + "/activation",
				PROPERTY_HINT_ENUM,
				"Sigmoid,Relu,Leaky relu,Tanh,Linear,Binary,Soft Max"));

		neurons_enum_hint += "Neuron " + itos(i) + ",";
	}

	neurons_enum_hint.resize(neurons_enum_hint.size() - 1);

	for (int i(0); i < link_count; ++i) {

		p_list->push_back(PropertyInfo(
				Variant::INT,
				"links/link_" + itos(i) + "/parent",
				PROPERTY_HINT_ENUM,
				neurons_enum_hint));

		p_list->push_back(PropertyInfo(
				Variant::INT,
				"links/link_" + itos(i) + "/child",
				PROPERTY_HINT_ENUM,
				neurons_enum_hint));

		p_list->push_back(PropertyInfo(
				Variant::REAL,
				"links/link_" + itos(i) + "/weight"));

		p_list->push_back(PropertyInfo(
				Variant::BOOL,
				"links/link_" + itos(i) + "/recurrent"));
	}
}

void SharpBrainAreaStructureEditable::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_neuron_count", "neuron_count"), &SharpBrainAreaStructureEditable::set_neuron_count);
	ClassDB::bind_method(D_METHOD("get_neuron_count"), &SharpBrainAreaStructureEditable::get_neuron_count);

	ClassDB::bind_method(D_METHOD("set_link_count", "link_count"), &SharpBrainAreaStructureEditable::set_link_count);
	ClassDB::bind_method(D_METHOD("get_link_count"), &SharpBrainAreaStructureEditable::get_link_count);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "neuron_count"), "set_neuron_count", "get_neuron_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "link_count"), "set_link_count", "get_link_count");
}

SharpBrainAreaStructureEditable::SharpBrainAreaStructureEditable() :
		neuron_count(0),
		link_count(0) {
}

void SharpBrainAreaStructureEditable::set_neuron_count(int p_neuron_count) {
	neuron_count = p_neuron_count;
	neurons.resize(neuron_count);
	_change_notify();
	emit_changed();
}

int SharpBrainAreaStructureEditable::get_neuron_count() const {
	return neuron_count;
}

void SharpBrainAreaStructureEditable::set_link_count(int p_link_count) {
	link_count = p_link_count;
	links.resize(link_count);
	_change_notify();
	emit_changed();
}

int SharpBrainAreaStructureEditable::get_link_count() const {
	return link_count;
}

void SharpBrainAreaStructureEditable::make_brain_area(brain::SharpBrainArea &r_area) {
	r_area.clear();
	for (int i(0); i < neuron_count; ++i) {

		const brain::NeuronId id = r_area.add_neuron();

		if (neurons[i].type == NEURON_TYPE_INPUT)
			r_area.set_neuron_as_input(id);
		else if (neurons[i].type == NEURON_TYPE_OUTPUT)
			r_area.set_neuron_as_output(id);

		const brain::BrainArea::Activation activation = brain::BrainArea::Activation(int(neurons[i].activation));

		r_area.set_neuron_activation(id, activation);
	}

	for (int i(0); i < link_count; ++i) {
		r_area.add_link(
				links[i].neuron_parent_id,
				links[i].neuron_child_id,
				links[i].weight,
				links[i].recurrent);
	}
}

void SharpBrainAreaStructureEditable::make_brain_area(brain::NtGenome &r_genome) {
	r_genome.clear();
	for (int i(0); i < neuron_count; ++i) {
		const brain::NtNeuronGene::NeuronGeneType type = static_cast<brain::NtNeuronGene::NeuronGeneType>(neurons[i].type);
		const brain::BrainArea::Activation activation = static_cast<brain::BrainArea::Activation>(neurons[i].activation);

		r_genome.add_neuron(type, activation);
	}

	int innovation_number(1);
	for (int i(0); i < link_count; ++i) {
		r_genome.add_link(
				links[i].neuron_parent_id,
				links[i].neuron_child_id,
				links[i].weight,
				links[i].recurrent,
				innovation_number++);
	}
}

SharpBrainAreaStructureAncestor::SharpBrainAreaStructureAncestor() :
		input_count(0),
		output_count(0),
		randomize_weights(0),
		input_activation_func(BrainArea::ACTIVATION_LEAKY_RELU),
		output_activation_func(BrainArea::ACTIVATION_LINEAR) {
}

void SharpBrainAreaStructureAncestor::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_input_count", "input_count"), &SharpBrainAreaStructureAncestor::set_input_count);
	ClassDB::bind_method(D_METHOD("get_input_count"), &SharpBrainAreaStructureAncestor::get_input_count);
	ClassDB::bind_method(D_METHOD("set_output_count", "output_count"), &SharpBrainAreaStructureAncestor::set_output_count);
	ClassDB::bind_method(D_METHOD("get_output_count"), &SharpBrainAreaStructureAncestor::get_output_count);
	ClassDB::bind_method(D_METHOD("set_randomize_weights", "randomize_weights"), &SharpBrainAreaStructureAncestor::set_randomize_weights);
	ClassDB::bind_method(D_METHOD("get_randomize_weights"), &SharpBrainAreaStructureAncestor::get_randomize_weights);
	ClassDB::bind_method(D_METHOD("set_input_activation_func", "input_activation_func"), &SharpBrainAreaStructureAncestor::set_input_activation_func);
	ClassDB::bind_method(D_METHOD("get_input_activation_func"), &SharpBrainAreaStructureAncestor::get_input_activation_func);
	ClassDB::bind_method(D_METHOD("set_output_activation_func", "output_activation_func"), &SharpBrainAreaStructureAncestor::set_output_activation_func);
	ClassDB::bind_method(D_METHOD("get_output_activation_func"), &SharpBrainAreaStructureAncestor::get_output_activation_func);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "input_count"), "set_input_count", "get_input_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output_count"), "set_output_count", "get_output_count");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "randomize_weights"), "set_randomize_weights", "get_randomize_weights");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "input_activation_func", PROPERTY_HINT_ENUM, "Sigmoid,Relu,Leaky relu,Tanh,Linear,Binary,Soft Max"), "set_input_activation_func", "get_input_activation_func");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output_activation_func", PROPERTY_HINT_ENUM, "Sigmoid,Relu,Leaky relu,Tanh,Linear,Binary,Soft Max"), "set_output_activation_func", "get_output_activation_func");
}

void SharpBrainAreaStructureAncestor::make_brain_area(brain::SharpBrainArea &r_area) {
}

void SharpBrainAreaStructureAncestor::make_brain_area(brain::NtGenome &r_genome) {
	r_genome.construct(
			input_count,
			output_count,
			randomize_weights,
			static_cast<brain::BrainArea::Activation>(input_activation_func),
			static_cast<brain::BrainArea::Activation>(output_activation_func));
}

void SharpBrainArea::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_structure", "struct"), &SharpBrainArea::set_structure);
	ClassDB::bind_method(D_METHOD("get_structure"), &SharpBrainArea::get_structure);

	ClassDB::bind_method(D_METHOD("update_shape_area"), &SharpBrainArea::update_shape_area);

	ClassDB::bind_method(D_METHOD("description"), &SharpBrainArea::description);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "structure", PROPERTY_HINT_RESOURCE_TYPE, "SharpBrainAreaStructure"), "set_structure", "get_structure");
}

SharpBrainArea::SharpBrainArea() {}

void SharpBrainArea::set_structure(Ref<SharpBrainAreaStructure> p_struct) {

	if (structure.is_valid()) {
		structure->disconnect("changed", this, "update_shape_area");
	}

	structure = p_struct;
	update_shape_area();

	if (structure.is_valid()) {
		structure->connect("changed", this, "update_shape_area");
	}
}

Ref<SharpBrainAreaStructure> SharpBrainArea::get_structure() const {
	return structure;
}

String SharpBrainArea::description() const {
	String s("");
	s += "Neuron count: " + itos(brain_area.get_neuron_count());
	s += " - Hidden neurons count: " + itos(brain_area.get_neuron_count() - (brain_area.get_input_layer_size() + brain_area.get_output_layer_size()));
	return s;
}

bool SharpBrainArea::guess(
		Ref<SynapticTerminals> p_input,
		Ref<SynapticTerminals> r_result) {

	ERR_FAIL_COND_V(p_input.is_null(), false);
	ERR_FAIL_COND_V(r_result.is_null(), false);

	return brain_area.guess(
			p_input->matrix,
			r_result->matrix);
}

void SharpBrainArea::update_shape_area() {
	if (structure.is_null())
		return;
	structure->make_brain_area(brain_area);
}
