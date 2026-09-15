#ifndef SRC_MODES_SCHEDULE_MODE_H
#define SRC_MODES_SCHEDULE_MODE_H

#include <time.h>
#include "../relays/relay_manager.h"
#include "../utils/data_types.h"

class ScheduleMode {
 public:
  void configureDefaults(ScheduleState& schedule) const;
  bool updateFromJson(const String& json, ScheduleState& schedule) const;
  void tick(ScheduleState& schedule, RelayManager& relayManager) const;
};

#endif
