
/**
  @author AndreaCatania
*/

#include "weights_raw_resource_format_loader.h"
#include "modules/brain/areas/sharp_brain_area.h"

WeightsRawResourceFormatLoader::WeightsRawResourceFormatLoader() {
}

WeightsRawResourceFormatLoader::~WeightsRawResourceFormatLoader() {
}

RES WeightsRawResourceFormatLoader::load(const String &p_path, const String &p_original_path, Error *r_error) {
	Ref<SharpBrainAreaStructureFile> f;
	f.instance();
	f->set_file_path(p_path);
	return f;
}

void WeightsRawResourceFormatLoader::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("brainweights");
}

bool WeightsRawResourceFormatLoader::handles_type(const String &p_type) const {
	return p_type == "SharpBrainAreaStructureFile";
}

String WeightsRawResourceFormatLoader::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "brainweights")
		return "SharpBrainAreaStructureFile";
	return "";
}
