#include <cstring>
#include "profiles.h"

static const SpeciesProfile PROFILES[] = {
    {"tilapia", "Tilapia", 24.0F, 30.0F, 6.5F, 8.5F, 5.0F, true},
    {"koi", "Koi", 20.0F, 28.0F, 6.8F, 8.2F, 5.5F, true},
    {"shrimp", "Tom the / Shrimp", 27.0F, 31.0F, 7.5F, 8.5F, 5.0F, true}
};

const SpeciesProfile* getSpeciesProfile(const char* profile_id) {
    for (const auto& profile : PROFILES) {
        if (strcmp(profile.id, profile_id) == 0) {
            return &profile;
        }
    }
    return &PROFILES[0];
}
