#include "brain_area.h"

void SynapticTerminals::_bind_methods() {

	ClassDB::bind_method(D_METHOD("resize", "size"), &SynapticTerminals::resize);
	ClassDB::bind_method(D_METHOD("size"), &SynapticTerminals::size);

	ClassDB::bind_method(D_METHOD("set_values", "values"), &SynapticTerminals::set_values);
	ClassDB::bind_method(D_METHOD("set_all", "value"), &SynapticTerminals::set_all);

	ClassDB::bind_method(D_METHOD("set_value", "index", "value"), &SynapticTerminals::set_value);
	ClassDB::bind_method(D_METHOD("get_value", "index"), &SynapticTerminals::get_value);

	ClassDB::bind_method(D_METHOD("paint_on_texture", "texture"), &SynapticTerminals::paint_on_texture);
	ClassDB::bind_method(D_METHOD("paint_on_image", "image"), &SynapticTerminals::paint_on_image);
}

void SynapticTerminals::resize(int p_size) {
	matrix.resize(p_size, 1);
}

int SynapticTerminals::size() const {
	return matrix.get_row_count();
}

void SynapticTerminals::set_values(const Vector<real_t> &p_input) {
	ERR_FAIL_COND(size() != p_input.size());
	matrix.unsafe_set_row(0, p_input.ptr());
}

void SynapticTerminals::set_all(real_t p_value) {
	matrix.set_all(p_value);
}

void SynapticTerminals::set_value(int p_index, real_t p_value) {
	return matrix.set(p_index, 0, p_value);
}

real_t SynapticTerminals::get_value(int p_index) const {
	return matrix.get(p_index, 0);
}

void SynapticTerminals::paint_on_texture(Ref<Texture> r_texture) {

	ERR_FAIL_COND(r_texture.is_null());

	Ref<Image> image = r_texture->get_data();

	paint_on_image(image);

	const int width(image->get_width());
	const int height(image->get_height());

	VS::get_singleton()->texture_set_data_partial(
			r_texture->get_rid(),
			image,
			0, 0, width, height, 0, 0, 0, 0);
}

void SynapticTerminals::paint_on_image(Ref<Image> r_image) {

	ERR_FAIL_COND(r_image.is_null());

	const int width(r_image->get_width());
	const int height(r_image->get_height());

	ERR_FAIL_COND(width * height != matrix.get_row_count());

	r_image->lock();

	for (int i(0); i < matrix.get_row_count(); ++i) {
		const real_t v = matrix.get(i, 0);
		const int y = i / width;
		const int x = i - y * height;
		r_image->set_pixel(x, y, Color(v, 0., 0., 0.));
	}

	r_image->unlock();
}

void BrainArea::_bind_methods() {

	BIND_ENUM_CONSTANT(ACTIVATION_SIGMOID);
	BIND_ENUM_CONSTANT(ACTIVATION_RELU);
	BIND_ENUM_CONSTANT(ACTIVATION_LEAKY_RELU);
	BIND_ENUM_CONSTANT(ACTIVATION_TANH);
	BIND_ENUM_CONSTANT(ACTIVATION_LINEAR);
	BIND_ENUM_CONSTANT(ACTIVATION_BINARY);
	BIND_ENUM_CONSTANT(ACTIVATION_SOFTMAX);
}

BrainArea::BrainArea() {}
