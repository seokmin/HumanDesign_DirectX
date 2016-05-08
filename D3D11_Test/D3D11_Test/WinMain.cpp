#include <windows.h>
#include <time.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dCompiler.h>
#include <d3dx11.h>
#include "dxerr.h"
#include "d3dUtil.h"
#include "MyTime.h"
//���ۿ� ����ü

template<class T, size_t N>
size_t size(T(&)[N]) { return N; }

struct MyVertex {
	XMFLOAT3 pos;
	XMFLOAT4 color;

	XMFLOAT3 normal;

	XMFLOAT2 tex;
};
struct	 ConstantBuffer
{
	XMMATRIX wvp;
	XMMATRIX world;
	XMFLOAT4 lightDir;
	XMFLOAT4 lightColor;
};
XMFLOAT4 lightDirection = { 1.f,1.f,1.f,1.f };
XMFLOAT4 lightColor = { 1.f,1.f,1.f,1.f };
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



XMMATRIX                g_World;
XMMATRIX                g_World2;
XMMATRIX                g_View;
XMMATRIX                g_Projection;
ID3D11Buffer*			g_pConstantBuffer = nullptr;

ID3D11Texture2D*			g_pDepthStencil = nullptr;
ID3D11DepthStencilView*		g_pDepthStencilView = nullptr;

ID3D11ShaderResourceView*	g_pTextureRV = nullptr;
ID3D11SamplerState*			g_pSamplerLinear = nullptr;

#define FAIL_CHECK(_hr_)\
if(FAILED(_hr_)) return _hr_

ID3D11RasterizerState *		g_pSolidRS;
ID3D11RasterizerState *		g_pWireFrameRS;

void CreateRenderState1()
{
	D3D11_RASTERIZER_DESC      rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;		// Fill �ɼ�
	rasterizerDesc.CullMode = D3D11_CULL_BACK;	// Culling �ɼ�
	rasterizerDesc.FrontCounterClockwise = false;		// ��/�޸� ���� ����

	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pSolidRS);
}
void CreateRenderState2()
{
	D3D11_RASTERIZER_DESC      rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;		// Fill �ɼ�
	rasterizerDesc.CullMode = D3D11_CULL_NONE;	// Culling �ɼ�
	rasterizerDesc.FrontCounterClockwise = false;		// ��/�޸� ���� ����

	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pWireFrameRS);
}

HRESULT   LoadTexture()
{
	
	HRESULT hr = D3DX11CreateShaderResourceViewFromFile(
		g_pd3dDevice,
		L"Textures/images.jpg",
		nullptr,
		nullptr,
		&g_pTextureRV,
		nullptr
	);
	FAIL_CHECK(hr);
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	FAIL_CHECK(hr);

	return S_OK;
}

void CalculateMatrixForBox(float dt);
void CalculateMatrixForBox2(float dt);
void CreateDepthStencilTexture();

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

	CreateDepthStencilTexture();
	g_pImmediateContext->OMSetRenderTargets(1,//���� ��� ����. ��� ���ҽ� 1 �ʰ�
		&g_pRenderTargetView,
		g_pDepthStencilView);//���� / ���ٽ� ����


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

void Render(float dt)
{
	//just clear the backbuffer
	float ClearColor[4] = { 0.3f,0.3f,0.3f,1.f };//r,g,b,a
	//clear
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);

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

		//texture settings
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->RSSetState(g_pSolidRS);
		CalculateMatrixForBox(dt);
		g_pImmediateContext->DrawIndexed(36, 0, 0);
		
		//g_pImmediateContext->RSSetState(g_pWireFrameRS);
		CalculateMatrixForBox2(dt);
		g_pImmediateContext->DrawIndexed(36, 0, 0);
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
	CHECK_AND_RELEASE(g_pConstantBuffer);
	CHECK_AND_RELEASE(g_pDepthStencilView);
	CHECK_AND_RELEASE(g_pDepthStencil);
	CHECK_AND_RELEASE(g_pSolidRS);
	CHECK_AND_RELEASE(g_pSamplerLinear);
	CHECK_AND_RELEASE(g_pTextureRV);
	if (g_pImmediateContext)
		g_pImmediateContext->ClearState();
	if (g_pRenderTargetView)
		g_pRenderTargetView->Release();
	if (g_pSwapChain)
		g_pSwapChain->Release();
	if (g_pd3dDevice)
		g_pd3dDevice->Release();
}

void CreateDepthStencilTexture()
{
	//create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = 800;
	descDepth.Height = 600;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);

	//create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	descDSV.Flags = 0;
	g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
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
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0}
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
// 	MyVertex vertices[] = 
// 	{
// 		{XMFLOAT3(0.7f,0.7f,1.f),(const float*)&Colors::Red},	//0
// 		{XMFLOAT3(0.7f,-0.7f,1.f),(const float*)&Colors::Blue },	//1
// 		{XMFLOAT3(-0.7f,-0.7f,1.f),(const float*)&Colors::Green },//2
// 		{ XMFLOAT3(-0.7f,0.7f,1.f),(const float*)&Colors::White },//3
// 	};
// 	UINT indices[] =
// 	{
// 		0,1,2,
// 		2,3,0
// 	};
	MyVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f),  XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT3(-0.33f, 0.33f, -0.33f) , XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f),   XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT3(0.33f, 0.33f, -0.33f) , XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f),    XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.33f, 0.33f, 0.33f) , XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f),   XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(-0.33f, 0.33f, 0.33f) , XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f), XMFLOAT3(-0.33f, -0.33f, -0.33f) , XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f),  XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f), XMFLOAT3(0.33f, -0.33f, -0.33f) , XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f),   XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.33f, -0.33f, 0.33f) , XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f),  XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(-0.33f, -0.33f, 0.33f) , XMFLOAT2(0.0f, 1.0f) },
	};


	UINT indices[] =
	{
		3, 1, 0,
		2, 1, 3,
		0, 5, 4,
		1, 5, 0,
		3, 4, 7,
		0, 4, 3,
		1, 6, 5,
		2, 6, 1,
		2, 7, 6,
		3, 7, 2,
		6, 4, 5,
		7, 4, 6,
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

HRESULT CreateConstantBuffer()
{
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;
	g_pd3dDevice->CreateBuffer(&cbd, nullptr, &g_pConstantBuffer);

	return S_OK;
}

void CalculateMatrixForBox(float dt)
{
	XMMATRIX mat = XMMatrixRotationY(dt);
	mat *= XMMatrixRotationX(-dt);
	g_World = mat;

	XMMATRIX wvp = g_World * g_View * g_Projection;
	ConstantBuffer cb;
	cb.wvp = XMMatrixTranspose(wvp);

	cb.world = XMMatrixTranspose(g_World);
	cb.lightDir = lightDirection;
	cb.lightColor = lightColor;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}
void CalculateMatrixForBox2(float dt)
{
	float scaleValue = sinf(dt)*0.5f + 1;
	XMMATRIX scale = XMMatrixScaling(scaleValue, scaleValue, scaleValue); // scale
	XMMATRIX rotate = XMMatrixRotationZ(dt);	// rotate
	float moveValue = cosf(dt) * 5.f;	// move position
	XMMATRIX position = XMMatrixTranslation(moveValue, 0.0f, 0.0f);
	g_World2 = scale * rotate * position;	// S * R * T

	XMMATRIX wvp = g_World2 * g_View * g_Projection;
	ConstantBuffer cb;
	cb.wvp = XMMatrixTranspose(wvp);

	cb.world = XMMatrixTranspose(g_World);
	cb.lightDir = lightDirection;
	cb.lightColor = lightColor;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}

HRESULT InitMatrix()
{
	

	g_World = XMMatrixIdentity();

	XMVECTOR pos = XMVectorSet(0.f, 0.f, -5.f, 1.f);
	XMVECTOR target = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	g_View = XMMatrixLookAtLH(pos, target, up);

	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2,
		800.f / (FLOAT)600.f,
		0.01f, 100.f);
	return S_OK;

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

	CreateConstantBuffer();

	CreateRenderState1();
	CreateRenderState2();

	LoadTexture();
	InitMatrix();

	CMyTime::GetInstance()->Init();
	MSG			msg;
	while (true)
	{
		CMyTime::GetInstance()->ProcessTime();
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			static float deltaTime = 0;
			deltaTime += CMyTime::GetInstance()->GetElapsedTime();		// GetDeltaTime�� ������ 0.00005f���� �������� �� ��.

			Render(deltaTime);
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