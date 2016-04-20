#include <windows.h>
#include <time.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dCompiler.h>
#include <d3dx11.h>
#include <dxerr.h>

#include "d3dUtil.h"
//���ۿ� ����ü

template<class T, size_t N>
size_t size(T(&)[N]) { return N; }

struct MyVertex {
	XMFLOAT3 pos;
	XMFLOAT4 color;
};

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

ID3D11VertexShader*		g_pVertexShader = nullptr;//���ؽ����̴�
ID3D11InputLayout*		g_pVertexLayout = nullptr;//�Է������⿡�� ����� �Է� ���̾ƿ�
ID3D11Buffer*			g_pVertexBuffer = nullptr;
ID3D11Buffer*			g_pIndexBuffer = nullptr;
ID3D11PixelShader*		g_pPixelShader = nullptr;//�ȼ����̴�
DWORD					g_nIndex = 0;

#define FAIL_CHECK(_hr_)\
if(FAILED(_hr_)) return _hr_

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
	FAIL_CHECK(hr);


	//Create a render target view
	ID3D11Texture2D*	pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0,//�ĸ� ���� �ε���. �������϶� �߿�. ������ 1���̹Ƿ� 0.
		__uuidof(ID3D11Texture2D),//���� ����
		(LPVOID*)&pBackBuffer);//�޾ƿ� ����
	FAIL_CHECK(hr);

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer,
		nullptr,//�ڿ� ����
		&g_pRenderTargetView);
	pBackBuffer->Release();//get �ؿ� �ڿ��� �� release
	FAIL_CHECK(hr);

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
	float ClearColor[4] = { 0.3f,0.3f,0.3f,1.f };//r,g,b,a
	//clear
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	//render code
	{
		//set input assembler
		g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);//�߿�

		UINT stride = sizeof(MyVertex);
		UINT offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
		g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		//set shader and draw
		g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
		g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
		g_pImmediateContext->DrawIndexed(6, 0, 0);
	}


	//render - ����۸� ����Ʈ���ۿ� �׸���
	g_pSwapChain->Present(0, 0);
}

#define CHECK_AND_RELEASE(_x_) if(_x_) _x_->Release()
void CleanupDevice()
{
	CHECK_AND_RELEASE(g_pVertexBuffer);
	CHECK_AND_RELEASE(g_pIndexBuffer);
	CHECK_AND_RELEASE(g_pVertexLayout);
	CHECK_AND_RELEASE(g_pVertexShader);
	CHECK_AND_RELEASE(g_pPixelShader);

	if (g_pImmediateContext)
		g_pImmediateContext->ClearState();
	if (g_pRenderTargetView)
		g_pRenderTargetView->Release();
	if (g_pSwapChain)
		g_pSwapChain->Release();
	if (g_pd3dDevice)
		g_pd3dDevice->Release();
}

HRESULT CreateShader()
{
	//���ؽ� ���̴� ������
	ID3DBlob *pErrorBlob = nullptr;
	ID3DBlob *pVSBlob = nullptr;
	HRESULT hr = D3DX11CompileFromFile(
		L"MyShader.fx", 0, 0,
		"VS", "vs_5_0",//�������� �Լ� �̸�, ���̴� ����
		0, 0, 0,
		&pVSBlob, &pErrorBlob, 0);
	FAIL_CHECK(hr);

	//���ؽ� ���̴� ����
	hr = g_pd3dDevice->CreateVertexShader(
		pVSBlob->GetBufferPointer(),//������ �� ���̴�
		pVSBlob->GetBufferSize(),
		0, &g_pVertexShader);
	FAIL_CHECK(hr);


	//�Է� ���̾ƿ� ��ü ����
	D3D11_INPUT_ELEMENT_DESC	layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};//D3D!!_INPUT_ELEMENT_DESC : Semantic, Semantic Index, �ڷ�����, ����� ����(0~15), AlignedByteOffset(D3D11_APPEND_ALIGNED_ELEMENT ���� �ڵ� ���,�Է� �з�(������,�ν��Ͻ���), ���к� �ݺ� ���� ����
	UINT numElements = ARRAYSIZE(layout);
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements,//����
		pVSBlob->GetBufferPointer(),//�����ϵ� ���̴�
		pVSBlob->GetBufferSize(),
		&g_pVertexLayout);
	pVSBlob->Release();
	FAIL_CHECK(hr);
	

	//�ȼ� ���̴� ������ �� ����
	ID3DBlob *pPSBlob = nullptr;
	hr = D3DX11CompileFromFile(
		L"MyShader.fx", 0, 0,
		"PS", "ps_5_0",//�������� �Լ� �̸�, ���̴� ����
		0, 0, 0,
		&pPSBlob, &pErrorBlob, 0);
	FAIL_CHECK(hr);
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), 0, &g_pPixelShader);
	pPSBlob->Release();
	FAIL_CHECK(hr);

	return hr;
}

//���� ���� ����
HRESULT CreateVertexBufferAndIndexBuffer()
{
	MyVertex vertices[] = 
	{
		{XMFLOAT3(0.7f,0.7f,1.f),(const float*)&Colors::Red},	//0
		{XMFLOAT3(0.7f,-0.7f,1.f),(const float*)&Colors::Blue },	//1
		{XMFLOAT3(-0.7f,-0.7f,1.f),(const float*)&Colors::Green },//2
		{ XMFLOAT3(-0.7f,0.7f,1.f),(const float*)&Colors::White },//3
	};
	UINT indices[] =
	{
		0,1,2,
		2,3,0
	};
	g_nIndex = size(indices);

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(vertices);
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = vertices;//�ʱ�ȭ�ϱ� ���� ���� �迭 ������
	auto hr = g_pd3dDevice->CreateBuffer(&bd,//������ ������ ����
		&initData,
		&g_pVertexBuffer);
	FAIL_CHECK(hr);

	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(indices);
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;

	ZeroMemory(&initData,sizeof(initData));
	initData.pSysMem = indices;
	hr = g_pd3dDevice->CreateBuffer(&bd,
		&initData,
		&g_pIndexBuffer);
	return hr;
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
	//���̴� ����
	CreateShader();
	CreateVertexBufferAndIndexBuffer();

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