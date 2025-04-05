// D3D12TestApp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "D3D12TestApp.h"
#include "D3D12Stuff.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                            MyRegisterClass(HINSTANCE hInstance);
std::shared_ptr<D3DBaseClient>  InitInstance(HINSTANCE, int);
LRESULT CALLBACK                WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK                About(HWND, UINT, WPARAM, LPARAM);
 
// Support console output for stdio
struct ConsoleWrapper
{
    ConsoleWrapper()
    {
        if (AllocConsole())
        {
            OutputDebugString(L"Created console.");
            FILE* fDummy;
            freopen_s(&fDummy, "CONIN$", "r", stdin);
            freopen_s(&fDummy, "CONOUT$", "w", stderr);
            freopen_s(&fDummy, "CONOUT$", "w", stdout);
        }
        else
        {
            OutputDebugString(L"Failed to create console.");
        }
        
    }

    ~ConsoleWrapper()
    {
        FreeConsole();
        OutputDebugString(L"Free console.");
    }
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Create a console to support std::cout
    auto pConsole = new ConsoleWrapper;

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D3D12TESTAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    std::shared_ptr<D3DBaseClient> d3dClient = InitInstance(hInstance, nCmdShow);
    if (!d3dClient)
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D3D12TESTAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // If there are Window messages then process them.
        if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Otherwise, draw.
        else
        {
            d3dClient->Draw();
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_D3D12TESTAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_D3D12TESTAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
std::shared_ptr<D3DBaseClient> InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    UINT width = 1280;
    UINT height = 640;
    UINT framesPerSecond = 60;
    UINT swapChainBufferCount = 2;
    
    hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, width, height, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return nullptr;
   }

   // App code.
   bool debugEnable = true;
   std::shared_ptr<D3DBaseClient> d3dClient = D3DBaseClient::Create(
       debugEnable, width, height, framesPerSecond, swapChainBufferCount, hWnd);

   if (!d3dClient)
   {
       return nullptr;
   }

   D3D12_VIEWPORT viewport = {
        .TopLeftX = 0.0,
        .TopLeftY = 0.0,
        .Width = FLOAT(width/2),
        .Height = FLOAT(height/2),
        .MinDepth = 0.0,
        .MaxDepth = 1.0 };
   D3D12_RECT scissorRectangle = {
        .left = 0,
        .top = 0,
        .right = LONG(width/2),
        .bottom = LONG(height/2) };
   d3dClient->OnResize(viewport, scissorRectangle);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return d3dClient;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_ACTIVATE:
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
