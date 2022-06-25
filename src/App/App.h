#pragma once

#include "App.g.h"
#include "App.base.h"
#include "Settings.h"
#include "HotkeyManager.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void OnClose();

	bool Initialize(Magpie::App::Settings const& settings, uint64_t hwndHost);

	uint64_t HwndHost() const {
		return _hwndHost;
	}

	Magpie::App::Settings Settings() const {
		return _settings;
	}

	Magpie::App::HotkeyManager HotkeyManager() const {
		return _hotkeyManager;
	}

	Magpie::Runtime::MagRuntime MagRuntime() const {
		return _magRuntime;
	}

	event_token HostWndFocusChanged(EventHandler<bool> const& handler);
	void HostWndFocusChanged(event_token const& token) noexcept;

	void OnHostWndFocusChanged(bool isFocused);
	bool IsHostWndFocused() const noexcept {
		return _isHostWndFocused;
	}

private:
	void _HotkeyManger_HotkeyPressed(IInspectable const&, HotkeyAction action);

	Magpie::App::Settings _settings{ nullptr };
	Magpie::Runtime::MagSettings _magSettings;
	Magpie::App::HotkeyManager _hotkeyManager{ nullptr };
	Magpie::Runtime::MagRuntime _magRuntime;
	uint64_t _hwndHost{};

	event<EventHandler<bool>> _hostWndFocusChangedEvent;
	bool _isHostWndFocused = false;

	Magpie::App::HotkeyManager::HotkeyPressed_revoker _hotkeyPressedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
