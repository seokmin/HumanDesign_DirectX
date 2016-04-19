#include <windows.h>
#include <time.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dCompiler.h>
#include <d3dx11.h>
#include <dxerr.h>

#define szWindowClass	TEXT("First")
#define szTitle			TEXT("First App")

HWND					g_hWnd = NULL;
IDXGISwapChain*			g_pSwapChain = nullptr;
ID3D11Device*			g_pd3dDevice = nullptr;
D3D_FEATURE_LEVEL		g_featureLevel = D3D_FEATURE_LEVEL_11_0;
//win32에서의 DC와 같다.
//DX에서는 렌더 대상 설정 및 자원을 연결하고 GPU에 렌더링 명령 수행
ID3D11DeviceContext*	g_pImmediateContext = nullptr;
ID3D11RenderTargetView*	g_pRenderTargetView = nullptr;


HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	//Flag 설정
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;//디버그 layer를 활성화하여 output창에 디버그 메시지를 출력한다
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UINT numFeatureLvels = ARRAYSIZE(featureLevels);



	//Swap Chain Setting
	DXGI_SWAP_CHAIN_DESC sd;//swap chain description
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1; //백버퍼 갯수. 1로 설정했으니 더블버퍼링

	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //백버퍼 포맷
	sd.BufferDesc.RefreshRate.Numerator = 60; //분자
	sd.BufferDesc.RefreshRate.Denominator = 1; //분모

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //백버퍼 렌더링
	sd.OutputWindow = g_hWnd; //출력 타겟
	sd.SampleDesc.Count = 1;//멀티샘플링.(MSAA. CheckMultisampleQualityLevels()를 통해 지원가능 품질 수준을 알아낸 후에 사용가능)
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(
		0,//기본 디스플레이 어댑터 사용
		D3D_DRIVER_TYPE_HARDWARE,//3d 하드웨어 가속
		0,//소프트웨어 구동 안함
		createDeviceFlags,
		featureLevels,
		numFeatureLvels,
		D3D11_SDK_VERSION,//sdk 버전
		&sd,//스왑체인 desc
		&g_pSwapChain,//생성된 스왑체인
		&g_pd3dDevice,//생성된 device
		&g_featureLevel,//사용된 featurelevel
		&g_pImmediateContext);//생성된 immediateContext
	if (FAILED(hr))
		return hr;

	
	//Create a render target view
	ID3D11Texture2D*	pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0,//후면 버퍼 인덱스. 여러개일때 중요. 지금은 1개이므로 0.
		__uuidof(ID3D11Texture2D),//버퍼 형식
		(LPVOID*)&pBackBuffer);//받아온 버퍼
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer,
		nullptr,//자원 형식
		&g_pRenderTargetView);
	pBackBuffer->Release();//get 해온 뒤에는 꼭 release
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1,//렌더 대상 갯수. 장면 분할시 1 초과
		&g_pRenderTargetView,
		nullptr);//깊이 / 스텐실 버퍼


	D3D11_VIEWPORT vp;
	vp.Width = 800;//뷰포트 너비
	vp.Height = 600;//뷰포트 높이
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	vp.TopLeftX = 0.f;//그리기 시작 원점
	vp.TopLeftY = 1.f;
	g_pImmediateContext->RSSetViewports(1, &vp);//RS : rasterizer stage

	return hr;
}

void Render()
{
	//just clear the backbuffer
	float ClearColor[4] = { 0.f,0.125f,0.3f,1.f };//r,g,b,a

	//clear
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	//render - 백버퍼를 프론트버퍼에 그린다
	g_pSwapChain->Present(0, 0);
}

void CleanupDevice()
{
	if (g_pImmediateContext)
		g_pImmediateContext->ClearState();
	if (g_pRenderTargetView)
		g_pRenderTargetView->Release();
	if (g_pSwapChain)
		g_pSwapChain->Release();
	if (g_pd3dDevice)
		g_pd3dDevice->Release();
}

LRESULT CALLBACK WndProc(HWND hWnd
	, UINT message
	, WPARAM wParam
	, LPARAM lParam);

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdParam,
	int nCmdShow)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;

	if (!RegisterClassEx(&wcex))
		return 0;

	HWND	hWnd = CreateWindowEx(WS_EX_APPWINDOW
		, szWindowClass
		, szTitle
		, WS_OVERLAPPEDWINDOW
		, CW_USEDEFAULT
		, CW_USEDEFAULT
		, 800
		, 600
		, NULL
		, NULL
		, hInstance
		, NULL);

	if (!hWnd)
		return 0;

	ShowWindow(hWnd, nCmdShow);

	g_hWnd = hWnd;
	InitDevice();

	MSG			msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}
	CleanupDevice();
	return (int)msg.wParam;
}

// 메시지 처리 함수
LRESULT CALLBACK WndProc(HWND hWnd
	, UINT message
	, WPARAM wParam
	, LPARAM lParam)
{
	HDC	hdc;
	PAINTSTRUCT	ps;

	switch (message)
	{
	case WM_CREATE:

		break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}