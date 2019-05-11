#ifndef WEIGHTS_RAW_RESOURCE_FORMAT_LOADER_H
#define WEIGHTS_RAW_RESOURCE_FORMAT_LOADER_H

#include "core/io/resource_loader.h"

/**
  @author AndreaCatania
*/

class WeightsRawResourceFormatLoader : public ResourceFormatLoader {
public:
	WeightsRawResourceFormatLoader();
	virtual ~WeightsRawResourceFormatLoader();

	virtual RES load(const String &p_path, const String &p_original_path, Error *r_error = NULL);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

#endif // WEIGHTS_RAW_RESOURCE_FORMAT_LOADER_H
