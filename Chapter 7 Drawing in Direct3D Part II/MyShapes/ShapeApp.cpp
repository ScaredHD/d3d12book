#include "ShapeApp.h"

#include "Common/GeometryGenerator.h"

using namespace DirectX;

void ShapeApp::OnInitialize() {
    BuildRenderItems();
    BuildFrameResources();
    BuildGeometry();

    CreateCbvDescriptorHeap();
    CreateCbufferViews();
}

void ShapeApp::OnUpdate() {
    currentFrameResourceIndex_ = (currentFrameResourceIndex_ + 1) % s_frameResourceCount;
    currentFrameResource_ = frameResources_[currentFrameResourceIndex_].get();

    if (GetFenceValue() < currentFrameResource_->fence) {
        // GPU fence < frame resource fence. Meaning this frame resource is already updated by CPU
        // and not yet processed by GPU. This indicates that the frame resource array is full and
        // CPU should wait for GPU.
        auto event = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(GetFence()->SetEventOnCompletion(currentFrameResource_->fence, event));
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }

    UpdateMainPassCBuffer();
    UpdateObjectCBuffers();
}

void ShapeApp::Draw() {
    auto allocator = currentFrameResource_->commandAllocator;
    ID3D12GraphicsCommandList* cmdList = GetCommandList();

    ThrowIfFailed(allocator.Reset());

    if (wireFrameMode_) {
        ThrowIfFailed(GetCommandList()->Reset(allocator.Get(),
                                              pipelineStateObjects_["opaque_wireframe"].Get()));
    } else {
        ThrowIfFailed(
            GetCommandList()->Reset(allocator.Get(), pipelineStateObjects_["opaque"].Get()));
    }

    SetViewportAndScissorRects();

    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &transition);

    cmdList->ClearRenderTargetView(GetCurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    cmdList->ClearDepthStencilView(GetDepthStencilView(),
                                   D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                   1.0f,
                                   0,
                                   0,
                                   nullptr);
    auto rtv = GetCurrentBackBufferView();
    auto dsv = GetDepthStencilView();
    cmdList->OMSetRenderTargets(1, &rtv, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap_.Get()};
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    int passCbvIdx = passCbvStartIndex_ + currentFrameResourceIndex_;
    auto passHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap_->GetGPUDescriptorHandleForHeapStart());
    passHandle.Offset(passCbvIdx, GetCbvSrvDescriptorSize());
    cmdList->SetGraphicsRootDescriptorTable(1, passHandle);

    DrawRenderItems();

    transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      D3D12_RESOURCE_STATE_PRESENT);
    cmdList->ResourceBarrier(1, &transition);

    ThrowIfFailed(cmdList->Close());
    ExecuteCommandList();

    ThrowIfFailed(GetSwapChain()->Present(0, 0));
    SwapBuffers();

    // Mark fence for this frame's commands and record this fence in frame resource of this
    // frame. If GPU fence >= resource fence, then GPU has finished using this resource.
    currentFrameResource_->fence = ++nextFence;
    GetCommandQueue()->Signal(GetFence(), nextFence);
}

void ShapeApp::BuildFrameResources() {
    for (int i = 0; i < s_frameResourceCount; ++i) {
        frameResources_.push_back(
            std::make_unique<FrameResource>(GetDevice(), 1, renderItems_.size()));
    }
}

void ShapeApp::BuildGeometry() {
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3.0f);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = static_cast<UINT>(box.Vertices.size());
    UINT sphereVertexOffset = gridVertexOffset + static_cast<UINT>(grid.Vertices.size());
    UINT cylinderVertexOffset = sphereVertexOffset + static_cast<UINT>(sphere.Vertices.size());

    UINT boxIndexOffset = 0;
    UINT gridIndexOffset = static_cast<UINT>(box.Indices32.size());
    UINT sphereIndexOffset = gridIndexOffset + static_cast<UINT>(grid.Indices32.size());
    UINT cylinderIndexOffset = sphereIndexOffset + static_cast<UINT>(sphere.Indices32.size());

    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = static_cast<UINT>(box.Indices32.size());
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    SubmeshGeometry gridSubmesh;
    gridSubmesh.IndexCount = static_cast<UINT>(grid.Indices32.size());
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;

    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = static_cast<UINT>(sphere.Indices32.size());
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

    SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.IndexCount = static_cast<UINT>(cylinder.Indices32.size());
    cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

    auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() +
                            cylinder.Vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
        vertices[k].pos = box.Vertices[i].Position;
        vertices[k].color = XMFLOAT4(DirectX::Colors::DarkGreen);
    }

    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k) {
        vertices[k].pos = grid.Vertices[i].Position;
        vertices[k].color = XMFLOAT4(DirectX::Colors::ForestGreen);
    }

    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].pos = sphere.Vertices[i].Position;
        vertices[k].color = XMFLOAT4(DirectX::Colors::Crimson);
    }

    for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k) {
        vertices[k].pos = cylinder.Vertices[i].Position;
        vertices[k].color = XMFLOAT4(DirectX::Colors::SteelBlue);
    }

    std::vector<std::uint16_t> indices;
    indices.insert(std::end(indices), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(std::end(indices),
                   std::begin(grid.GetIndices16()),
                   std::end(grid.GetIndices16()));
    indices.insert(std::end(indices),
                   std::begin(sphere.GetIndices16()),
                   std::end(sphere.GetIndices16()));
    indices.insert(std::end(indices),
                   std::begin(cylinder.GetIndices16()),
                   std::end(cylinder.GetIndices16()));

    const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(GetDevice(),
                                                        GetCommandList(),
                                                        vertices.data(),
                                                        vbByteSize,
                                                        geo->VertexBufferUploader);
    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(GetDevice(),
                                                       GetCommandList(),
                                                       indices.data(),
                                                       ibByteSize,
                                                       geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs["box"] = boxSubmesh;
    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["sphere"] = sphereSubmesh;
    geo->DrawArgs["cylinder"] = cylinderSubmesh;

    geometries_[geo->Name] = std::move(geo);
}

void ShapeApp::BuildRenderItems() {
    auto box = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box->modelToWorld,
                    XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    box->CBufferElementIndex = 0;
    box->geo = geometries_["shapeGeo"].get();
    box->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box->indexCount = box->geo->DrawArgs["box"].IndexCount;
    box->indexStart = box->geo->DrawArgs["box"].StartIndexLocation;
    box->baseVertex = box->geo->DrawArgs["box"].BaseVertexLocation;
    renderItems_.push_back(std::move(box));

    // Build columns and spheres in rows
    UINT objCBIndex = 2;
    for (int i = 0; i < 5; ++i) {
        auto leftCylinder = std::make_unique<RenderItem>();
        auto rightCylinder = std::make_unique<RenderItem>();
        auto leftSphere = std::make_unique<RenderItem>();
        auto rightSphere = std::make_unique<RenderItem>();

        XMMATRIX leftCylinderWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 0.5f);
        XMMATRIX rightCylinderWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 0.5f);

        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

        XMStoreFloat4x4(&leftCylinder->modelToWorld, rightCylinderWorld);
        leftCylinder->CBufferElementIndex = objCBIndex++;
        leftCylinder->geo = geometries_["shapeGeo"].get();
        leftCylinder->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylinder->indexCount = leftCylinder->geo->DrawArgs["cylinder"].IndexCount;
        leftCylinder->indexStart = leftCylinder->geo->DrawArgs["cylinder"].StartIndexLocation;
        leftCylinder->baseVertex = leftCylinder->geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&rightCylinder->modelToWorld, leftCylinderWorld);
        rightCylinder->CBufferElementIndex = objCBIndex++;
        rightCylinder->geo = geometries_["shapeGeo"].get();
        rightCylinder->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylinder->indexCount = rightCylinder->geo->DrawArgs["cylinder"].IndexCount;
        rightCylinder->indexStart = rightCylinder->geo->DrawArgs["cylinder"].StartIndexLocation;
        rightCylinder->baseVertex = rightCylinder->geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&leftSphere->modelToWorld, leftSphereWorld);
        leftSphere->CBufferElementIndex = objCBIndex++;
        leftSphere->geo = geometries_["shapeGeo"].get();
        leftSphere->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphere->indexCount = leftSphere->geo->DrawArgs["sphere"].IndexCount;
        leftSphere->indexStart = leftSphere->geo->DrawArgs["sphere"].StartIndexLocation;
        leftSphere->baseVertex = leftSphere->geo->DrawArgs["sphere"].BaseVertexLocation;

        XMStoreFloat4x4(&rightSphere->modelToWorld, rightSphereWorld);
        rightSphere->CBufferElementIndex = objCBIndex++;
        rightSphere->geo = geometries_["shapeGeo"].get();
        rightSphere->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphere->indexCount = rightSphere->geo->DrawArgs["sphere"].IndexCount;
        rightSphere->indexStart = rightSphere->geo->DrawArgs["sphere"].StartIndexLocation;
        rightSphere->baseVertex = rightSphere->geo->DrawArgs["sphere"].BaseVertexLocation;

        renderItems_.push_back(std::move(leftCylinder));
        renderItems_.push_back(std::move(rightCylinder));
        renderItems_.push_back(std::move(leftSphere));
        renderItems_.push_back(std::move(rightSphere));
    }

    for (auto& e : renderItems_) {
        opaqueRenderItems_.push_back(e.get());
    }
}

void ShapeApp::CreateCbvDescriptorHeap() {
    auto objCount = static_cast<UINT>(opaqueRenderItems_.size());

    auto descriptorCount = (objCount + 1) * FrameResourceCount();

    passCbvStartIndex_ = objCount * FrameResourceCount();

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = descriptorCount;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(cbvHeap_.GetAddressOf())));
}

void ShapeApp::CreateCbufferViews() {
    // Create CBV for each object constant buffer in each frame
    UINT objectCbufferElemByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstant));

    for (int frameIdx = 0; frameIdx < FrameResourceCount(); ++frameIdx) {
        auto objectCbuffer = frameResources_[frameIdx]->objectCBuffer->Resource();
        for (UINT objIdx = 0; objIdx < OpaqueRenderItemCount(); ++objIdx) {
            // Get object constant buffer location
            D3D12_GPU_VIRTUAL_ADDRESS cbAddressGpu = objectCbuffer->GetGPUVirtualAddress();
            cbAddressGpu += objectCbufferElemByteSize * objIdx;

            // Get corresponding CBV location (handle) in CBV heap
            int cbvIdx = frameIdx * OpaqueRenderItemCount() + objIdx;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
                cbvHeap_->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(cbvIdx, GetCbvSrvDescriptorSize());

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
            cbvDesc.BufferLocation = cbAddressGpu;
            cbvDesc.SizeInBytes = objectCbufferElemByteSize;
            GetDevice()->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    // Create CBV for pass constant buffer for each frame (last three CBVs)
    UINT passCbufferElemByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstant));

    for (int frameIdx = 0; frameIdx < FrameResourceCount(); ++frameIdx) {
        auto passCbuffer = frameResources_[frameIdx]->passCBuffer->Resource();

        // Get pass constant buffer location
        D3D12_GPU_VIRTUAL_ADDRESS cbAddressGpu = passCbuffer->GetGPUVirtualAddress();

        // Get corresponding CBV location (handle) in CBV heap
        int cbvIdx = passCbvStartIndex_ + frameIdx;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap_->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(cbvIdx, GetCbvSrvDescriptorSize());

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.BufferLocation = cbAddressGpu;
        cbvDesc.SizeInBytes = passCbufferElemByteSize;
        GetDevice()->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void ShapeApp::UpdateObjectCBuffers() {
    auto* currentObjectCBuffer = currentFrameResource_->objectCBuffer.get();
    for (const auto& e : renderItems_) {
        // TODO: dirtyFrameResourceCount ?
        if (e->dirtyFrameResourceCount > 0) {
            XMMATRIX world = XMLoadFloat4x4(&e->modelToWorld);
            ObjectConstant objConstants;
            XMStoreFloat4x4(&objConstants.modelToWorld, XMMatrixTranspose(world));
            currentObjectCBuffer->CopyData(e->CBufferElementIndex, objConstants);
            --e->dirtyFrameResourceCount;
        }
    }
}

void ShapeApp::UpdateMainPassCBuffer() {
    XMMATRIX view = XMLoadFloat4x4(&view_);
    XMMATRIX proj = XMLoadFloat4x4(&proj_);
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);

    XMVECTOR detView = XMMatrixDeterminant(view);
    XMVECTOR detProj = XMMatrixDeterminant(proj);
    XMVECTOR detViewProj = XMMatrixDeterminant(viewProj);
    XMMATRIX invView = XMMatrixInverse(&detView, view);
    XMMATRIX invProj = XMMatrixInverse(&detProj, proj);
    XMMATRIX invViewProj = XMMatrixInverse(&detViewProj, viewProj);

    XMMATRIX viewT = XMMatrixTranspose(view);
    XMMATRIX projT = XMMatrixTranspose(proj);
    XMMATRIX viewProjT = XMMatrixTranspose(viewProj);
    XMMATRIX invViewT = XMMatrixTranspose(invView);
    XMMATRIX invProjT = XMMatrixTranspose(invProj);
    XMMATRIX invViewProjT = XMMatrixTranspose(invViewProj);

    XMStoreFloat4x4(&mainPassCBuffer_.view, viewT);
    XMStoreFloat4x4(&mainPassCBuffer_.proj, projT);
    XMStoreFloat4x4(&mainPassCBuffer_.viewProj, viewProjT);
    XMStoreFloat4x4(&mainPassCBuffer_.invView, invViewT);
    XMStoreFloat4x4(&mainPassCBuffer_.invProj, invProjT);
    XMStoreFloat4x4(&mainPassCBuffer_.invViewProj, invViewProjT);

    mainPassCBuffer_.eyePos = eyePos_;
    mainPassCBuffer_.renderTargetSize = XMFLOAT2(static_cast<float>(GetClientWidth()),
                                                 static_cast<float>(GetClientHeight()));
    mainPassCBuffer_.invRenderTargetSize = XMFLOAT2(1.0f / GetClientWidth(),
                                                    1.0f / GetClientHeight());

    mainPassCBuffer_.nearZ = 1.0f;
    mainPassCBuffer_.farZ = 100.0f;
    mainPassCBuffer_.totalTime = timer.TotalTimeFromStart();
    mainPassCBuffer_.deltaTime = timer.DeltaTimeSecond();

    currentFrameResource_->passCBuffer.get()->CopyData(0, mainPassCBuffer_);
}

void ShapeApp::DrawRenderItems() {
    ID3D12GraphicsCommandList* cmdList = GetCommandList();

    for (auto& item : renderItems_) {
        auto vbv = item->geo->VertexBufferView();
        cmdList->IASetVertexBuffers(0, 1, &vbv);

        auto ibv = item->geo->IndexBufferView();
        cmdList->IASetIndexBuffer(&ibv);

        cmdList->IASetPrimitiveTopology(item->primitiveType);

        UINT cbvIdx = currentFrameResourceIndex_ * static_cast<UINT>(OpaqueRenderItemCount()) +
                      item->CBufferElementIndex;
        auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap_->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(cbvIdx, GetCbvSrvDescriptorSize());

        cmdList->SetGraphicsRootDescriptorTable(0, handle);

        cmdList->DrawIndexedInstanced(item->indexCount, 1, item->indexStart, item->baseVertex, 0);
    }
}
