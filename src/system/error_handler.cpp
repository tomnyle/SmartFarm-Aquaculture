#include "error_handler.h"

void ErrorHandler::setError(SystemState& state, const String& error) const {
  state.error = error;
  state.status = "ERROR";
}

void ErrorHandler::clear(SystemState& state) const {
  state.error = "";
  state.status = "RUNNING";
}
