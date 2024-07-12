#include "MyBoxApp.h"

#include "Common/MathHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void MyBoxApp::OnInitialize() {
    MyD3DApp::OnInitialize();

    ThrowIfFailed(GetCommandList()->Reset(GetCommandAllocator(), nullptr));

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    // Execute the initialization commands
    ThrowIfFailed(GetCommandList()->Close());
    ExecuteCommandList();
    FlushCommandQueue();
}

void MyBoxApp::OnResize() {
    MyD3DApp::OnResize();

    XMMATRIX m = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&matProjection, m);
}

void MyBoxApp::OnUpdate() {
    MyD3DApp::OnUpdate();

    if (IsMouseDown()) {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(GetMouseX() - lastMousePosX));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(GetMouseY() - lastMousePosY));
        // Update angles based on input to orbit camera around box.
        // d3dbook has += instead. Feels unnatural
        cameraPose.theta -= dx;
        cameraPose.phi -= dy;
        // Restrict the angle mPhi.
        cameraPose.phi = MathHelper::Clamp(cameraPose.phi, 0.1f, MathHelper::Pi - 0.1f);

        // std::string s = std::to_string(IsMouseDown()) + "\t" + std::to_string(dx);
        // s += "\t" + std::to_string(dy) + "\n";
        // ::OutputDebugStringA(s.c_str());

        lastMousePosX = GetMouseX();
        lastMousePosY = GetMouseY();
    }

    float x = cameraPose.radius * sinf(cameraPose.phi) * cosf(cameraPose.theta);
    float y = cameraPose.radius * cosf(cameraPose.phi);
    float z = cameraPose.radius * sinf(cameraPose.phi) * sinf(cameraPose.theta);

    XMMATRIX model = XMLoadFloat4x4(&matModel);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

    XMMATRIX proj = XMLoadFloat4x4(&matProjection);

    XMMATRIX modelViewProj = model * view * proj;

    ConstantBufferObject obj;
    XMStoreFloat4x4(&obj.modelViewProj, XMMatrixTranspose(modelViewProj));
    uploader->CopyData(0, obj);
}

void MyBoxApp::OnMouseDown(int xPos, int yPos) {
    lastMousePosX = GetMouseX();
    lastMousePosY = GetMouseY();
    SetCapture(GetWindowHandle());
}

void MyBoxApp::OnMouseUp(int xPos, int yPos) {
    ReleaseCapture();
}

void MyBoxApp::Draw() {
    ThrowIfFailed(GetCommandAllocator()->Reset());

    ThrowIfFailed(GetCommandList()->Reset(GetCommandAllocator(), pso.Get()));

    SetViewportAndScissorRects();

    // Transition of back buffer to render target state
    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET);
    GetCommandList()->ResourceBarrier(1, &transition);

    auto rtv = GetCurrentBackBufferView();
    GetCommandList()->ClearRenderTargetView(rtv, Colors::LightSteelBlue, 0, nullptr);

    auto dsv = GetDepthStencilView();
    GetCommandList()->ClearDepthStencilView(dsv,
                                            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                            1.0f,
                                            0,
                                            0,
                                            nullptr);

    GetCommandList()->OMSetRenderTargets(1, &rtv, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap.Get()};
    GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    GetCommandList()->SetGraphicsRootSignature(rootSignitature.Get());

    auto vbv = boxGeo->VertexBufferView();
    GetCommandList()->IASetVertexBuffers(0, 1, &vbv);

    auto ibv = boxGeo->IndexBufferView();
    GetCommandList()->IASetIndexBuffer(&ibv);

    GetCommandList()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Bind actual cbvHeap (argument) to root signature (parameter)
    GetCommandList()->SetGraphicsRootDescriptorTable(0,
                                                     cbvHeap->GetGPUDescriptorHandleForHeapStart());

    GetCommandList()->DrawIndexedInstanced(boxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

    transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      D3D12_RESOURCE_STATE_PRESENT);
    GetCommandList()->ResourceBarrier(1, &transition);

    ThrowIfFailed(GetCommandList()->Close());

    ExecuteCommandList();

    // Swap back and front buffers
    ThrowIfFailed(GetSwapChain()->Present(0, 0));
    SwapBuffers();

    FlushCommandQueue();
}

LRESULT MyBoxApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_LBUTTONDOWN) {
        MyBoxApp::OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    if (msg == WM_LBUTTONUP) {
        MyBoxApp::OnMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    return MyD3DApp::HandleMessage(hwnd, msg, wParam, lParam);
}

void MyBoxApp::BuildDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        GetDevice()->CreateDescriptorHeap(&cbvHeapDesc,
                                          IID_PPV_ARGS(cbvHeap.ReleaseAndGetAddressOf())));
}

void MyBoxApp::BuildConstantBuffers() {
    uploader = std::make_unique<UploadBuffer<ConstantBufferObject>>(GetDevice(), 1, true);

    UINT bytesize = d3dUtil::CalcConstantBufferByteSize(sizeof(ConstantBufferObject));

    D3D12_GPU_VIRTUAL_ADDRESS uploadBufferAddressGPU = uploader->Resource()->GetGPUVirtualAddress();
    int boxConstantBufferIndex = 0;
    uploadBufferAddressGPU += boxConstantBufferIndex * bytesize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = uploadBufferAddressGPU;
    cbvDesc.SizeInBytes = bytesize;

    GetDevice()->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void MyBoxApp::BuildRootSignature() {
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // Create a single descriptor table of CBVs
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  // table type
                  1,                                // number of descriptors in the table
                  0  // base shader register arguments are bound to for this root parameter
    );
    slotRootParameter[0].InitAsDescriptorTable(1,         // number of ranges
                                               &cbvTable  // pointer to array of ranges
    );

    // a root signature is an array of root parameters
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        1,
        slotRootParameter,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a
    // single constant buffer
    ComPtr<ID3DBlob> serializedRootSig{};
    ComPtr<ID3DBlob> errorBlob{};
    auto result = D3D12SerializeRootSignature(&rootSigDesc,
                                              D3D_ROOT_SIGNATURE_VERSION_1,
                                              serializedRootSig.GetAddressOf(),
                                              errorBlob.GetAddressOf());

    if (errorBlob) {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(result);

    ThrowIfFailed(
        GetDevice()->CreateRootSignature(0,
                                         serializedRootSig->GetBufferPointer(),
                                         serializedRootSig->GetBufferSize(),
                                         IID_PPV_ARGS(rootSignitature.ReleaseAndGetAddressOf())));
}

void MyBoxApp::BuildShadersAndInputLayout() {
    vertexShaderByteCode = d3dUtil::CompileShader(L"color.hlsl", nullptr, "VS", "vs_5_0");
    pixelShaderByteCode = d3dUtil::CompileShader(L"color.hlsl", nullptr, "PS", "ps_5_0");

    inputLayout = {{"POSITION",
                    0,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0,
                    0,
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0},
                   {"COLOR",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    0,
                    12,
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0}};
}

void MyBoxApp::BuildBoxGeometry() {
    std::array<Vertex, 8> vertices = {
        Vertex{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)},
        Vertex{XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)},
        Vertex{XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)},
        Vertex{XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)},
        Vertex{XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)},
        Vertex{XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)},
        Vertex{XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)},
        Vertex{XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)}};

    // clang-format off
    std::array<std::uint16_t, 36> indices = {
        // front face
        0, 1, 2,
        0, 2, 3,
        // back face
        4, 6, 5,
        4, 7, 6,
        // left face
        4, 5, 1,
        4, 1, 0,
        // right face
        3, 2, 6,
        3, 6, 7,
        // top face
        1, 5, 6,
        1, 6, 2,
        // bottom face
        4, 0, 3,
        4, 3, 7
    };
    // clang-format on

    const UINT vbByteSize = vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

    boxGeo = std::make_unique<MeshGeometry>();
    boxGeo->Name = "boxGeo";

    // Create VB and IB on CPU
    ThrowIfFailed(D3DCreateBlob(vbByteSize, boxGeo->VertexBufferCPU.ReleaseAndGetAddressOf()));
    CopyMemory(boxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, boxGeo->IndexBufferCPU.ReleaseAndGetAddressOf()));
    CopyMemory(boxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    // Create VB and IB on GPU
    boxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(GetDevice(),
                                                           GetCommandList(),
                                                           vertices.data(),
                                                           vbByteSize,
                                                           boxGeo->VertexBufferUploader);
    boxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(GetDevice(),
                                                          GetCommandList(),
                                                          indices.data(),
                                                          ibByteSize,
                                                          boxGeo->IndexBufferUploader);

    boxGeo->VertexByteStride = sizeof(Vertex);
    boxGeo->VertexBufferByteSize = vbByteSize;
    boxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    boxGeo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;
    boxGeo->DrawArgs["box"] = submesh;
}

void MyBoxApp::BuildPSO() {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = {inputLayout.data(), (UINT)inputLayout.size()};
    psoDesc.pRootSignature = rootSignitature.Get();
    psoDesc.VS = {reinterpret_cast<BYTE*>(vertexShaderByteCode->GetBufferPointer()),
                  vertexShaderByteCode->GetBufferSize()};
    psoDesc.PS = {reinterpret_cast<BYTE*>(pixelShaderByteCode->GetBufferPointer()),
                  pixelShaderByteCode->GetBufferSize()};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = GetBackBufferFormat();
    psoDesc.SampleDesc.Count = Msaa4xEnabled() ? 4 : 1;
    psoDesc.SampleDesc.Quality = Msaa4xEnabled() ? GetMsaa4xQuality() - 1 : 0;
    psoDesc.DSVFormat = GetDepthStencilFormat();

    ThrowIfFailed(
        GetDevice()->CreateGraphicsPipelineState(&psoDesc,
                                                 IID_PPV_ARGS(pso.ReleaseAndGetAddressOf())));
}