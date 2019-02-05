#ifndef PROFILER_H
#define PROFILER_H

#include "core/typedefs.h"
#include "core/ustring.h"
#include <map>

/**
  * Really simple profiler to track flex server performances
  */

#ifdef DEBUG_ENABLED
#define PROFILE(name) Profile __profiler_tcaerdna_(Profiler::get_profile_data((name)));
#else
#define PROFILE(name)
#endif

struct ProfileData {
	uint64_t execution_time;
	uint64_t call_count;

	ProfileData() {
		clear();
	}

	float get_execution_time_sec() const;

	void clear() {
		execution_time = 0;
		call_count = 0;
	}
};

class Profiler {

	static std::map<String, ProfileData> data;

public:
	Profiler();

	static void clear();
	static ProfileData &get_profile_data(const String &p_name);
	static const std::map<String, ProfileData> &get_data() {
		return data;
	}
};

class Profile {
	ProfileData &data;
	uint64_t initial_usec;

public:
	Profile(ProfileData &p_data);
	~Profile();
};

#endif // PROFILER_H
