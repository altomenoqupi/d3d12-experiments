#pragma once
#ifndef _D3D12STUFF_H_
#define _D3D12STUFF_H_


#include <iostream>
#include <memory>
#include <string>
#include <windows.h>
#include <vector>
#include <windows.h>
#include <wrl.h>

#include <DirectXColors.h>
#include <DirectXMath.h>
#include <initguid.h>
#include "D3D12.h"
#include "dxgi.h"

#include "UtilsFrameworks.h"

// To resolve linker error for CreateDXGIFactory
#pragma comment(lib, "dxgi.lib")
// To resolve linked error for D3D12 calls
#pragma comment(lib,"d3d12.lib")
// To #include "D3dx12.h"
#pragma comment(lib,"dxguid.lib")

// Base class from which D3D12 clients do D3D12 stuff.
class D3DBaseClient
{
public:
	// Create base class instance.
	// Initialize D3D12 stuff.
	static std::shared_ptr<D3DBaseClient> Create(
		bool debugEnable, UINT width, UINT height, UINT framesPerSecond, UINT swapChainBufferCount, HWND outputWindow)
	{
		if (debugEnable)
		{
			Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
			if (D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) != Result::SUCCESS)
			{
				return nullptr;
			}
			debugController->EnableDebugLayer();
		}

		auto d3d12Client = std::shared_ptr<D3DBaseClient>(new D3DBaseClient(width, height, framesPerSecond, outputWindow));
		Result status = Result::SUCCESS;

		if (d3d12Client->CreateFactory() != Result::SUCCESS)
		{
			return nullptr;
		}

		if (d3d12Client->FillAdapterList() == 0)
		{
			return nullptr;
		}

		uint32_t adapterIdx = d3d12Client->GetAdapter(d3d12Client->m_adapterName);

		if (d3d12Client->CreateDevice(adapterIdx) != Result::SUCCESS)
		{
			return nullptr;
		}

		if (d3d12Client->CreateFences() != Result::SUCCESS)
		{
			return nullptr;
		}

		if (d3d12Client->SetDescriptorSizes() != Result::SUCCESS)
		{
			return nullptr;
		}

		// MSAA support
		if (d3d12Client->ValidateQualitySupport() != Result::SUCCESS)
		{
			return nullptr;
		}

		if (d3d12Client->InitCommandInfra(D3D12_COMMAND_LIST_TYPE_DIRECT) != Result::SUCCESS)
		{
			return nullptr;
		}

		if (d3d12Client->CreateSwapChain() != Result::SUCCESS)
		{
			return nullptr;
		}

		if (d3d12Client->CreateDescriptorHeapsRtvDsv() != Result::SUCCESS)
		{
			return nullptr;
		}

		if (d3d12Client->InitViewPort() != Result::SUCCESS)
		{
			return nullptr;
		}

		if(d3d12Client->InitScissorRectangles() != Result::SUCCESS)
		{
			return nullptr;
		}


		return d3d12Client;
	}

	// No default ctor.
	D3DBaseClient() = delete;
	// Dtor
	virtual ~D3DBaseClient()
	{
		OutputDebugString(L"D3DBaseClient dtor.");
		if (m_comptrDevice != nullptr)
		{
			FlushCommandQueue();
		}
	}

	// Main public methods
	virtual Result OnResize(D3D12_VIEWPORT& viewport, D3D12_RECT& scissorRectangle);
	virtual Result Draw();

private:
	template <D3D12_DESCRIPTOR_HEAP_TYPE type>
	struct BaseDescriptorInfo
	{
		BaseDescriptorInfo() :
			m_type(type), m_size(0)
		{
		};

		Result SetSize(Microsoft::WRL::ComPtr<ID3D12Device> pDevice)
		{
			m_size = pDevice->GetDescriptorHandleIncrementSize(m_type);
			std::cout << "Descriptor type: " << m_type << " with size:" << m_size << ".\n";
			return Result::SUCCESS;
		};

		D3D12_DESCRIPTOR_HEAP_TYPE m_type;
		UINT m_size;
	};

	struct ResourceStuff
	{
		// Swap Chain
		DXGI_SWAP_CHAIN_DESC												m_swapChainDesc;
		Microsoft::WRL::ComPtr <IDXGISwapChain>								m_comptrSwapChain;

		static const UINT												    m_swapChainBufferCount = 2;
		UINT																m_currentBackBuffer = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource>								m_swapChainBuffer[m_swapChainBufferCount];
		Microsoft::WRL::ComPtr<ID3D12Resource>								m_depthStencilBuffer;

		// Constant Buffer Desc Info
		struct BaseDescriptorInfo<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>			m_rtvDescriptorInfo;
		// Shader Resource Desc Info
		struct BaseDescriptorInfo<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>			m_dsvDescriptorInfo;
		// Unordered Access Desc Info
		struct BaseDescriptorInfo<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_srvDescriptorInfo;

		// RTV Heap for the buffer resources in the swap chain
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>						m_comptrRtvHeap;
		// DSV Heap for depth/stencil buffer
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>						m_comptrDsvHeap;
	};

	struct WindowInfo
	{
		HWND m_hMainWnd = nullptr;
	};

	// Private Ctor
	D3DBaseClient(
		UINT width, UINT height, UINT framesPerSecond, HWND outputWindow) :
		m_clientImageWidth(width),
		m_clientImageHeight(height),
		m_windowInfo{outputWindow}
	{
		m_refreshRate={ framesPerSecond , 1 };
	}

	virtual Result CreateFactory();
	virtual uint32_t FillAdapterList();
	virtual uint32_t GetAdapter(std::wstring description);
	virtual Result CreateDevice(uint32_t adapterIdx);
	virtual Result CreateFences();
	virtual Result SetDescriptorSizes();
	virtual Result ValidateQualitySupport();
	virtual Result InitCommandInfra(D3D12_COMMAND_LIST_TYPE cmdListType);
	virtual Result CreateSwapChain();
	virtual Result CreateDescriptorHeapsRtvDsv();
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtv() const;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDsv() const;
	virtual ID3D12Resource* GetCurrentBackBuffer();
	virtual void AdvanceBackBuffer();
	virtual Result FlushCommandQueue();
	virtual Result CreateRenderTargetView();
	virtual Result CreateDepthStencilBufferAndView();
	virtual Result InitViewPort();
	virtual Result InitScissorRectangles();
	virtual Result SetViewPort(
		float width, float height, float topLeftX, float topLeftY, float minDepth, float maxDepth);
	virtual Result SetViewPort(D3D12_VIEWPORT& viewport);
	virtual Result SetScissorRectangles(LONG left, LONG top, LONG right, LONG bottom);
	virtual Result SetScissorRectangles(D3D12_RECT& scissorRectangle);

	Microsoft::WRL::ComPtr<IDXGIFactory> m_comptrDxgiFactory;
	D3D_FEATURE_LEVEL m_MinimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	// We don't own the objects pointed to by the members of the container below.
	std::vector<IDXGIAdapter*> m_adapterList;
	Microsoft::WRL::ComPtr <ID3D12Device> m_comptrDevice;
	Microsoft::WRL::ComPtr <ID3D12Fence> m_comptrFence;
	UINT64 m_currentFenceValue;

	const std::wstring m_adapterName = L"NVIDIA";
	const DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	UINT m_numMsaaQualityLevels = 0;
	const UINT m_msaaSampleCount = 1;
	UINT m_clientImageWidth;
	UINT m_clientImageHeight;
	DXGI_RATIONAL m_refreshRate;
	WindowInfo m_windowInfo;
	ResourceStuff m_resourceStuff;
	D3D12_VIEWPORT m_screenViewport;
	D3D12_RECT m_scissorRectangle;

	Microsoft::WRL::ComPtr <ID3D12PipelineState> m_comptrPipelineState = nullptr;
	Microsoft::WRL::ComPtr <ID3D12CommandQueue> m_comptrCommandQueue;
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> m_comptrGraphicsCommandList;
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator> m_comptrCommandAllocator;
};

class PerformanceMeasurement
{
public:
	PerformanceMeasurement() = default;
	~PerformanceMeasurement() {};
private:

};

#endif // _D3D12STUFF_H_