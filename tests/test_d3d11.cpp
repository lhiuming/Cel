// D3D11 tutorial from www.braynzarsoft.net
// with modification and additional comments 

// Include and link appropriate libraries and headers //

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>  // new shader compiler from d3d11; D3DX is deprecated since windows 8
#include <DirectXMath.h>  // new math lib from d3d11 

//Global Declarations - Interfaces//
IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;

///////////////**************new**************////////////////////
ID3D11Buffer* triangleVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3DBlob* VS_Buffer; // not using ID3D10Blob; new from d3dcompiler
ID3DBlob* PS_Buffer;
ID3D11InputLayout* vertLayout;
///////////////**************new**************////////////////////

//Global Declarations - Others//
LPCTSTR WndClassName = "firstwindow";
HWND hwnd = NULL;
HRESULT hr;

const int Width = 300;
const int Height = 300;

//Function Prototypes//
bool InitializeDirect3d11App(HINSTANCE hInstance);
void CleanUp();
bool InitScene();
void UpdateScene();
void DrawScene();

bool InitializeWindow(HINSTANCE hInstance,
  int ShowWnd,
  int width, int height,
  bool windowed);
int messageloop();

LRESULT CALLBACK WndProc(HWND hWnd,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam);

///////////////**************new**************////////////////////
//Vertex Structure and Vertex Layout (Input Layout)//
struct Vertex    //Overloaded Vertex Structure
{
  Vertex() {}
  Vertex(float x, float y, float z)
    : pos(x, y, z) {}

  DirectX::XMFLOAT3 pos;  // DirectXMath use DirectX namespace 
};

D3D11_INPUT_ELEMENT_DESC layout[] =
{
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
UINT numElements = ARRAYSIZE(layout);
///////////////**************new**************////////////////////

int WINAPI WinMain(HINSTANCE hInstance,    //Main windows function
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nShowCmd)
{

  if (!InitializeWindow(hInstance, nShowCmd, Width, Height, true))
  {
    MessageBox(0, "Window Initialization - Failed",
      "Error", MB_OK);
    return 0;
  }

  if (!InitializeDirect3d11App(hInstance))    //Initialize Direct3D
  {
    MessageBox(0, "Direct3D Initialization - Failed",
      "Error", MB_OK);
    return 0;
  }

  if (!InitScene())    //Initialize our scene
  {
    MessageBox(0, "Scene Initialization - Failed",
      "Error", MB_OK);
    return 0;
  }

  messageloop();

  CleanUp();

  return 0;
}

bool InitializeWindow(HINSTANCE hInstance,
  int ShowWnd,
  int width, int height,
  bool windowed)
{
  typedef struct _WNDCLASS {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HANDLE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCTSTR lpszMenuName;
    LPCTSTR lpszClassName;
  } WNDCLASS;

  WNDCLASSEX wc;

  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = NULL;
  wc.cbWndExtra = NULL;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WndClassName;
  wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&wc))
  {
    MessageBox(NULL, "Error registering class",
      "Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  hwnd = CreateWindowEx(
    NULL,
    WndClassName,
    "Lesson 4 - Begin Drawing",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    width, height,
    NULL,
    NULL,
    hInstance,
    NULL
  );

  if (!hwnd)
  {
    MessageBox(NULL, "Error creating window",
      "Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  ShowWindow(hwnd, ShowWnd);
  UpdateWindow(hwnd);

  return true;
}

bool InitializeDirect3d11App(HINSTANCE hInstance)
{
  //Describe our Buffer
  DXGI_MODE_DESC bufferDesc;

  ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

  bufferDesc.Width = Width;
  bufferDesc.Height = Height;
  bufferDesc.RefreshRate.Numerator = 60;
  bufferDesc.RefreshRate.Denominator = 1;
  bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

  //Describe our SwapChain
  DXGI_SWAP_CHAIN_DESC swapChainDesc;

  ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

  swapChainDesc.BufferDesc = bufferDesc;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1;
  swapChainDesc.OutputWindow = hwnd;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


  //Create our SwapChain
  hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
    D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon);

  //Create our BackBuffer
  ID3D11Texture2D* BackBuffer;
  hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

  //Create our Render Target
  hr = d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
  BackBuffer->Release();

  //Set our Render Target
  d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, NULL);

  return true;
}

void CleanUp()
{
  //Release the COM Objects we created
  SwapChain->Release();
  d3d11Device->Release();
  d3d11DevCon->Release();
  renderTargetView->Release();
  ///////////////**************new**************////////////////////
  triangleVertBuffer->Release();
  VS->Release();
  PS->Release();
  VS_Buffer->Release();
  PS_Buffer->Release();
  vertLayout->Release();
  ///////////////**************new**************////////////////////
}

///////////////**************new**************////////////////////
bool InitScene()
{
  //Compile Shaders from shader file
  // NOTE: new interface for D3DCompiler; different from the original tutorial  
  hr = D3DCompileFromFile(
    L"effects.hlsl",  // shader file name 
    0, // shader macros
    0, // shader includes  
    "VS", // shader entry pointer
    "vs_4_0", // shader target: shader model version or effect type 
    0, 0, // two optional flags 
    &VS_Buffer, // recieve compiled shader code 
    0 // receive optional error repot 
  );
  hr = D3DCompileFromFile(L"effects.hlsl", 0, 0, "PS", "ps_4_0", 0, 0, &PS_Buffer, 0);

  //Create the Shader Objects
  hr = d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
  hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

  //Set Vertex and Pixel Shaders
  d3d11DevCon->VSSetShader(VS, 0, 0);
  d3d11DevCon->PSSetShader(PS, 0, 0);

  //Create the vertex buffer
  Vertex v[] =
  {
    Vertex(0.0f, 0.5f, 0.5f),
    Vertex(0.5f, -0.5f, 0.5f),
    Vertex(-0.5f, -0.5f, 0.5f),
  };

  D3D11_BUFFER_DESC vertexBufferDesc;
  ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

  vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vertexBufferDesc.CPUAccessFlags = 0;
  vertexBufferDesc.MiscFlags = 0;

  D3D11_SUBRESOURCE_DATA vertexBufferData;

  ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
  vertexBufferData.pSysMem = v;
  hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &triangleVertBuffer);

  //Set the vertex buffer
  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  d3d11DevCon->IASetVertexBuffers(0, 1, &triangleVertBuffer, &stride, &offset);

  //Create the Input Layout
  d3d11Device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(),
    VS_Buffer->GetBufferSize(), &vertLayout);

  //Set the Input Layout
  d3d11DevCon->IASetInputLayout(vertLayout);

  //Set Primitive Topology
  d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  //Create the Viewport
  D3D11_VIEWPORT viewport;
  ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = Width;
  viewport.Height = Height;

  //Set the Viewport
  d3d11DevCon->RSSetViewports(1, &viewport);

  return true;
}
///////////////**************new**************////////////////////

void UpdateScene()
{

}

///////////////**************new**************////////////////////
void DrawScene()
{
  //Clear our backbuffer
  float bgColor[4] = { (0.0f, 0.0f, 0.0f, 0.0f) };
  d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

  //Draw the triangle
  d3d11DevCon->Draw(3, 0);

  //Present the backbuffer to the screen
  SwapChain->Present(0, 0);
}
///////////////**************new**************////////////////////

int messageloop() {
  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  while (true)
  {
    BOOL PeekMessageL(
      LPMSG lpMsg,
      HWND hWnd,
      UINT wMsgFilterMin,
      UINT wMsgFilterMax,
      UINT wRemoveMsg
    );

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (msg.message == WM_QUIT)
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else {
      // run game code            
      UpdateScene();
      DrawScene();
    }
  }
  return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (msg)
  {
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE) {
      DestroyWindow(hwnd);
    }
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hwnd,
    msg,
    wParam,
    lParam);
}