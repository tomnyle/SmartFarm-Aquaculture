#ifndef SRC_SYSTEM_ERROR_HANDLER_H
#define SRC_SYSTEM_ERROR_HANDLER_H

#include "system_state.h"

class ErrorHandler {
 public:
  void setError(SystemState& state, const String& error) const;
  void clear(SystemState& state) const;
};

#endif
