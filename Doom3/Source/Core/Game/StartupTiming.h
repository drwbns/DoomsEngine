#pragma once

#include <cstdlib>

namespace dooms
{
	/// <summary>
	/// Whether start up phase timings are written to startup_timing.txt.
	///
	/// Off unless DOOMS_STARTUP_TIMING is set in the environment. An
	/// environment variable rather than a config key because the first phase
	/// being timed is the one that reads the config: by the time a key could
	/// be consulted, the measurement it would gate has already happened.
	///
	/// The file exists because the in engine log cannot carry this. The
	/// garbage collector prints a line per object it collects, so anything
	/// logged during start up is buried thousands of lines deep before the
	/// window is interactive. That made a file worth writing, but not worth
	/// writing on every run of a shipped build.
	///
	/// Read once. Nothing changes the environment while the engine runs, and
	/// this is called from inside the phases it measures.
	/// </summary>
	inline bool IsStartupTimingEnabled()
	{
		// getenv_s rather than getenv, which this project's SDL checks reject,
		// and rather than the Win32 call, which would pull windows.h into
		// everything that includes this. Asking for the size alone tells us
		// whether the variable is set without allocating anything.
		static const bool bIsEnabled = []()
		{
			size_t requiredSize = 0;
			return (getenv_s(&requiredSize, nullptr, 0, "DOOMS_STARTUP_TIMING") == 0) && (requiredSize > 0);
		}();

		return bIsEnabled;
	}
}
