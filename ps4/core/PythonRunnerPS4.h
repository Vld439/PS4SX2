// SPDX-License-Identifier: GPL-3.0+
#pragma once

namespace PS4Python
{
	// Initialize the embedded Python subsystem
	bool Initialize();

	// If the current game is Dragon Ball Z Budokai Tenkaichi 3, launch the background trainer thread
	bool StartTrainerIfMatching(const char* disc_serial);

	// Stop trainer thread and cleanup runtime
	void Shutdown();

	// Check if trainer is currently running
	bool IsRunning();
} // namespace PS4Python
