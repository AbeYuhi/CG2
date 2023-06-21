#include <Windows.h>
#pragma region STD関連
#include <string>
#include <format>
#include <cstdint>
#include <vector>
#include <list>
#pragma endregion
#pragma region DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
#include <dxcapi.h>
#pragma endregion
#pragma region ImGuiライブラリのを使用できるようにする
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#pragma endregion
#pragma region DirectTex関連
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#pragma endregion
#pragma region 自作フォルダ
#include "Vector3.h"
#include "Vector3_Math.hpp"
#include "Vector2.h"
#include "Matrix4x4.h"
#include "Camera.h"
#pragma endregion
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

#pragma region 構造体の宣言
struct Vector4 {
    float x;
    float y;
    float z;
    float w;
};

struct TransformStructure {
    Vector3 scale;
    Vector3 rotate;
    Vector3 translate;
};

struct VertexData {
    Vector4 position;
    Vector2 texcoode;
    Vector3 normal;
};

struct Material {
    Vector4 color;
    int32_t enableLighting;
};

struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
};

struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float intensity;
};

#pragma endregion

#pragma region 関数のプロトタイプ宣言
/// <summary>
/// ウィンドウプロシージャ
/// </summary>
/// <param name="hwnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/// <summary>
/// ロガー
/// </summary>
/// <param name="message">デバックログに出力したい変数など</param>
void Log(const std::string& message);

/// <summary>
/// 文字列をwstringに変換
/// </summary>
/// <param name="str">変換したい文字列</param>
/// <returns></returns>
std::wstring ConvertString(const std::string& str);

/// <summary>
/// 文字列をstringに変換
/// </summary>
/// <param name="str">変換したい文字列</param>
/// <returns></returns>
std::string ConvertString(const std::wstring& str);

IDxcBlob* CompileShader(
    //CompileするShaderファイルへのパス
    const std::wstring& filePath,
    //Compilerに使用するprofile
    const wchar_t* profile,
    //初期化で生成したものを3つ
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* incluedeHandler);

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

DirectX::ScratchImage LoadTexture(const std::string& filePath);

ID3D12Resource* CreateTextureResources(ID3D12Device* device, const DirectX::TexMetadata& metadata);

void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

void PushCommandList(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue, IDXGISwapChain4* swapChain, ID3D12Fence* fence, uint64_t& fenceValue, HANDLE fenceEvent);

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
#pragma endregion

#pragma region define
#define M_PI 3.14f
#pragma endregion

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    CoInitializeEx(0, COINIT_MULTITHREADED);

    WNDCLASS wc{};
    //ウィンドウプロシージャ
    wc.lpfnWndProc = WindowProc;
    //ウィンドウクラス名
    wc.lpszClassName = L"CG2WindowClass";
    //インスタンスハンドル
    wc.hInstance = GetModuleHandle(nullptr);
    //カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    //ウィンドウクラスを登録する
    RegisterClass(&wc);

    //クライアント領域のサイズ
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeigth = 720;

    //ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc = {0, 0, kClientWidth, kClientHeigth};

    //クライアント領域を元に実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    //ウィンドウの生成
    HWND hwnd = CreateWindow(
        wc.lpszClassName,       //利用するクラス名
        L"CG2",                 //タイトルバーの文字(何でも良い)
        WS_OVERLAPPEDWINDOW,    //よく見るウィンドウスタイル
        CW_USEDEFAULT,          //表示X座標(Windowsに任せる)
        CW_USEDEFAULT,          //表示Y座標(WindowsOSに任せる)
        wrc.right - wrc.left,   //ウィンドウ横幅
        wrc.bottom - wrc.top,   //ウィンドウ縦幅
        nullptr,                //親ウィンドウハンドル
        nullptr,                //メニューハンドル
        wc.hInstance,           //インスタンスハンドル
        nullptr);               //オプション

    //ウィンドウを表示する
    ShowWindow(hwnd, SW_SHOW); 

#ifdef _DEBUG
    ID3D12Debug1* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        //デバックレイヤーを有効化する
        debugController->EnableDebugLayer();
        //さらにGPU側でもチェックを行うようにする
        debugController->SetEnableGPUBasedValidation(TRUE);
    }
#endif // _DEBUG


    //DXGIファクトリーの生成
    IDXGIFactory7* dxgiFactory = nullptr;
    //HRESULTはウィンドウ系のエラーコードであり、
    //関数が成功したかどうかをSUCCEEDEDマクロで判定できる
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
    //初期化の根本的なエラーが出た場合はプログラムが間違っているかどうか、どうにも
    //できない場合が多いのでassertにしておく
    assert(SUCCEEDED(hr));

    //使用するアダプタ用の変数。最初にnullptrを入れておく
    IDXGIAdapter4* useAdapter = nullptr;
    //良いアダプタを順に頼む
    for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; i++) {
        //アダプターの情報を取得する
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));//取得できないのは一大事
        //ソフトウェアアダプタでなければ採用
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            //採用したアダプタの情報をログに出力。wstringの方なので注意
            Log(ConvertString(std::format(L"Use Adapter:{}", adapterDesc.Description)));
            break;
        }
        useAdapter = nullptr; //ソフトウェアアダプタの場合は見なかったことにする
    }
    //適切なアダプターが見つからなかったので起動できない
    assert(useAdapter != nullptr);

    ID3D12Device* device = nullptr;
    //機能レベルとログ出力用の文字列
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,  D3D_FEATURE_LEVEL_12_1,  D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] = {"12.2", "12.1", "12.0"};
    //高い順に生成できるか試していく
    for (size_t i = 0; i < _countof(featureLevels); i++) {
        //採用したアダプターでデバイスを生成
        hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
        //指定した機能レベルでデバイスが生成できたかを確認
        if (SUCCEEDED(hr)) {
            //生成できたのでログ出力を行ってループを抜ける
            Log(std::format("featureLevels : {}\n", featureLevelStrings[i]));
            break;
        }
    }
    //デバイスの生成がうまくいかなかったので起動できない
    assert(device != nullptr);
    Log("Complete create D3D12Device!!!\n");//初期化完了のログをだす

#ifdef _DEBUG
    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        //ヤバイエラー時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        //エラー時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        //警告時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        
        //抑制するメッセージのID
        D3D12_MESSAGE_ID denyIds[] = {
            //Windows11でのDXGIデバックレイヤーとDX12デバックレイヤーの相互作用バグによるエラーメッセージ
            //https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };
        //抑制するレベル
        D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;
        //指定したメッセージの表示を抑制する
        infoQueue->PushStorageFilter(&filter);
        //解放
        infoQueue->Release();
    }
#endif // _DEBUG

    //コマンドキューの生成
    ID3D12CommandQueue* commandQueue = nullptr;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
    //生成できなかった場合起動できない
    assert(SUCCEEDED(hr));

    //コマンドアロケーターを生成する
    ID3D12CommandAllocator* commandAllocator = nullptr;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    //コマンドアロケーターの生成がうまくいかなかった場合起動できない
    assert(SUCCEEDED(hr));

    //コマンドリストを生成する
    ID3D12GraphicsCommandList* commandList = nullptr;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
    //コマンドリストの生成がうまくいかなかった場合起動できない
    assert(SUCCEEDED(hr));

    //スワップチェーンを生成する
    IDXGISwapChain4* swapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = kClientWidth;                         //画面の幅。ウィンドウのクライアント領域と同じサイズにしておく
    swapChainDesc.Height = kClientHeigth;                       //画面の高さ。ウィンドウのクライアント領域と同じサイズにしておく
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;          //色の形式
    swapChainDesc.SampleDesc.Count = 1;                         //マルチサンプルしない
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//描画のターゲットとして利用する
    swapChainDesc.BufferCount = 2;                              //ダブルバッファ
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;        //モニタにうつしたら、中身を破棄

    //コマンドキュー、ウィンドウハンドル、設定を渡して生成する
    hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
    assert(SUCCEEDED(hr));

    //RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
    ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
    //SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
    ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

    //SwapChainからResourceを引っ張ってくる
    ID3D12Resource* swapChainResource[2] = { nullptr };
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResource[0]));
    //うまく取得できなかったら起動できない
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResource[1]));
    assert(SUCCEEDED(hr));

    //DescriptorSizeを取得しておく
    const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    //RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;       //出力結果をSRGBに変換して書き込む
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  //2dテクスチャとして書き込む
    //ディスクリプタの先頭を取得する
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    //RTVを2つ作るのでディスクリプタを2つ用意
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    //まず1つ目を作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(swapChainResource[0], &rtvDesc, rtvHandles[0]);
    //2つ目のディスクリプタハンドルを得る(自分で)
    rtvHandles[1].ptr = rtvHandles[0].ptr + descriptorSizeRTV;
    //2つ目を作る
    device->CreateRenderTargetView(swapChainResource[1], &rtvDesc, rtvHandles[1]);


    //初期値0でFenceを作る
    ID3D12Fence* fence = nullptr;
    uint64_t fenceValue = 0;
    hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(hr));

    //FenceのSignalを持つためのイベントを作成する
    HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent != nullptr);

    //DxcCompilerを初期化
    IDxcUtils* dxcUtils = nullptr;
    IDxcCompiler3* dxcCompiler = nullptr;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));

    //現時点ではincludeはしないが、includeに対応するための設定を行っておく
    IDxcIncludeHandler* includeHandler = nullptr;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    assert(SUCCEEDED(hr));

    //RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0; //0から始まる
    descriptorRange[0].NumDescriptors = 1; //数は1つ
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; //SRVを使う
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; //OFFsetを自動計算

    //RootParameter作成。複数設定できるので配列。今回は1つだけなので長さ1の配列
    D3D12_ROOT_PARAMETER rootParameters[4] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;         //CBVを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;      //PixelShaderで使う
    rootParameters[0].Descriptor.ShaderRegister = 0;                         //レジスタ番号0とバインド
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;         //CBVを使う
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;     //VertexShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                         //レジスタ番号0とバインド
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; //DescriptorTableを使う
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; //PixelShaderで使う
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; //Tableの中身の配列を指定
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); //Tableで利用する数
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; 
    rootParameters[3].Descriptor.ShaderRegister = 1;
    descriptionRootSignature.pParameters = rootParameters;                   //ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters);       //配列の長さ

    //Samplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; //バイオリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //0～1の範囲外をリピート
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; //比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; //ありったけのMipMapを使う
    staticSamplers[0].ShaderRegister = 0; //レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; //PixelShaderで使う
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    //シリアライズしてバイナリする
    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    //バイナリを元に生成
    ID3D12RootSignature* rootSignature = nullptr;
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));  

    //InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    //BlendStateの設定
    D3D12_BLEND_DESC blendDesc{};
    //すべての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    //RasiterzerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    //裏面(時計回り)を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    //三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    //DepthStencilTextureをウィンドウのサイズで作成
    ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeigth);
    //DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
    ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
    //DSVの設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //Format。基本的にはResourceに合わせる
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; //2dTexture
    //DSVHeapの先頭にDSVをつくる
    device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    //DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    //Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    //書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    //比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    //Shaderをコンパイルする
    IDxcBlob* vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vertexShaderBlob != nullptr);

    IDxcBlob* pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(pixelShaderBlob != nullptr);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature;                                                   //RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                    //InputLayout
    graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() }; //VertexShader
    graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };   //PixelShader
    graphicsPipelineStateDesc.BlendState = blendDesc;                                                           //BlendState
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;                                                 //RasterizerState
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    //書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    //利用するトポロジ(形状)のタイプ。三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    //どのように画面に色を打ち込むかの設定(気にしなくて良い)
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    //実際に生成
    ID3D12PipelineState* graphicsPipelineState = nullptr;
    hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));

#pragma region 三角形
    int vertexNumber = 16 * 16 * 6;
    ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(VertexData) * vertexNumber);

    //頂点バッファビューを作成する
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    //リソースの先頭のアドレスから使う
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    //使用するリソースのサイズは頂点3つ分のサイズ
    vertexBufferView.SizeInBytes = sizeof(VertexData) * vertexNumber;
    //1頂点当たりのサイズ
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    //頂点リソースにデータを書き込む
    VertexData* vertexData = nullptr;
    //書き込むためのアドレスを取得
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    const uint32_t kSubdivision = 16;
    const float kLonEvery = (2 * M_PI) / kSubdivision;
    const float kLatEvery = (M_PI) / kSubdivision;
    //緯度の方向に分割 -π/1 ～ π/2
    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -M_PI / 2.0f + kLatEvery * latIndex; //現在の緯度
        //経度の方向に分割 0～2π
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
            float lon = lonIndex * kLonEvery;

            //1枚目
            vertexData[start].position = {std::cos(lat) * std::cos(lon),
                std::sin(lat),
                std::cos(lat) * std::sin(lon), 1.0f};
            vertexData[start].texcoode = {float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision)};
            vertexData[start].normal = { vertexData[start].position.x, vertexData[start].position.y, vertexData[start].position.z };
            vertexData[start].normal = Normalize(vertexData[start].normal);

            vertexData[start + 1].position = { std::cos(lat + kLatEvery) * std::cos(lon),
                std::sin(lat + kLatEvery),
                std::cos(lat + kLatEvery) * std::sin(lon), 1.0f};
            vertexData[start + 1].texcoode = { float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex + 1) / float(kSubdivision) };
            vertexData[start + 1].normal = { vertexData[start + 1].position.x, vertexData[start + 1].position.y, vertexData[start + 1].position.z };
            vertexData[start + 1].normal = Normalize(vertexData[start + 1].normal);

            vertexData[start + 2].position = { std::cos(lat) * std::cos(lon + kLonEvery),
                std::sin(lat),
                std::cos(lat) * std::sin(lon + kLonEvery), 1.0f};
            vertexData[start + 2].texcoode = { float(lonIndex + 1) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision) };
            vertexData[start + 2].normal = { vertexData[start + 2].position.x, vertexData[start + 2].position.y, vertexData[start + 2].position.z };
            vertexData[start + 2].normal = Normalize(vertexData[start + 2].normal);

            //2枚目
            vertexData[start + 3].position = { std::cos(lat) * std::cos(lon + kLonEvery),
                std::sin(lat),
                std::cos(lat) * std::sin(lon + kLonEvery), 1.0f };
            vertexData[start + 3].texcoode = { float(lonIndex + 1) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision) };
            vertexData[start + 3].normal = { vertexData[start + 3].position.x, vertexData[start + 3].position.y, vertexData[start + 3].position.z };
            vertexData[start + 3].normal = Normalize(vertexData[start + 3].normal);

            vertexData[start + 4].position = { std::cos(lat + kLatEvery) * std::cos(lon),
                std::sin(lat + kLatEvery),
                std::cos(lat + kLatEvery) * std::sin(lon), 1.0f };
            vertexData[start + 4].texcoode = { float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex + 1) / float(kSubdivision) };
            vertexData[start + 4].normal = { vertexData[start + 4].position.x, vertexData[start + 4].position.y, vertexData[start + 4].position.z };
            vertexData[start + 4].normal = Normalize(vertexData[start + 4].normal);

            vertexData[start + 5].position = { std::cos(lat + kLatEvery) * std::cos(lon + kLonEvery),
                std::sin(lat + kLatEvery),
                std::cos(lat + kLatEvery) * std::sin(lon + kLonEvery), 1.0f};
            vertexData[start + 5].texcoode = { float(lonIndex + 1) / float(kSubdivision), 1.0f - float(latIndex + 1) / float(kSubdivision) };
            vertexData[start + 5].normal = { vertexData[start + 5].position.x, vertexData[start + 5].position.y, vertexData[start + 5].position.z };
            vertexData[start + 5].normal = Normalize(vertexData[start + 5].normal);
        }
    }

    //WVP用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
    ID3D12Resource* transformationMatrixResource = CreateBufferResource(device, sizeof(TransformationMatrix));
    //マテリアル用にデータを書き込む
    TransformationMatrix* transformationMatrixData = nullptr;
    //書き込むためのアドレスを取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
    //単位行列を書き込んでおく
    transformationMatrixData->WVP = MakeIdentity4x4();
    transformationMatrixData->World = MakeIdentity4x4();

#pragma endregion

#pragma region スプライト
    //Sprite用の頂点リソースを作る
    ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);

    //頂点バッファビューを作成する
    D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
    //リソースの先頭のアドレスから使う
    vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
    //使用するリソースのサイズは頂点3つ分のサイズ
    vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
    //1頂点当たりのサイズ
    vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

    //頂点リソースにデータを書き込む
    VertexData* vertexDataSprite = nullptr;
    //書き込むためのアドレスを取得
    vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
    //1枚目の三角形
    vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; //左下
    vertexDataSprite[0].texcoode = { 0.0f, 1.0f };
    vertexDataSprite[0].normal = { 0.0f, 0.0f, -1.0f };
    vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; //左上
    vertexDataSprite[1].texcoode = { 0.0f, 0.0f };
    vertexDataSprite[1].normal = { 0.0f, 0.0f, -1.0f };
    vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; //右下
    vertexDataSprite[2].texcoode = { 1.0f, 1.0f };
    vertexDataSprite[2].normal = { 0.0f, 0.0f, -1.0f };
    //2枚目の三角形
    vertexDataSprite[3].position = { 0.0f, 0.0f, 0.0f, 1.0f }; //左上
    vertexDataSprite[3].texcoode = { 0.0f, 0.0f };
    vertexDataSprite[3].normal = { 0.0f, 0.0f, -1.0f };
    vertexDataSprite[4].position = { 640.0f, 0.0f, 0.0f, 1.0f }; //右上
    vertexDataSprite[4].texcoode = { 1.0f, 0.0f };
    vertexDataSprite[4].normal = { 0.0f, 0.0f, -1.0f };
    vertexDataSprite[5].position = { 640.0f, 360.0f, 0.0f, 1.0f }; //右下
    vertexDataSprite[5].texcoode = { 1.0f, 1.0f };
    vertexDataSprite[5].normal = { 0.0f, 0.0f, -1.0f };

    //WVP用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
    ID3D12Resource* transformationMatrixResourceSprite = CreateBufferResource(device, sizeof(TransformationMatrix));
    //マテリアル用にデータを書き込む
    TransformationMatrix* transformationMatrixDataSprite = nullptr;
    //書き込むためのアドレスを取得
    transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
    //単位行列を書き込んでおく
    transformationMatrixDataSprite->WVP = MakeIdentity4x4();
    transformationMatrixDataSprite->World = MakeIdentity4x4();

#pragma endregion

    //マテリアル用のリソースを作る。
    ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(Material));
    //マテリアル用にデータを書き込む
    Material* materialData = nullptr;
    //書き込むためのアドレスを取得
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    //色の設定
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData->enableLighting = true;

    //スプライト用のマテリアル
    ID3D12Resource* materialResourceSprite = CreateBufferResource(device, sizeof(Material));
    //マテリアル用にデータを書き込む
    Material* materialDataSprite = nullptr;
    //書き込むためのアドレスを取得
    materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
    //色の設定
    materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialDataSprite->enableLighting = false;

    //照明
    ID3D12Resource* directionalLightResource = CreateBufferResource(device, sizeof(DirectionalLight));
    DirectionalLight* directionalLightData = nullptr;
    directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
    directionalLightData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    directionalLightData->direction = Vector3(0.0f, -1.0f, 0.0f);
    directionalLightData->intensity = 1.0f;

    //Transform変数を作る
    TransformStructure transform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    TransformStructure transformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

    //Textureを読んで転送する
    DirectX::ScratchImage mipImage = LoadTexture("Resource/Images/uvChecker.png");
    DirectX::ScratchImage mipImage2 = LoadTexture("Resource/Images/monsterBall.png");
    const DirectX::TexMetadata& metadata = mipImage.GetMetadata();
    const DirectX::TexMetadata& metadata2 = mipImage2.GetMetadata();
    ID3D12Resource* textureResource = CreateTextureResources(device, metadata);
    ID3D12Resource* textureResource2 = CreateTextureResources(device, metadata2);
    ID3D12Resource* intermediateResource = UploadTextureData(textureResource, mipImage, device, commandList);
    ID3D12Resource* intermediateResource2 = UploadTextureData(textureResource2, mipImage2, device, commandList);
    PushCommandList(commandList, commandAllocator, commandQueue, swapChain, fence, fenceValue, fenceEvent);
    //転送が終わったので、ここでReleseしてもよい
    intermediateResource->Release();
    intermediateResource2->Release();

    //SRV関係
    //metaDataを基にSRVの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
    //SRVを作成する
    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
    //SRVの生成
    device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);
    device->CreateShaderResourceView(textureResource2, &srvDesc, textureSrvHandleCPU2);

    //ビューポート
    D3D12_VIEWPORT viewport{};
    //クライアント領域のサイズと一緒にして画面全体に表示
    viewport.Width = kClientWidth;
    viewport.Height = kClientHeigth;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    //シザー短形
    D3D12_RECT scissorRect{};
    //基本的にビューポートと同じ短形が構成されるようにする
    scissorRect.left = 0;
    scissorRect.right = kClientWidth;
    scissorRect.top = 0;
    scissorRect.bottom = kClientHeigth;

    //ImGuiの初期化。詳細はさして重要ではない
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(device,
        swapChainDesc.BufferCount,
        rtvDesc.Format,
        srvDescriptorHeap,
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    Camera* camera = new Camera();
    camera->Initialize();

    bool useMonsterBall = true;
    bool isDrawSprite = true;

    MSG msg{};
    //ウィンドウの×ボタンが押されるまでループ
    while (msg.message != WM_QUIT) {
        //ウィンドウにメッセージが来てたら最優先で処理させる
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            //ImGui
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            //ゲームの処理
            //開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
            ImGui::ShowDemoWindow();

            transform.rotate.y += 0.03f;
            if (transform.rotate.y < -2 * M_PI || 2 * M_PI < transform.rotate.y) {
                transform.rotate.y = 0;
            }
            ImGui::Begin("Sphere");
            float* sphereScale[3] = { &transform.scale.x, &transform.scale.y, &transform.scale.z };
            float* sphereRotate[3] = { &transform.rotate.x, &transform.rotate.y, &transform.rotate.z };
            float* sphereTranslate[3] = { &transform.translate.x, &transform.translate.y, &transform.translate.z };
            ImGui::SliderFloat3("scale", *sphereScale, -10, 10);
            ImGui::SliderFloat3("rotate", *sphereRotate, -2 * M_PI, 2 * M_PI);
            ImGui::SliderFloat3("translate", *sphereTranslate, -100, 100);
            ImGui::End();

            camera->Update();
            
            //WVPMatrixに変換するだけで後の処理はDirectXが勝手にやってくれる
            Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
            Matrix4x4 viewMatrix = Inverse(camera->GetWorldTransform());
            Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeigth), 0.1f, 100.0f);
            Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

            transformationMatrixData->WVP = worldViewProjectionMatrix;
            transformationMatrixData->World = worldMatrix;

            //スプライト用のWVPMatrixを作る
            //WVPMatrixに変換するだけで後の処理はDirectXが勝手にやってくれる
            Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
            Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
            Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeigth), 0.0f, 100.0f);
            Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));

            transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
            transformationMatrixDataSprite->World = worldMatrixSprite;
            ImGui::Begin("sprite");
            float* pos[3] = { &transformSprite.translate.x, &transformSprite.translate.y, &transformSprite.translate.z };
            ImGui::SliderFloat3("pos", *pos, -500, 500);
            ImGui::End();

            ImGui::Begin("Texture");
            ImGui::Checkbox("useMonsterBall", &useMonsterBall);
            ImGui::Checkbox("isDrawSprite ", &isDrawSprite);
            ImGui::End();

            ImGui::Begin("Light");
            ImGui::SliderFloat3("direction", &directionalLightData->direction.x, -2 * M_PI, 2 * M_PI);
            directionalLightData->direction = Normalize(directionalLightData->direction);
            ImGui::End();

            //ここまで
            //ImGuiの内部コマンドを生成
            ImGui::Render();

            //これから書き込むバックバッファのインデックスを取得
            UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

            //TransitionBarrierの設定
            D3D12_RESOURCE_BARRIER barrier{};
            //今回のバリアはTransition
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            //Noneにしておく
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            //バリアを張る対象のリソース。現在のバックバッファに対して行う
            barrier.Transition.pResource = swapChainResource[backBufferIndex];
            //遷移前の(現在)のResourceState
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            //遷移後のResourceState
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            //TransitionBarrierを張る
            commandList->ResourceBarrier(1, &barrier);

            //描画先のRTVとDSV設定する
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
            //指定した深度で画面全体をクリアする
            commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            //指定した色で全体画面をクリアする
            float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f}; //青っぽい色、RGBAの順
            commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
            //描画用のDescriptorHeapの設定
            ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
            commandList->SetDescriptorHeaps(1, descriptorHeaps);

            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);
            //RootSignatureを設定。PSOに設定しているけど別途設定が必要
            commandList->SetGraphicsRootSignature(rootSignature);
            commandList->SetPipelineState(graphicsPipelineState);
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            //形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばよい
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            //マテリアルCBufferの場所を設定
            commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
            //影
            commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
            //wvp用のCBufferの場所を設定
            commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
            //SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である
            commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
            //描画!(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
            commandList->DrawInstanced(vertexNumber, 1, 0, 0);
            //スプライトの描画。変更が必要なものだけ変更する
            commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
            commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
            //TransformationMatrixCBufferの場所を設定
            commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
            //テクスチャの選択
            commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
            //描画
            if (isDrawSprite) {
                commandList->DrawInstanced(6, 1, 0, 0);
            }

            //実際のcommandListのImGuiの描画コマンドを積む
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

            //画面に描く処理はすべて終わり、画面に映すので、状態を遷移
            //今回はRenderTargetからPresentにする
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            //TransitionBarrierを張る
            commandList->ResourceBarrier(1, &barrier);

            //すべてのコマンドを積んでから実行すること
            PushCommandList(commandList, commandAllocator, commandQueue, swapChain, fence, fenceValue, fenceEvent);
        }
    }

#pragma region 解放処理
    //解放処理
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    textureResource->Release();
    materialResourceSprite->Release();
    materialResource->Release();
    transformationMatrixResourceSprite->Release();
    vertexResourceSprite->Release();
    transformationMatrixResource->Release();
    vertexResource->Release();
    depthStencilResource->Release();
    dsvDescriptorHeap->Release();
    graphicsPipelineState->Release();
    signatureBlob->Release();
    if (errorBlob) {
        errorBlob->Release();
    }
    rootSignature->Release();
    pixelShaderBlob->Release();
    vertexShaderBlob->Release();

    CloseHandle(fenceEvent);
    fence->Release();
    srvDescriptorHeap->Release();
    rtvDescriptorHeap->Release();
    swapChainResource[0]->Release();
    swapChainResource[1]->Release();
    swapChain->Release();
    commandList->Release();
    commandAllocator->Release();
    commandQueue->Release();
    device->Release();
    useAdapter->Release();
    dxgiFactory->Release();

#pragma endregion

#ifdef _DEBUG
    debugController->Release();
#endif // _DEBUG
    CloseWindow(hwnd);

    //リソースリークチェック
    IDXGIDebug1* debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
        debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        debug->Release();
    }

    CoUninitialize();

	return 0;
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;
    }
    //メッセージに応じてゲーム固有の処理を行う
    switch (msg)
    {
    case WM_DESTROY:
        //OSに対して、アプリの終了を伝える
        PostQuitMessage(0);
        return 0;
    }

    //標準のメッセージ処理を行う
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message) {
    OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0) {
        return std::wstring();
    }
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

std::string ConvertString(const std::wstring& str) {
    if (str.empty()) {
        return std::string();
    }

    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) {
        return std::string();
    }
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}

IDxcBlob* CompileShader(
    //CompileするShaderファイルへのパス
    const std::wstring& filePath,
    //Compilerに使用するprofile
    const wchar_t* profile,
    //初期化で生成したものを3つ
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* incluedeHandler) {

    //これからシェーダーをコンパイルする旨をログに出す
    Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile{}\n", filePath, profile)));
    //hlslファイルを読む
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    //読めなかったら止める
    assert(SUCCEEDED(hr));
    //読み込んだファイルの内容を設定する
    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8; //utf-8の文字コードであることを通知

    LPCWSTR arguments[] = {
        filePath.c_str(),           //コンパイル対象のhlslファイル名
        L"-E", L"main",             //エントリーポイントの指定。基本的にmain以外にはしない
        L"-T", profile,             //ShaderProfileの設定
        L"-Zi", L"-Qembed_debug",   //デバッグ用の情報を埋め込む
        L"-Od",                     //最適化を外しておく
        L"-Zpr"                     //メモリレイアウトは行優先
    };
    //実際にShaderをコンパイルする
    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler->Compile(
        &shaderSourceBuffer,            //読み込んだファイル
        arguments,                      //コンパイルオプション
        _countof(arguments),            //コンパイルオプションの数
        incluedeHandler,                //includeが含まれた諸々
        IID_PPV_ARGS(&shaderResult));   //コンパイル結果

    //コンパイルエラーではなくdxcが起動できないほど致命的な状況
    assert(SUCCEEDED(hr));

    //警告・エラーが出てたらログに出して止める
    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        Log(shaderError->GetStringPointer());
        //警告エラーダメ絶対
        assert(false);
    }

    //コンパイル結果から実行用のバイナリ部分を取得
    IDxcBlob* shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    //成功したログを出す
    Log(ConvertString(std::format(L"Compile Succeded, path{}, profile{}", filePath, profile)));
    //もう使わないリソースを解散
    shaderSource->Release();
    //実行用のバイナリを返却
    return shaderBlob;
}

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
    HRESULT hr = NULL;

    //頂点リソース用のヒープの設定
    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;//UploadHeapを使う
    //頂点リソースの設定
    D3D12_RESOURCE_DESC vertexResourceDesc{};
    //バッファリソース。テクスチャの場合はまた別の設定をする
    vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexResourceDesc.Width = sizeInBytes;//リソースのサイズ。引数の「sizeInBytes」を設定
    //バッファの場合はこれらは1にする決まり
    vertexResourceDesc.Height = 1;
    vertexResourceDesc.DepthOrArraySize = 1;
    vertexResourceDesc.MipLevels = 1;
    vertexResourceDesc.SampleDesc.Count = 1;
    //バッファの場合はこれにする決まり
    vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    //実際に頂点リソースを作る
    ID3D12Resource* vertexResource = nullptr;
    hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
    assert(SUCCEEDED(hr));

    return vertexResource;
}

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
    ID3D12DescriptorHeap* DescriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc{};
    DescriptorHeapDesc.Type = heapType;
    DescriptorHeapDesc.NumDescriptors = numDescriptors;
    DescriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = device->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&DescriptorHeap));
    //ディスクリプタヒープが作れなかったので起動できない
    assert(SUCCEEDED(hr));
    return DescriptorHeap;
}

DirectX::ScratchImage LoadTexture(const std::string& filePath) {
    //テクスチャファイルを読んでプログラムで扱えるようにする
    DirectX::ScratchImage image{};
    std::wstring filePathW = ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    assert(SUCCEEDED(hr));

    //ミニマップの作成
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
    assert(SUCCEEDED(hr));

    //ミニマップ付きのデータを返す
    return mipImages;
}

ID3D12Resource* CreateTextureResources(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
    //1.metadataを基にResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = UINT(metadata.width);
    resourceDesc.Height = UINT(metadata.height);
    resourceDesc.MipLevels = UINT(metadata.mipLevels);
    resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);
    resourceDesc.Format = metadata.format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    ////2.利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケース版がある
    //D3D12_HEAP_PROPERTIES heapProperties{};
    //heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;                           //細かい設定を行う
    //heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;    //WriteBackポリシーでCPUアクセス可能
    //heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;             //プロセッサの近くに配置
    
    //2.一般的なケース版
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
     
    //3.Resourceを生成
    ID3D12Resource* resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,                    //Heapの設定
        D3D12_HEAP_FLAG_NONE,               //Heapの特殊な設定。特になし
        &resourceDesc,                      //Resourceの設定
        //D3D12_RESOURCE_STATE_GENERIC_READ,  //初回のResourceState。Textureは基本読むだけ「特殊なケースの場合」
        D3D12_RESOURCE_STATE_COPY_DEST,     //データ転送される設定
        nullptr,                            //Clear最適値。使わないのでnullptr
        IID_PPV_ARGS(&resource));           //作成するResourceポインタへのポインタ
    assert(SUCCEEDED(hr));
    return resource;
}

//特殊なケースの場合
void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
    //Meta情報を取得
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    //全MipMapについて
    for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
        //MipMapLevelを指定して各Imageを取得
        const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
        //Textureに転送
        HRESULT hr = texture->WriteToSubresource(
            UINT(mipLevel),
            nullptr,                //全領域へコピー
            img->pixels,            //元データアドレス
            UINT(img->rowPitch),    //1ラインサイズ
            UINT(img->slicePitch)   //1枚サイズ
        );
        assert(SUCCEEDED(hr));
    }
}

//一般的なケース
[[nodiscard]]
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
    uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
    ID3D12Resource* intermediateResource = CreateBufferResource(device, intermediateSize);
    UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
    //Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    commandList->ResourceBarrier(1, &barrier);
    return intermediateResource;
}

//DepthStencilTextureの作成関数
ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
    //生成するResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width; //Textureの幅
    resourceDesc.Height = height; //Textureの高さ
    resourceDesc.MipLevels = 1; //mipmapの数
    resourceDesc.DepthOrArraySize = 1; //奥行き or 配列Texutureの配列数
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //DeothStencilとして利用可能なフォーマット
    resourceDesc.SampleDesc.Count = 1; //サンプリングカウント。1固定
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //2次元
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; //DepthStencilとして使う通知

    //利用するHeapの設定
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    //深層度のクリア設定
    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f; //1.0f(最大値)でクリア
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //フォーマット。Resourceと合わせる

    //Resourceの生成
    ID3D12Resource* resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties, //Heapの設定
        D3D12_HEAP_FLAG_NONE, //Heapの特殊な設定。特になし
        &resourceDesc, //Resourceの設定
        D3D12_RESOURCE_STATE_DEPTH_WRITE, //深層度を書き込む状態にしておく
        &depthClearValue, //Clear最適値
        IID_PPV_ARGS(&resource)); //作成するResourceポインタのポインタ
    assert(SUCCEEDED(hr));

    return resource;
}

//コマンドリストをクローズさせてから次のコマンドリストの準備まで
void PushCommandList(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue, IDXGISwapChain4* swapChain, ID3D12Fence* fence, uint64_t& fenceValue, HANDLE fenceEvent) {
    //コマンドリストをCLOSE;
    HRESULT hr = commandList->Close();
    assert(SUCCEEDED(hr));
    //GPUにコマンドリストの実行を行わせる
    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, commandLists);
    //GPUとOSに画面の交換を行うよう通知する
    swapChain->Present(1, 0);
    //Fenceの値を更新
    fenceValue++;
    //GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
    commandQueue->Signal(fence, fenceValue);
    //Fenceの値が指定したSignal値にたどり着いているか確認する
    //GetCompleteValueの初期値はFence作成時に渡した初期値
    if (fence->GetCompletedValue() < fenceValue) {
        //指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        //イベントを待つ
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    //次のフレーム用のコマンドリストを準備
    hr = commandAllocator->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList->Reset(commandAllocator, nullptr);
    assert(SUCCEEDED(hr));
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handleCPU.ptr += (descriptorSize * index);
    return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handleGPU.ptr += (descriptorSize * index);
    return handleGPU;
}