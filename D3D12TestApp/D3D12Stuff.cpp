
#include "D3D12Stuff.h"
#include "D3DX12.h"

Result D3DBaseClient::CreateFactory()
{
	Result ret = Result::SUCCESS;
	CreateDXGIFactory1(IID_PPV_ARGS(&m_comptrDxgiFactory));
	return ret;
}

uint32_t D3DBaseClient::FillAdapterList()
{
	uint32_t numAdaptersFound = 0;

	IDXGIAdapter* pAdapter = nullptr;
	while (m_comptrDxgiFactory->EnumAdapters(numAdaptersFound, &pAdapter) !=
		DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		pAdapter->GetDesc(&desc);
		std::wstring text = L"***Adapter[";
		text += std::to_wstring(numAdaptersFound);
		text += L"]: ";
		text += desc.Description;
		text += L"\n";
		OutputDebugString(L"Found adapter: ");
		OutputDebugString(text.c_str());
		m_adapterList.push_back(pAdapter);
		++numAdaptersFound;
	}

	return numAdaptersFound;
}

uint32_t D3DBaseClient::GetAdapter(std::wstring description)
{
	uint32_t ret = 0;
	for (IDXGIAdapter* pAdapter : m_adapterList)
	{
		DXGI_ADAPTER_DESC desc;
		pAdapter->GetDesc(&desc);
		if (std::wstring(desc.Description).find(description) != std::wstring::npos)
		{
			return ret;
		}
		else
		{
			++ret;
		}
	}

	return ret;
}

Result D3DBaseClient::CreateDevice(uint32_t adapterIdx)
{
	HRESULT result = D3D12CreateDevice(
		(adapterIdx >= m_adapterList.size()) ? nullptr : m_adapterList[adapterIdx],
		m_MinimumFeatureLevel,
		IID_PPV_ARGS(&m_comptrDevice));
	if (S_OK != result)
	{
		std::cout << "D3D12CreateDevice() failed with " << result << "!\n";
		m_comptrDevice = (ID3D12Device*)nullptr;
		return Result::FAIL;
	}
	return Result::SUCCESS;
}

Result D3DBaseClient::CreateFences()
{
	m_currentFenceValue = 0ull;
	HRESULT result = m_comptrDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_comptrFence));
	if (S_OK != result)
	{
		std::cout << "CreateFence() failed with " << result << "!\n";
		return Result::FAIL;
	}
	return Result::SUCCESS;
}

Result D3DBaseClient::ValidateQualitySupport()
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multiSampleQualityLevels;
	multiSampleQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	multiSampleQualityLevels.Format = m_backBufferFormat;
	multiSampleQualityLevels.NumQualityLevels = 0;
	multiSampleQualityLevels.SampleCount = m_msaaSampleCount;

	HRESULT result = m_comptrDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&multiSampleQualityLevels,
		sizeof(multiSampleQualityLevels));
	if (S_OK != result)
	{
		std::cout << "CheckFeatureSupport() failed with " << result << "!\n";
		return Result::FAIL;
	}
	else if (multiSampleQualityLevels.NumQualityLevels == 0)
	{
		std::cout << "CheckFeatureSupport() returned unexpected NumQualityLevels!\n";
		return Result::FAIL;
	}
	m_numMsaaQualityLevels = multiSampleQualityLevels.NumQualityLevels;

	return Result::SUCCESS;
}

// 
// Create command queue, command list, command allocator.
//
Result D3DBaseClient::InitCommandInfra(D3D12_COMMAND_LIST_TYPE cmdListType)
{
	// Command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = cmdListType;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HRESULT result = m_comptrDevice->CreateCommandQueue(
		&queueDesc, IID_PPV_ARGS(&m_comptrCommandQueue));
	if (S_OK != result)
	{
		std::cout << "CreateCommandQueue() failed with " << result << "!\n";
		return Result::FAIL;
	}
	// Command allocator
	result = m_comptrDevice->CreateCommandAllocator(
		cmdListType, IID_PPV_ARGS(&m_comptrCommandAllocator));
	if (S_OK != result)
	{
		std::cout << "CreateCommandAllocator() failed with " << result << "!\n";
		return Result::FAIL;
	}

	// Command list
	result = m_comptrDevice->CreateCommandList(
		0,
		cmdListType,
		m_comptrCommandAllocator.Get(),
		m_comptrPipelineState.Get(),
		IID_PPV_ARGS(m_comptrGraphicsCommandList.GetAddressOf()));
	if (S_OK != result)
	{
		std::cout << "CreateCommandList() failed with " << result << "!\n";
		return Result::FAIL;
	}

	return Result::SUCCESS;
}

Result D3DBaseClient::CreateSwapChain()
{
	m_resourceStuff.m_comptrSwapChain.Reset();

	m_resourceStuff.m_swapChainDesc.BufferDesc.Width = m_clientImageWidth;
	m_resourceStuff.m_swapChainDesc.BufferDesc.Height = m_clientImageHeight;
	m_resourceStuff.m_swapChainDesc.BufferDesc.RefreshRate.Numerator = m_refreshRate.Numerator;
	m_resourceStuff.m_swapChainDesc.BufferDesc.RefreshRate.Denominator = m_refreshRate.Denominator;
	m_resourceStuff.m_swapChainDesc.BufferDesc.Format = m_backBufferFormat;
	m_resourceStuff.m_swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_resourceStuff.m_swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	m_resourceStuff.m_swapChainDesc.SampleDesc.Count = m_msaaSampleCount;
	m_resourceStuff.m_swapChainDesc.SampleDesc.Quality = m_numMsaaQualityLevels ? (m_numMsaaQualityLevels - 1) : 0;
	m_resourceStuff.m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_resourceStuff.m_swapChainDesc.BufferCount = m_resourceStuff.m_swapChainBufferCount;
	m_resourceStuff.m_swapChainDesc.OutputWindow = m_windowInfo.m_hMainWnd;
	m_resourceStuff.m_swapChainDesc.Windowed = true;
	m_resourceStuff.m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	m_resourceStuff.m_swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	HRESULT result = m_comptrDxgiFactory->CreateSwapChain(
		m_comptrCommandQueue.Get(),
		&m_resourceStuff.m_swapChainDesc,
		m_resourceStuff.m_comptrSwapChain.GetAddressOf());
	if (S_OK != result)
	{
		std::cout << "CreateSwapChain() failed with " << result << "!\n";
		return Result::FAIL;
	}

	return Result::SUCCESS;
}

Result D3DBaseClient::SetDescriptorSizes()
{
	HRESULT result = m_resourceStuff.m_rtvDescriptorInfo.SetSize(m_comptrDevice);
	if (S_OK != result)
	{
		std::cout << "SetSize() for RTV desc. failed with " << result << "!\n";
		return Result::FAIL;
	}
	result = m_resourceStuff.m_dsvDescriptorInfo.SetSize(m_comptrDevice);
	if (S_OK != result)
	{
		std::cout << "SetSize() for DSV desc. failed with " << result << "!\n";
		return Result::FAIL;
	}
	result = m_resourceStuff.m_srvDescriptorInfo.SetSize(m_comptrDevice);
	if (S_OK != result)
	{
		std::cout << "SetSize() for SRV desc. failed with " << result << "!\n";
		return Result::FAIL;
	}

	return Result::SUCCESS;
}

// Create heaps
Result D3DBaseClient::CreateDescriptorHeapsRtvDsv()
{
	HRESULT result = S_OK;
	// RTV Heap for swap chain buffers
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = m_resourceStuff.m_swapChainDesc.BufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	result = m_comptrDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(m_resourceStuff.m_comptrRtvHeap.GetAddressOf()));
	if (S_OK != result)
	{
		std::cout << "CreateDescriptorHeap() failed with " << result << "!\n";
		return Result::FAIL;
	}

	// DSV heap for 1 DS buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	result = m_comptrDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(m_resourceStuff.m_comptrDsvHeap.GetAddressOf()));
	if (S_OK != result)
	{
		std::cout << "CreateDescriptorHeap() failed with " << result << "!\n";
		return Result::FAIL;
	}

	return Result::SUCCESS;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DBaseClient::GetCurrentRtv() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_resourceStuff.m_comptrRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_resourceStuff.m_currentBackBuffer,
		m_resourceStuff.m_rtvDescriptorInfo.m_size);
}
D3D12_CPU_DESCRIPTOR_HANDLE D3DBaseClient::GetDsv() const
{
	return m_resourceStuff.m_comptrDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* D3DBaseClient::GetCurrentBackBuffer()
{
	return m_resourceStuff.m_swapChainBuffer[m_resourceStuff.m_currentBackBuffer].Get();
}

void D3DBaseClient::AdvanceBackBuffer()
{
	m_resourceStuff.m_currentBackBuffer = 
		(m_resourceStuff.m_currentBackBuffer + 1) % m_resourceStuff.m_swapChainBufferCount;
}

Result D3DBaseClient::CreateRenderTargetView()
{
	// Create RTV to the back buffer to bind the latter to the output merger stage of the pipeline.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
		m_resourceStuff.m_comptrRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT bufferIdx = 0; bufferIdx < m_resourceStuff.m_swapChainDesc.BufferCount; bufferIdx++)
	{
		// Obtain a pointer to the back buffer resource.
		HRESULT result = m_resourceStuff.m_comptrSwapChain->GetBuffer(
			bufferIdx,
			IID_PPV_ARGS(&m_resourceStuff.m_swapChainBuffer[bufferIdx]));
		if (S_OK != result)
		{
			std::cout << "GetBuffer() failed with " << result << "!\n";
			return Result::FAIL;
		}

		// Create RTV
		auto p = m_resourceStuff.m_swapChainBuffer[bufferIdx].Get();
		// TODO: the following will throw an exception. Handle it.
		m_comptrDevice->CreateRenderTargetView(
			p, nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_resourceStuff.m_rtvDescriptorInfo.m_size);
	}

	return Result::SUCCESS;
}

Result D3DBaseClient::CreateDepthStencilBufferAndView()
{
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_resourceStuff.m_swapChainDesc.BufferDesc.Width;
	depthStencilDesc.Height = m_resourceStuff.m_swapChainDesc.BufferDesc.Height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	depthStencilDesc.SampleDesc.Count = m_msaaSampleCount;
	depthStencilDesc.SampleDesc.Quality = m_numMsaaQualityLevels ? (m_numMsaaQualityLevels - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// Create resource (Depth/Stencil Buffer)
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_depthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT result = m_comptrDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(m_resourceStuff.m_depthStencilBuffer.GetAddressOf()));
	if (S_OK != result)
	{
		std::cout << "CreateCommittedResource() failed with " << result << "!\n";
		return Result::FAIL;
	}

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_depthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	m_comptrDevice->CreateDepthStencilView(
		m_resourceStuff.m_depthStencilBuffer.Get(), &dsvDesc, GetDsv());

	return Result::SUCCESS;
}

Result D3DBaseClient::FlushCommandQueue()
{
	Result ret = Result::SUCCESS;
	m_currentFenceValue++;

	// New fence point in the command queue.
	HRESULT result = m_comptrCommandQueue->Signal(m_comptrFence.Get(), m_currentFenceValue);
	if (S_OK != result)
	{
		std::cout << "Signal() failed with " << result << "!\n";
		return Result::FAIL;
	}

	// Wait for the GPU to update teh fence point.
	if (m_comptrFence->GetCompletedValue() != m_currentFenceValue)
	{
		HANDLE fenceEventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		result = m_comptrFence->SetEventOnCompletion(m_currentFenceValue, fenceEventHandle);
		if (S_OK != result)
		{
			std::cout << "SetEventOnCompletion() failed with " << result << "!\n";
			ret = Result::FAIL;
		}
		else
		{
			WaitForSingleObject(fenceEventHandle, INFINITE);
		}

		CloseHandle(fenceEventHandle);
	}

	return ret;
}

Result D3DBaseClient::InitViewPort()
{
	Result result = Result::SUCCESS;

	m_screenViewport.Width = static_cast<float>(m_clientImageWidth);
	m_screenViewport.Height = static_cast<float>(m_clientImageHeight);
	m_screenViewport.TopLeftX = 0.0f;
	m_screenViewport.TopLeftY = 0.0f;
	m_screenViewport.MinDepth = 0.0f;
	m_screenViewport.MaxDepth = 1.0f;

	return result;
}

Result D3DBaseClient::InitScissorRectangles()
{
	Result result = Result::SUCCESS;

	m_scissorRectangle.left = 0;
	m_scissorRectangle.top = 0;
	m_scissorRectangle.right = m_clientImageWidth;
	m_scissorRectangle.bottom = m_clientImageHeight;

	return result;
}

Result D3DBaseClient::SetViewPort(
	float width, float height, float topLeftX, float topLeftY, float minDepth, float maxDepth)
{
	D3D12_VIEWPORT viewport = {
				.TopLeftX = topLeftX,
				.TopLeftY = topLeftY,
				.Width = width, 
				.Height = height,
				.MinDepth = minDepth,
				.MaxDepth = maxDepth,   
		};

	return SetViewPort(viewport);
}
Result D3DBaseClient::SetViewPort(D3D12_VIEWPORT& viewport)
{
	Result result = Result::SUCCESS;
	if (((viewport.Width + viewport.TopLeftX) > m_clientImageWidth) ||
		((viewport.Height + viewport.TopLeftY) > m_clientImageHeight))
	{
		return Result::INVALID_PARAMETERS;
	}

	m_screenViewport.Width		= viewport.Width;
	m_screenViewport.Height		= viewport.Height;
	m_screenViewport.TopLeftX	= viewport.TopLeftX;
	m_screenViewport.TopLeftY	= viewport.TopLeftY;
	m_screenViewport.MinDepth	= viewport.MinDepth;
	m_screenViewport.MaxDepth	= viewport.MaxDepth;

	// m_comptrGraphicsCommandList->RSSetViewports(1, &m_screenViewport);

	return result;
}

Result D3DBaseClient::SetScissorRectangles(LONG left, LONG top, LONG right, LONG bottom)
{
	D3D12_RECT scissorRectangle = {
		.left = left,
		.top = top,
		.right = right,
		.bottom = bottom
	};

	return SetScissorRectangles(scissorRectangle);
}

Result D3DBaseClient::SetScissorRectangles(D3D12_RECT& scissorRectangle)
{
	Result result = Result::SUCCESS;
	if ((scissorRectangle.right - scissorRectangle.left) > m_clientImageWidth)
	{
		return Result::INVALID_PARAMETERS;
	}
	if ((scissorRectangle.bottom - scissorRectangle.top) > m_clientImageHeight)
	{
		return Result::INVALID_PARAMETERS;
	}

	m_scissorRectangle.left		= scissorRectangle.left;
	m_scissorRectangle.top		= scissorRectangle.top;
	m_scissorRectangle.right	= scissorRectangle.right;
	m_scissorRectangle.bottom	= scissorRectangle.bottom;

	// m_comptrGraphicsCommandList->RSSetScissorRects(1, &m_scissorRectangle);

	return result;
}

Result D3DBaseClient::OnResize(D3D12_VIEWPORT& viewport, D3D12_RECT& scissorRectangle)
{
	Result result = Result::SUCCESS;

	assert(m_comptrDevice != nullptr);
	assert(m_resourceStuff.m_comptrSwapChain != nullptr);

	FlushCommandQueue();

	// Reset command list.
	m_comptrGraphicsCommandList->Reset(m_comptrCommandAllocator.Get(), nullptr);

	// Release existing resources
	for (UINT bufferIdx = 0; bufferIdx < m_resourceStuff.m_swapChainDesc.BufferCount; bufferIdx++)
	{
		m_resourceStuff.m_swapChainBuffer[bufferIdx].Reset();
	}
	m_resourceStuff.m_depthStencilBuffer.Reset();

	// Resize the swap chain.
	m_resourceStuff.m_comptrSwapChain->ResizeBuffers(
		m_resourceStuff.m_swapChainDesc.BufferCount,
		m_resourceStuff.m_swapChainDesc.BufferDesc.Width,
		m_resourceStuff.m_swapChainDesc.BufferDesc.Height,
		m_resourceStuff.m_swapChainDesc.BufferDesc.Format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	// Reset current back buffer.
	m_resourceStuff.m_currentBackBuffer = 0;

	result = CreateRenderTargetView();
	if (Result::SUCCESS != result)
	{
		return result;
	}

	result = CreateDepthStencilBufferAndView();
	if (Result::SUCCESS != result)
	{
		return result;
	}

	// Transition the resource from its initial state to be used as a depth buffer.
	auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_resourceStuff.m_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_comptrGraphicsCommandList->ResourceBarrier(
		1,
		&transition);

	// Execute the resize commands.
	if (S_OK != m_comptrGraphicsCommandList->Close())
	{
		return Result::FAIL;
	}
	ID3D12CommandList* cmdsLists[] = { m_comptrGraphicsCommandList.Get() };
	m_comptrCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait for command to complete
	result = FlushCommandQueue();
	if (Result::SUCCESS != result)
	{
		return result;
	}

	result = SetViewPort(viewport);
	if (Result::SUCCESS != result)
	{
		return result;
	}

	result = SetScissorRectangles(scissorRectangle);
	if (Result::SUCCESS != result)
	{
		return result;
	}

	return result;
}

Result D3DBaseClient::Draw()
{
	Result result = Result::SUCCESS;

	// Reset command list memory allocator.
	// This step ssumes that the associated commands have completed.
	if (S_OK != m_comptrCommandAllocator->Reset())
	{
		return Result::FAIL;
	}

	// Reset command list.
	// This step can be taken after the command list has been added to the 
	// command queue via ExecuteCommandList.
	// Reusing the same command list reuses memory.
	if (S_OK != m_comptrGraphicsCommandList->Reset(m_comptrCommandAllocator.Get(), nullptr))
	{
		return Result::FAIL;
	}

	// Resource transition barrier.
	auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_comptrGraphicsCommandList->ResourceBarrier(1, &transition);

	// Set the viewport and scissor rectangle.
	// This needs to be reset whenever the command list is reset.
	m_comptrGraphicsCommandList->RSSetViewports(1, &m_screenViewport);
	m_comptrGraphicsCommandList->RSSetScissorRects(1, &m_scissorRectangle);

	// Clear back buffer and depth buffer.
	m_comptrGraphicsCommandList->ClearRenderTargetView(
		GetCurrentRtv(), DirectX::Colors::Red, 0, nullptr);
	m_comptrGraphicsCommandList->ClearDepthStencilView(
		GetDsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	// Specify the back buffer to write into.
	auto dsv = GetDsv();
	auto rtv = GetCurrentRtv();
	m_comptrGraphicsCommandList->OMSetRenderTargets(1, &rtv, true, &dsv);

	// Resource transition barrier.
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_comptrGraphicsCommandList->ResourceBarrier(1, &transition);

	// Done recording commands.
	if (S_OK != m_comptrGraphicsCommandList->Close())
	{
		return Result::FAIL;
	}

	// Add the command list to the queue for execution.
	ID3D12CommandList* commandList[] = {m_comptrGraphicsCommandList.Get()};
	m_comptrCommandQueue->ExecuteCommandLists(_countof(commandList), commandList);

	if (S_OK != m_resourceStuff.m_comptrSwapChain->Present(0, 0))
	{
		return Result::FAIL;
	}

	AdvanceBackBuffer();

	// Wait for commands to complete.
	// Note that this is inefficient.
	result = FlushCommandQueue();

	return result;
}