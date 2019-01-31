#include "profiler.h"

#include "core/math/math_funcs.h"
#include "core/os/os.h"

real_t ProfileData::get_execution_time_sec() const {
	return static_cast<real_t>(USEC_TO_SEC(execution_time));
}

std::map<String, ProfileData> Profiler::data;

Profiler::Profiler() {}

void Profiler::clear() {
	for (
			std::map<String, ProfileData>::iterator i = data.begin();
			i != data.end();
			++i) {

		i->second.clear();
	}
}

ProfileData &Profiler::get_profile_data(const String &p_name) {

	std::map<String, ProfileData>::iterator i = data.find(p_name);

	if (i == data.end()) {
		data[p_name] = ProfileData();
		i = data.find(p_name);
	}

	return i->second;
}

Profile::Profile(ProfileData &p_data) :
		data(p_data) {
	initial_usec = OS::get_singleton()->get_ticks_usec();
}

Profile::~Profile() {
	data.execution_time += OS::get_singleton()->get_ticks_usec() - initial_usec;
	++data.call_count;
}
