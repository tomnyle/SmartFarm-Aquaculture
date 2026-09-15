#ifndef SRC_SENSORS_SENSORS_H
#define SRC_SENSORS_SENSORS_H

#include "../utils/data_types.h"

class SensorDriver {
 public:
  virtual ~SensorDriver() = default;
  virtual bool begin() = 0;
  virtual SensorReading read() = 0;
  virtual bool isEnabled() const = 0;
};

#endif
