#pragma once
#include "pch.h"


class FrameSourceBase {
public:
	FrameSourceBase() {}

	virtual ~FrameSourceBase() {}

	// 不可复制，不可移动
	FrameSourceBase(const FrameSourceBase&) = delete;
	FrameSourceBase(FrameSourceBase&&) = delete;

	virtual bool Initialize() = 0;

	virtual ComPtr<ID3D11Texture2D> GetOutput() = 0;

	enum class UpdateState {
		NewFrame,
		NoUpdate,
		Waiting,
		Error
	};

	virtual UpdateState Update() = 0;

	virtual bool HasRoundCornerInWin11() = 0;

	virtual bool IsScreenCapture() = 0;

protected:
	static bool _GetWindowDpiScale(HWND hWnd, float& dpiScale);

	static bool _GetDpiAwareWindowClientOffset(HWND hWnd, POINT& clientOffset);

	static bool _CenterWindowIfNecessary(HWND hWnd, const RECT& rcWork);
};
