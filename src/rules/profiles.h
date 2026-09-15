#ifndef SRC_RULES_PROFILES_H
#define SRC_RULES_PROFILES_H

#include "../utils/data_types.h"

namespace rules_profiles {
const SpeciesProfile* all();
uint8_t count();
const SpeciesProfile* findByName(const char* name);
}

#endif
