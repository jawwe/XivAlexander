#pragma once

#include <memory>
#include <XivAlexanderCommon/Utils_ListenerManager.h>
#include <XivAlexanderCommon/Utils_NumericStatisticsTracker.h>
#include <XivAlexanderCommon/Utils_Win32.h>
#include <XivAlexanderCommon/Utils_Win32_Handle.h>
#include <XivAlexanderCommon/Utils_Win32_LoadedModule.h>

namespace App {
	namespace Network {
		class SocketHook;
	}

	namespace Misc {
		class DebuggerDetectionDisabler;
		class Logger;
	}

	class Config;

	class XivAlexApp {
		const Utils::Win32::LoadedModule m_module;
		const std::shared_ptr<Misc::DebuggerDetectionDisabler> m_detectionDisabler;
		const std::shared_ptr<Misc::Logger> m_logger;
		const std::shared_ptr<Config> m_config;

		struct Implementation;
		friend struct Implementation;
		std::unique_ptr<Implementation> m_pImpl;

		struct Implementation_GameWindow;
		friend struct Implementation_GameWindow;
		std::unique_ptr<Implementation_GameWindow> m_pGameWindow;

		bool m_bInternalUnloadInitiated = false;

		const Utils::Win32::Event m_loadCompleteEvent;

		// needs to be last, as "this" needs to be done initializing
		Utils::Win32::Thread m_hCustomMessageLoop;

	public:
		XivAlexApp();
		~XivAlexApp();

	private:
		void CustomMessageLoopBody();

	public:
		[[nodiscard]] _Maybenull_ HWND GetGameWindowHandle(bool wait = false) const;
		[[nodiscard]] DWORD GetGameWindowThreadId(bool wait = false) const;

		void RunOnGameLoop(std::function<void()> f);
		[[nodiscard]] std::string IsUnloadable() const;

		[[nodiscard]] Network::SocketHook* GetSocketHook();

		[[nodiscard]] const Utils::NumericStatisticsTracker& GetMessagePumpIntervalTrackerUs() const;
		[[nodiscard]] const Utils::NumericStatisticsTracker& GetRenderTimeTakenTrackerUs() const;
		[[nodiscard]] const Utils::NumericStatisticsTracker& GetSocketCallDelayTrackerUs() const;

		void GuaranteePumpBeginCounter(int64_t counterUs);

		static Utils::ListenerManager<XivAlexApp, void, XivAlexApp&> OnAppCreated;
		static XivAlexApp* GetCurrentApp();

	};
}
