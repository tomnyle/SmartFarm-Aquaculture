#ifndef SPECIES_RULES_H
#define SPECIES_RULES_H

#include <Arduino.h>

struct SpeciesRule {
  const char* name;

  float temp_min;
  float temp_max;
  float temp_critical_low;
  float temp_critical_high;

  float ph_min;
  float ph_max;

  float do_min;
  float do_critical;

  float co2_max;
  float turbidity_max;
};

static const SpeciesRule SPECIES_CARP = {
  "Cá Chép",
  20.0, 28.0, 15.0, 30.0,
  7.0, 8.5,
  5.5, 4.0,
  10.0, 80.0
};

static const SpeciesRule SPECIES_TILAPIA = {
  "Rô Phi",
  22.0, 30.0, 18.0, 33.0,
  6.5, 8.5,
  4.5, 3.5,
  12.0, 100.0
};

static const SpeciesRule SPECIES_CATFISH = {
  "Cá Tra",
  24.0, 30.0, 20.0, 32.0,
  6.5, 8.0,
  5.0, 4.0,
  10.0, 100.0
};

static const SpeciesRule SPECIES_SHRIMP = {
  "Tôm Thẻ",
  26.0, 32.0, 24.0, 33.0,
  7.5, 8.8,
  5.5, 4.5,
  8.0, 60.0
};

inline const SpeciesRule* getSpeciesRule(const char* species) {
  if (strcmp(species, "Cá Chép") == 0) return &SPECIES_CARP;
  if (strcmp(species, "Rô Phi") == 0) return &SPECIES_TILAPIA;
  if (strcmp(species, "Cá Tra") == 0) return &SPECIES_CATFISH;
  if (strcmp(species, "Tôm Thẻ") == 0) return &SPECIES_SHRIMP;
  return &SPECIES_TILAPIA;
}

#endif