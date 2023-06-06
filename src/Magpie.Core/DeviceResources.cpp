#include "pch.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "StrUtils.h"
#include "DirectXHelper.h"

namespace Magpie::Core {

bool DeviceResources::Initialize(HWND /*hwndScaling*/, const ScalingOptions& options) noexcept {
#ifdef _DEBUG
	UINT flag = DXGI_CREATE_FACTORY_DEBUG;
#else
	UINT flag = 0;
#endif // _DEBUG

	winrt::com_ptr<IDXGIFactory7> dxgiFactory;
	HRESULT hr = CreateDXGIFactory2(flag, IID_PPV_ARGS(dxgiFactory.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDXGIFactory2 失败", hr);
		return false;
	}

	// 检查可变帧率支持
	BOOL supportTearing = FALSE;
	hr = dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supportTearing, sizeof(supportTearing));
	if (FAILED(hr)) {
		Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
	}

	Logger::Get().Info(fmt::format("可变刷新率支持：{}", supportTearing ? "是" : "否"));

	if (!options.IsVSync() && !supportTearing) {
		Logger::Get().Error("当前显示器不支持可变刷新率");
		return false;
	}

	if (!_ObtainAdapterAndDevice(dxgiFactory.get(), options.graphicsCard)) {
		Logger::Get().Error("找不到可用的图形适配器");
		return false;
	}

	return true;
}

static void LogAdapter(const DXGI_ADAPTER_DESC1& adapterDesc) noexcept {
	Logger::Get().Info(fmt::format("当前图形适配器：\n\tVendorId：{:#x}\n\tDeviceId：{:#x}\n\t描述：{}",
		adapterDesc.VendorId, adapterDesc.DeviceId, StrUtils::UTF16ToUTF8(adapterDesc.Description)));
}

bool DeviceResources::_ObtainAdapterAndDevice(IDXGIFactory7* dxgiFactory, int adapterIdx) noexcept {
	winrt::com_ptr<IDXGIAdapter1> adapter;

	if (adapterIdx >= 0) {
		HRESULT hr = dxgiFactory->EnumAdapters1(adapterIdx, adapter.put());
		if (SUCCEEDED(hr)) {
			DXGI_ADAPTER_DESC1 desc;
			hr = adapter->GetDesc1(&desc);
			if (SUCCEEDED(hr)) {
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
					Logger::Get().Warn("用户指定的显示卡为 WARP，已忽略");
				} else if (_TryCreateD3DDevice(adapter)) {
					LogAdapter(desc);
					return true;
				} else {
					Logger::Get().Warn("用户指定的显示卡不支持 FL 11");
				}
			} else {
				Logger::Get().Error("GetDesc1 失败");
			}
		} else {
			Logger::Get().Warn("未找到用户指定的显示卡");
		}
	}

	// 枚举查找第一个支持 D3D11 的图形适配器
	for (UINT adapterIndex = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.put()));
		++adapterIndex
	) {
		DXGI_ADAPTER_DESC1 desc;
		HRESULT hr = adapter->GetDesc1(&desc);
		if (FAILED(hr)) {
			continue;
		}

		// 忽略 WARP
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		if (_TryCreateD3DDevice(adapter)) {
			LogAdapter(desc);
			return true;
		}
	}

	// 作为最后手段，回落到 Basic Render Driver Adapter（WARP）
	// https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp
	HRESULT hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 WARP 设备失败", hr);
		return false;
	}

	Logger::Get().Info("已创建 WARP 设备");
	return true;
}

bool DeviceResources::_TryCreateD3DDevice(const winrt::com_ptr<IDXGIAdapter1>& adapter) noexcept {
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	UINT nFeatureLevels = ARRAYSIZE(featureLevels);

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	if (DirectXHelper::IsDebugLayersAvailable()) {
		// 在 DEBUG 配置启用调试层
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	winrt::com_ptr<ID3D11Device> d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext> d3dDC;
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		adapter.get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createDeviceFlags,
		featureLevels,
		nFeatureLevels,
		D3D11_SDK_VERSION,
		d3dDevice.put(),
		&featureLevel,
		d3dDC.put()
	);

	if (FAILED(hr)) {
		Logger::Get().ComError("D3D11CreateDevice 失败", hr);
		return false;
	}

	std::string_view fl;
	switch (featureLevel) {
	case D3D_FEATURE_LEVEL_11_1:
		fl = "11.1";
		break;
	case D3D_FEATURE_LEVEL_11_0:
		fl = "11.0";
		break;
	default:
		fl = "未知";
		break;
	}
	Logger::Get().Info(fmt::format("已创建 D3D Device\n\t功能级别：{}", fl));

	_d3dDevice = d3dDevice.try_as<ID3D11Device5>();
	if (!_d3dDevice) {
		Logger::Get().Error("获取 ID3D11Device1 失败");
		return false;
	}
	
	_graphicsAdapter = adapter.try_as<IDXGIAdapter4>();
	if (!_graphicsAdapter) {
		Logger::Get().ComError("获取 IDXGIAdapter4 失败", hr);
		return false;
	}

	return true;
}

}
