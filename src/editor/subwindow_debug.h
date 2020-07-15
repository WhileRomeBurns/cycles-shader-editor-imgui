#pragma once

/**
 * @file
 * @brief Defines DebugSubwindow.
 */

#include <string>

#include "event.h"

namespace cse {
	class DebugSubwindow {
	public:
		DebugSubwindow();

		InterfaceEventArray run() const;
		
		void do_event(const InterfaceEvent& event);

	private:
		std::string run_validation() const;

		std::string message;
	};
}
