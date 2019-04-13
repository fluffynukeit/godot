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
	void resize(int p_size);
	int size() const;

	void set_all(real_t p_value);
	void set_values(const Vector<real_t> &p_values);
	void set_value__grid(
			int p_width,
			int p_height,
			int p_x,
			int p_y,
			real_t p_value,
			int p_propagation_radius);

	void set_value(int p_index, real_t p_value);
	real_t get_value(int p_index) const;

	void paint_on_texture(Ref<Texture> r_texture);
	void paint_on_image(Ref<Image> r_image);
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

	static void test() {}
};

VARIANT_ENUM_CAST(BrainArea::Activation);

#endif // BRAINAREA_H
