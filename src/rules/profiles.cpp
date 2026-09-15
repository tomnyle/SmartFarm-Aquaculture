#include <string.h>
#include "profiles.h"
#include "../config/profiles.h"

namespace {
constexpr SpeciesProfile kProfiles[] = {
    {config_profiles::PROFILE_KOI, {18.0f, 22.0f, 26.0f, 15.0f, 30.0f}, {6.8f, 7.4f, 8.2f, 6.5f, 8.8f}, {5.0f, 6.5f, 3.0f}, {50.0f, 80.0f, 95.0f, 20.0f}},
    {config_profiles::PROFILE_CATFISH, {24.0f, 28.0f, 32.0f, 20.0f, 35.0f}, {6.5f, 7.2f, 8.0f, 6.0f, 8.5f}, {4.0f, 5.5f, 2.5f}, {45.0f, 80.0f, 95.0f, 20.0f}},
    {config_profiles::PROFILE_SHRIMP, {26.0f, 28.0f, 30.0f, 20.0f, 35.0f}, {7.0f, 7.8f, 8.5f, 6.5f, 9.0f}, {5.0f, 7.0f, 3.0f}, {50.0f, 80.0f, 95.0f, 20.0f}},
    {config_profiles::PROFILE_TILAPIA, {24.0f, 28.0f, 32.0f, 18.0f, 35.0f}, {6.5f, 7.5f, 8.5f, 6.0f, 9.0f}, {4.0f, 6.0f, 2.5f}, {45.0f, 80.0f, 95.0f, 20.0f}},
};
}

namespace rules_profiles {
const SpeciesProfile* all() { return kProfiles; }
uint8_t count() { return sizeof(kProfiles) / sizeof(kProfiles[0]); }
const SpeciesProfile* findByName(const char* name) {
  for (uint8_t i = 0; i < count(); ++i) {
    if (strcmp(kProfiles[i].name, name) == 0) return &kProfiles[i];
  }
  return nullptr;
}

bool isSupported(const char* name) {
  for (uint8_t i = 0; i < count(); ++i) {
    if (strcmp(kProfiles[i].name, name) == 0) return true;
  }
  return false;
}
}
