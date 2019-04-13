#ifndef BRAINAREA_H
#define BRAINAREA_H

#include "scene/main/node.h"

#include "thirdparty/brain/brain/brain_areas/uniform_brain_area.h"

class SynapticTerminals : public Reference {
	GDCLASS(SynapticTerminals, Reference);

	friend class UniformBrainArea;

	brain::Matrix matrix;

	static void _bind_methods();

public:
	int terminal_count() const;
	real_t get(int p_row) const;
};

class BrainArea : public Node {
	GDCLASS(BrainArea, Node);

public:
	enum Activation {
		ACTIVATION_SIGMOID,
		ACTIVATION_RELU,
		ACTIVATION_LEAKY_RELU,
		ACTIVATION_TANH,
		ACTIVATION_LINEAR,
		ACTIVATION_BINARY,
		ACTIVATION_SOFTMAX,
		ACTIVATION_MAX
	};

private:
	static void _bind_methods();

public:
	BrainArea();
};

VARIANT_ENUM_CAST(BrainArea::Activation);

#endif // BRAINAREA_H
