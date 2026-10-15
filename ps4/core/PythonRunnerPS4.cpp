// SPDX-License-Identifier: GPL-3.0+
#include "PythonRunnerPS4.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

extern void ProbeLog(const char* format, ...);

#if defined(ENABLE_EMBEDDED_PYTHON)
#include <Python.h>

namespace PS4Python
{
	static std::atomic_bool s_running{false};
	static std::thread s_thread;

	bool Initialize()
	{
		if (Py_IsInitialized())
			return true;

		ProbeLog("PS4Python: Initializing CPython runtime...\n");
		Py_Initialize();
		return Py_IsInitialized() != 0;
	}

	bool StartTrainerIfMatching(const char* disc_serial)
	{
		if (!disc_serial || !disc_serial[0])
			return false;

		// SLUS-21678 (USA), SLES-54945 (EUR), SLPS-25815 (JPN)
		const bool is_bt3 = (strstr(disc_serial, "21678") != nullptr ||
		                     strstr(disc_serial, "54945") != nullptr ||
		                     strstr(disc_serial, "25815") != nullptr);

		if (!is_bt3)
			return false;

		if (!Initialize())
		{
			ProbeLog("PS4Python: Failed to initialize Python runtime.\n");
			return false;
		}

		if (s_running.load(std::memory_order_acquire))
			return true;

		s_running.store(true, std::memory_order_release);
		ProbeLog("PS4Python: Budokai Tenkaichi 3 detected (%s). Spawning Tag Team trainer...\n", disc_serial);

		s_thread = std::thread([]() {
			// Give VM and PINE loop 2 seconds to establish the emulation loop
			std::this_thread::sleep_for(std::chrono::seconds(2));

			const char* py_script =
				"import sys, os\n"
				"mod_path = '/data/PS4SX2/mods/bt3/tools'\n"
				"if mod_path not in sys.path:\n"
				"    sys.path.insert(0, mod_path)\n"
				"print('[PS4 TagTeam] Starting trainer script from:', mod_path)\n"
				"try:\n"
				"    import fresh_team_trainer\n"
				"    fresh_team_trainer.start()\n"
				"except Exception as e:\n"
				"    print('[PS4 TagTeam] Trainer error:', e)\n";

			PyRun_SimpleString(py_script);
			s_running.store(false, std::memory_order_release);
		});

		return true;
	}

	void Shutdown()
	{
		if (!s_running.load(std::memory_order_acquire))
			return;

		s_running.store(false, std::memory_order_release);
		if (s_thread.joinable())
			s_thread.join();

		if (Py_IsInitialized())
		{
			ProbeLog("PS4Python: Finalizing Python runtime...\n");
			Py_Finalize();
		}
	}

	bool IsRunning()
	{
		return s_running.load(std::memory_order_acquire);
	}
} // namespace PS4Python

#else

namespace PS4Python
{
	bool Initialize() { return false; }
	bool StartTrainerIfMatching(const char*) { return false; }
	void Shutdown() {}
	bool IsRunning() { return false; }
} // namespace PS4Python

#endif
