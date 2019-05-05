#include "brain_area.h"

#include "core/method_bind_ext.gen.inc"
#include "core/os/os.h"

void SynapticTerminals::_bind_methods() {

	ClassDB::bind_method(D_METHOD("resize", "size"), &SynapticTerminals::resize);
	ClassDB::bind_method(D_METHOD("size"), &SynapticTerminals::size);

	ClassDB::bind_method(D_METHOD("set_values", "values"), &SynapticTerminals::set_values);
	ClassDB::bind_method(D_METHOD("set_all", "value"), &SynapticTerminals::set_all);

	ClassDB::bind_method(D_METHOD("set_value__grid", "width", "height", "x", "y", "value", "propagation_radius", "propagation_force"), &SynapticTerminals::set_value__grid, DEFVAL(0), DEFVAL(1.));

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

void SynapticTerminals::set_all(real_t p_value) {
	matrix.set_all(p_value);
}

void SynapticTerminals::set_values(const Vector<real_t> &p_values) {
	ERR_FAIL_COND(size() != p_values.size());
	matrix.unsafe_set_row(0, p_values.ptr());
}

#define GRID_GET_ID(x, y, width) y *width + x

void SynapticTerminals::set_value__grid(
		int p_width,
		int p_height,
		int p_x,
		int p_y,
		real_t p_value,
		int p_propagation_radius,
		real_t p_propagation_force) {

	ERR_FAIL_COND(p_x < 0 || p_x >= p_width);
	ERR_FAIL_COND(p_y < 0 || p_y >= p_height);
	if (p_width * p_height != matrix.get_row_count()) {
		WARN_PRINT_ONCE("The synapsis are differnt quantity of width * height");
	}

	set_value(GRID_GET_ID(p_x, p_y, p_width), p_value);

	if (!p_propagation_radius)
		return;

	/// The below code is responsible for propagating the value to near neighbors
	/// To do it, this algorithm propagates the value using some passes, where
	/// each pass add a value to the grid elements that are around the previous
	/// elements.
	///
	/// The value propagation order is this one:
	///    ^------>
	///    |      |
	///    |      |
	///    <------v
	/// Where the value is propagated from top to right to down to left

	int ring_size = 1;
	for (int prop = 1; prop <= p_propagation_radius; ++prop) {
		ring_size += 2;

		const real_t delta_prop = (real_t(prop) / p_propagation_radius);

		// MIN <- is used to avoid that prop_value is completelly 0
		const real_t prop_value = (1. - pow(MIN(delta_prop, 0.999), p_propagation_force)) * p_value;
		const int x_start_in = p_x - prop;
		const int y_start_in = p_y - prop;

		const int x_end_in = x_start_in + ring_size - 1;
		const int y_end_in = y_start_in + ring_size - 1;

		// Checks if the current writing quad overflows totally the texture size
		if (x_start_in < 0 && y_start_in < 0 && x_end_in >= p_width && y_end_in >= p_height)
			break;

		int x;
		int y;

		// Assert for right side overflow
		if (y_start_in >= 0) {

			x = x_start_in + 1;
			y = y_start_in;

			x = MAX(x, 0);

			// Walk on top side (X)
			while (x <= x_end_in) {

				// Detect overflow
				if (x >= p_width) {
					break;
				}

				const real_t nv = get_value(GRID_GET_ID(x, y, p_width)) + prop_value;
				set_value(GRID_GET_ID(x, y, p_width), MIN(1., nv));
				++x;
			}
		}

		// Assert for right side overflow
		if (x_end_in < p_width) {

			x = x_end_in;
			y = y_start_in + 1;

			y = MAX(y, 0);

			// Walk on right side (Y)
			while (y <= y_end_in) {

				// Detect overflow
				if (y >= p_height) {
					break;
				}

				const real_t nv = get_value(GRID_GET_ID(x, y, p_width)) + prop_value;
				set_value(GRID_GET_ID(x, y, p_width), MIN(1., nv));
				++y;
			}
		}

		// Assert for down side overflow
		if (y_end_in < p_height) {

			x = x_start_in;
			y = y_end_in;

			// Walk on down side (X)
			// Iterate always using + to be more cache friendly
			/// Used < to avoid to write on the end is index,
			/// Since it's already written by the previous step
			while (x < x_end_in) {

				// Detect overflow
				if (x < 0) {
					++x;
					continue;
				}
				// Detect overflow
				if (x >= p_width) {
					break;
				}

				const real_t nv = get_value(GRID_GET_ID(x, y, p_width)) + prop_value;
				set_value(GRID_GET_ID(x, y, p_width), MIN(1., nv));
				++x;
			}
		}

		// Assert for left side overflow
		if (x_start_in >= 0) {

			x = x_start_in;
			y = y_start_in;

			// Walk on left side (Y)
			// Iterate always using + to be more cache friendly
			/// Used < to avoid to write on the end is index,
			/// Since it's already written by the previous step
			while (y < y_end_in) {

				// Detect overflow
				if (y < 0) {
					++y;
					continue;
				}
				// Detect overflow
				if (y >= p_height) {
					break;
				}

				const real_t nv = get_value(GRID_GET_ID(x, y, p_width)) + prop_value;
				set_value(GRID_GET_ID(x, y, p_width), MIN(1., nv));
				++y;
			}
		}
	}
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

	for (int x(0); x < width; ++x) {
		for (int y(0); y < height; ++y) {
			const real_t v = matrix.get(GRID_GET_ID(x, y, width), 0);
			r_image->set_pixel(x, y, Color(v, 0., 0., 0.));
		}
	}

	r_image->unlock();
}

void BrainArea::_bind_methods() {

	ClassDB::bind_method(D_METHOD("guess", "input", "result"), &BrainArea::guess);

	ClassDB::bind_method(D_METHOD("save_knowledge", "path", "overwrite"), &BrainArea::save_knowledge, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("load_knowledge", "path"), &BrainArea::load_knowledge);

	BIND_ENUM_CONSTANT(ACTIVATION_SIGMOID);
	BIND_ENUM_CONSTANT(ACTIVATION_RELU);
	BIND_ENUM_CONSTANT(ACTIVATION_LEAKY_RELU);
	BIND_ENUM_CONSTANT(ACTIVATION_TANH);
	BIND_ENUM_CONSTANT(ACTIVATION_LINEAR);
	BIND_ENUM_CONSTANT(ACTIVATION_BINARY);
	BIND_ENUM_CONSTANT(ACTIVATION_SOFTMAX);
}

BrainArea::BrainArea() {}

void BrainArea::save_knowledge(const String &p_path, bool p_overwrite) {

	ERR_FAIL_COND(p_overwrite == false && FileAccess::exists(p_path));
	Error e;
	FileAccess *f = FileAccess::open(p_path, FileAccess::WRITE, &e);

	if (e != OK) {
		ERR_EXPLAIN("Can't open file" + p_path + " because " + itos(e));
		ERR_FAIL();
	}

	std::vector<uint8_t> buffer;
	if (!get_internal_brain().get_buffer(buffer)) {
		f->close();
		memdelete(f);
		ERR_EXPLAIN("Can't save knowledge. File: " + p_path);
		ERR_FAIL();
	}

	f->store_buffer(buffer.data(), buffer.size());

	f->close();
	memdelete(f);
}

void BrainArea::load_knowledge(const String &p_path) {

	ERR_FAIL_COND(!FileAccess::exists(p_path));

	Error e;
	FileAccess *f = FileAccess::open(p_path, FileAccess::READ, &e);

	if (e != OK) {
		ERR_EXPLAIN("Can't open file because " + itos(e));
		ERR_FAIL();
	}

	std::vector<uint8_t> buffer;

	const uint32_t buffer_size = f->get_len();
	buffer.resize(buffer_size);

	const int readed = f->get_buffer(buffer.data(), buffer_size);
	if (readed != buffer_size) {
		f->close();
		memdelete(f);
		ERR_EXPLAIN("File corrupted: " + p_path);
		ERR_FAIL();
	}

	if (get_internal_brain().is_buffer_corrupted(buffer)) {
		f->close();
		memdelete(f);
		ERR_EXPLAIN("File corrupted: " + p_path);
		ERR_FAIL();
	}

	if (!get_internal_brain().is_buffer_compatible(buffer)) {
		f->close();
		memdelete(f);
		ERR_EXPLAIN("This knowledge has a different brain structure: " + p_path);
		ERR_FAIL();
	}

	if (!get_internal_brain().set_buffer(buffer)) {
		f->close();
		memdelete(f);
		ERR_EXPLAIN("File corrupted: " + p_path);
		ERR_FAIL();
	}

	f->close();
	memdelete(f);
}
