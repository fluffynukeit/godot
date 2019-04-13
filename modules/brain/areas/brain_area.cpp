#include "brain_area.h"

void SynapticTerminals::_bind_methods() {

	ClassDB::bind_method(D_METHOD("terminal_count"), &SynapticTerminals::terminal_count);
	ClassDB::bind_method(D_METHOD("get_value", "index"), &SynapticTerminals::get);
}

int SynapticTerminals::terminal_count() const {
	return matrix.get_row_count();
}

real_t SynapticTerminals::get(int p_index) const {
	return matrix.get(p_index, 0);
}

void BrainArea::_bind_methods() {

	BIND_ENUM_CONSTANT(ACTIVATION_SIGMOID);
}

BrainArea::BrainArea() {
}
