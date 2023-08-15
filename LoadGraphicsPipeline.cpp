#include "stdafx.h"
#include "Thesis.h"

void Thesis::LoadGraphicsPipeline(ComPtr<IDXGIFactory4> factory)
{
	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc{ };
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_graphicsCommandQueue)));

	CreateSwapChain(factory);

	// Create descriptor heap for render target view.
	CreateRTVDescriptorHeap();

	// Create frame resources.
	CreateFrameResources();

	// Create graphics command allocator.
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&m_graphicsCommandAllocator)));

	// Create a root signature for the graphics pipeline.
	CreateGraphicsPipelineRootSignature();

	// Create the pipeline state, which includes compiling and loading shaders.
	CreateGraphicsPipelineState();

	// Create the graphics command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsCommandAllocator.Get(),
		m_graphicsPipelineState.Get(), IID_PPV_ARGS(&m_graphicsCommandList)));

}

void Thesis::CreateSwapChain(Microsoft::WRL::ComPtr<IDXGIFactory4>& factory)
{
	ComPtr<IDXGISwapChain1> swapChain;

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;


	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_graphicsCommandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Thesis::CreateRTVDescriptorHeap()
{
	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));

	m_rtvDescriptorHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Thesis::CreateFrameResources() {
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (UINT n = 0; n < FrameCount; n++)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorHeapSize);
	}
}

void Thesis::CreateGraphicsPipelineState() {
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0",
		compileFlags, 0, &vertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0",
		compileFlags, 0, &pixelShader, nullptr));

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_graphicsPipelineRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	// #DXR Extra: Depth buffering
	// Add support for depth testing, using a 32-bit floating-point depth buffer.
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_graphicsPipelineState)));
}

void Thesis::CreateGraphicsPipelineRootSignature()
{
	// #DXR Extra: Perspective camera.
	// The root signature describes which data is accessed by the shader. The camera matrices are held in a constant 
	// buffer, itself referenced (on?) the heap. To do this we reference a range in the heap, and use that range as
	// the sole parameter of the shader. The camera buffer is associated in the index 0, making it accessible in 
	// the shader in the b0 register.
	CD3DX12_ROOT_PARAMETER constantParameter;
	CD3DX12_DESCRIPTOR_RANGE range;
	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	constantParameter.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

	// #DXR Extra: Refitting.
	// Per-instance properties buffer.
	CD3DX12_ROOT_PARAMETER matricesParameter;
	CD3DX12_DESCRIPTOR_RANGE matricesRange;
	matricesRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 /*desc count*/, 0 /*register*/, 0 /*space*/,
		1 /*heap slot*/);
	matricesParameter.InitAsDescriptorTable(1, &matricesRange, D3D12_SHADER_VISIBILITY_ALL);

	//Per instance properties index for the current geometry.
	CD3DX12_ROOT_PARAMETER indexParameter;
	indexParameter.InitAsConstants(1 /*value count*/, 1 /*register*/);

	std::vector<CD3DX12_ROOT_PARAMETER> params = { constantParameter, matricesParameter, indexParameter };
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(static_cast<UINT>(params.size()), params.data(), 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&m_graphicsPipelineRootSignature)));
}