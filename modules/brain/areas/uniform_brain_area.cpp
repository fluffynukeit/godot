#include "uniform_brain_area.h"

#include "core/os/os.h"
#include "thirdparty/brain/brain/math/math_funcs.h"

bool UniformBrainArea::_set(const StringName &p_name, const Variant &p_value) {
	String name(p_name);

	if (!name.begins_with("hidden_layer_"))
		return false;

	const int hidden_layer = name
									 .get_slicec('/', 0)
									 .get_slicec('_', 2)
									 .to_int();

	String what = name.get_slicec('/', 1);

	if (what == "size") {

		set_hidden_layer_size(hidden_layer, p_value);
	} else if (what == "activation") {

		set_hidden_layer_activation(
				hidden_layer,
				static_cast<Activation>(static_cast<int>(p_value)));
	} else {
		return false;
	}

	return true;
}

bool UniformBrainArea::_get(const StringName &p_name, Variant &r_ret) const {

	String name(p_name);

	if (!name.begins_with("hidden_layer_"))
		return false;

	const int hidden_layer = name
									 .get_slicec('/', 0)
									 .get_slicec('_', 2)
									 .to_int();

	String what = name.get_slicec('/', 1);

	if (what == "size") {

		r_ret = get_hidden_layer_size(hidden_layer);
	} else if (what == "activation") {

		r_ret = get_hidden_layer_activation(hidden_layer);
	} else {
		return false;
	}

	return true;
}

void UniformBrainArea::_get_property_list(List<PropertyInfo> *p_list) const {

	for (int i(0); i < get_hidden_layers_count(); ++i) {
		p_list->push_back(PropertyInfo(
				Variant::INT,
				"hidden_layer_" + itos(i) + "/size"));

		p_list->push_back(PropertyInfo(
				Variant::INT,
				"hidden_layer_" + itos(i) + "/activation",
				PROPERTY_HINT_ENUM,
				"Sigmoid,Relu,Leaky Relu,Tanh,Linear,Binary,Soft Max"));
	}
}

void UniformBrainArea::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_input_layer_size", "size"), &UniformBrainArea::set_input_layer_size);
	ClassDB::bind_method(D_METHOD("get_input_layer_size"), &UniformBrainArea::get_input_layer_size);

	ClassDB::bind_method(D_METHOD("set_hidden_layers_count", "count"), &UniformBrainArea::set_hidden_layers_count);
	ClassDB::bind_method(D_METHOD("get_hidden_layers_count"), &UniformBrainArea::get_hidden_layers_count);

	ClassDB::bind_method(D_METHOD("set_output_layer_size", "size"), &UniformBrainArea::set_output_layer_size);
	ClassDB::bind_method(D_METHOD("get_output_layer_size"), &UniformBrainArea::get_output_layer_size);

	ClassDB::bind_method(D_METHOD("set_output_layer_activation", "activation"), &UniformBrainArea::set_output_layer_activation);
	ClassDB::bind_method(D_METHOD("get_output_layer_activation"), &UniformBrainArea::get_output_layer_activation);

	ClassDB::bind_method(D_METHOD("prepare_to_learn"), &UniformBrainArea::prepare_to_learn);
	ClassDB::bind_method(D_METHOD("learn", "input", "expected", "learning_rate"), &UniformBrainArea::learn);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "input_layer_size"), "set_input_layer_size", "get_input_layer_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hidden_layers_count"), "set_hidden_layers_count", "get_hidden_layers_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output_layer_size"), "set_output_layer_size", "get_output_layer_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output_layer_activation", PROPERTY_HINT_ENUM, "Sigmoid,Relu,Leaky Relu,Tanh,Linear,Binary,Soft Max"), "set_output_layer_activation", "get_output_layer_activation");
}

UniformBrainArea::UniformBrainArea() {}

void UniformBrainArea::set_input_layer_size(int p_size) {
	brain_area.set_input_layer_size(p_size);
}

int UniformBrainArea::get_input_layer_size() {
	return brain_area.get_input_layer_size();
}

void UniformBrainArea::set_hidden_layers_count(int p_count) {

	p_count = MAX(0, p_count);

	const int prev = brain_area.get_hidden_layers_count();
	brain_area.set_hidden_layers_count(p_count);

	// Auto initialization
	if (prev < p_count) {
		const int size(brain_area.get_input_layer_size());

		for (int i(prev); i < p_count; ++i) {
			brain_area.set_hidden_layer(
					i,
					size,
					brain::BrainArea::ACTIVATION_SIGMOID);
		}
	}

	_change_notify();
}

int UniformBrainArea::get_hidden_layers_count() const {
	return brain_area.get_hidden_layers_count();
}

void UniformBrainArea::set_hidden_layer_size(int p_hidden_layer, int p_size) {
	ERR_FAIL_INDEX(p_hidden_layer, get_hidden_layers_count());
	brain_area.set_hidden_layer_size(p_hidden_layer, p_size);
}

int UniformBrainArea::get_hidden_layer_size(int p_hidden_layer) const {
	ERR_FAIL_INDEX_V(p_hidden_layer, get_hidden_layers_count(), 0);
	return brain_area.get_hidden_layer_size(p_hidden_layer);
}

void UniformBrainArea::set_hidden_layer_activation(int p_hidden_layer, Activation p_activation) {
	ERR_FAIL_INDEX(p_hidden_layer, get_hidden_layers_count());
	ERR_FAIL_INDEX(p_activation, ACTIVATION_MAX);
	brain_area.set_hidden_layer_activation(
			p_hidden_layer,
			static_cast<brain::BrainArea::Activation>(p_activation));
}

BrainArea::Activation UniformBrainArea::get_hidden_layer_activation(int p_hidden_layer) const {
	ERR_FAIL_INDEX_V(p_hidden_layer, get_hidden_layers_count(), ACTIVATION_MAX);
	return static_cast<Activation>(brain_area.get_hidden_layer_activation(p_hidden_layer));
}

void UniformBrainArea::set_output_layer_size(int p_size) {
	brain_area.set_output_layer_size(p_size);
}

int UniformBrainArea::get_output_layer_size() const {
	return brain_area.get_output_layer_size();
}

void UniformBrainArea::set_output_layer_activation(Activation p_activation) {
	brain_area.set_output_layer_activation(
			static_cast<brain::BrainArea::Activation>(p_activation));
}

BrainArea::Activation UniformBrainArea::get_output_layer_activation() const {
	return static_cast<Activation>(brain_area.get_output_layer_activation());
}

void UniformBrainArea::prepare_to_learn() {
	brain::Math::seed(OS::get_singleton()->get_unix_time());
	brain_area.randomize_weights(1);
	brain_area.randomize_biases(1);
}

real_t UniformBrainArea::learn(
		const Vector<real_t> &p_input,
		const Vector<real_t> &p_expected,
		real_t learning_rate) {

	brain::Matrix input(p_input.size(), 1, p_input.ptr());
	brain::Matrix expected(p_expected.size(), 1, p_expected.ptr());

	return brain_area.learn(input, expected, learning_rate, &learning_cache);
}

bool UniformBrainArea::guess(
		Ref<SynapticTerminals> p_input,
		Ref<SynapticTerminals> r_result) {

	return brain_area.guess(p_input->matrix, r_result->matrix);
}
