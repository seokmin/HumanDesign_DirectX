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
//win32������ DC�� ����.
//DX������ ���� ��� ���� �� �ڿ��� �����ϰ� GPU�� ������ ��� ����
ID3D11DeviceContext*	g_pImmediateContext = nullptr;
ID3D11RenderTargetView*	g_pRenderTargetView = nullptr;


HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	//Flag ����
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;//����� layer�� Ȱ��ȭ�Ͽ� outputâ�� ����� �޽����� ����Ѵ�
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
	sd.BufferCount = 1; //����� ����. 1�� ���������� ������۸�

	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //����� ����
	sd.BufferDesc.RefreshRate.Numerator = 60; //����
	sd.BufferDesc.RefreshRate.Denominator = 1; //�и�

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //����� ������
	sd.OutputWindow = g_hWnd; //��� Ÿ��
	sd.SampleDesc.Count = 1;//��Ƽ���ø�.(MSAA. CheckMultisampleQualityLevels()�� ���� �������� ǰ�� ������ �˾Ƴ� �Ŀ� ��밡��)
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(
		0,//�⺻ ���÷��� ����� ���
		D3D_DRIVER_TYPE_HARDWARE,//3d �ϵ���� ����
		0,//����Ʈ���� ���� ����
		createDeviceFlags,
		featureLevels,
		numFeatureLvels,
		D3D11_SDK_VERSION,//sdk ����
		&sd,//����ü�� desc
		&g_pSwapChain,//������ ����ü��
		&g_pd3dDevice,//������ device
		&g_featureLevel,//���� featurelevel
		&g_pImmediateContext);//������ immediateContext
	if (FAILED(hr))
		return hr;

	
	//Create a render target view
	ID3D11Texture2D*	pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0,//�ĸ� ���� �ε���. �������϶� �߿�. ������ 1���̹Ƿ� 0.
		__uuidof(ID3D11Texture2D),//���� ����
		(LPVOID*)&pBackBuffer);//�޾ƿ� ����
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer,
		nullptr,//�ڿ� ����
		&g_pRenderTargetView);
	pBackBuffer->Release();//get �ؿ� �ڿ��� �� release
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1,//���� ��� ����. ��� ���ҽ� 1 �ʰ�
		&g_pRenderTargetView,
		nullptr);//���� / ���ٽ� ����


	D3D11_VIEWPORT vp;
	vp.Width = 800;//����Ʈ �ʺ�
	vp.Height = 600;//����Ʈ ����
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	vp.TopLeftX = 0.f;//�׸��� ���� ����
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

	//render - ����۸� ����Ʈ���ۿ� �׸���
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

// �޽��� ó�� �Լ�
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