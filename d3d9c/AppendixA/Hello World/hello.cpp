//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: hello.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Demonstrates creating a Windows application.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

// Include the windows header file, this has all the
// Win32 API structures, types, and function declarations
// we need to program Windows.
#include <windows.h>

// The main window handle.  This is used to identify
// the main window we are going to create.
HWND MainWindowHandle = 0; 

// Wraps the code necessary to initialize a windows
// application.  Function returns true if initialization
// was successful, else it returns false.
bool InitWindowsApp(HINSTANCE instanceHandle, int show); 

// Wraps the message loop code.
int  Run();               
          
// The window procedure, handles events our window
// receives.
LRESULT CALLBACK WndProc(HWND hWnd,
						 UINT msg,
						 WPARAM wParam,
						 LPARAM lParam); 

// Windows equivalant to main()
int WINAPI WinMain(HINSTANCE hInstance,  
			       HINSTANCE hPrevInstance, 
			       PSTR      pCmdLine, 
		           int       nShowCmd)
{
	// First we create and initialize our Windows 
	// application.  Notice we pass the application 
	// hInstance and the nShowCmd from WinMain as
	// parameters.
	if(!InitWindowsApp(hInstance, nShowCmd)) 
	{
		::MessageBox(0, "Init - Failed", "Error", MB_OK);
		return 0;
	}

	// Once our application has been created and 
	// initialized we enter the message loop.  We
	// stay in the message loop until a WM_QUIT
	// mesage is received, indicating the application
	// should be terminated.
	return Run(); // enter message loop
}

bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
	// The first task to creating a window is to describe
	// its characteristics by filling out a WNDCLASS 
	// structure.
	WNDCLASS wc; 

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = instanceHandle;
	wc.hIcon         = ::LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = ::LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = 
	static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
	wc.lpszMenuName  = 0;
	wc.lpszClassName = "Hello";

	// Then we register this window class description 
	// with Windows so that we can create a window based
	// on that description.
	if(!::RegisterClass(&wc)) 
	{
		::MessageBox(0, "RegisterClass - Failed", 0, 0);
		return false;
	}

	// With our window class description registered, we
	// can create a window with the CreateWindow function.
	// Note, this function returns a HWND to the created
	// window, which we save in MainWindowHandle.  Through
	// MainWindowHandle we can reference this particular
	// window we are creating.
	MainWindowHandle = ::CreateWindow(
		                   "Hello",
		                   "Hello",
		                   WS_OVERLAPPEDWINDOW,
		                   CW_USEDEFAULT, 
		                   CW_USEDEFAULT,
		                   CW_USEDEFAULT,
		                   CW_USEDEFAULT,
		                   0,
		                   0, 
		                   instanceHandle,
		                   0);

	if(MainWindowHandle == 0)
	{
		::MessageBox(0, "CreateWindow - Failed", 0, 0);
		return false;
	}

	// Finally we show and update the window we just created.
	// Observe we pass MainWindowHandle to these functions so
	// that these functions know what particular window to 
	// show and update.
	::ShowWindow(MainWindowHandle, show);
	::UpdateWindow(MainWindowHandle);

	return true;
}

int Run()
{
	MSG msg;
	::ZeroMemory(&msg, sizeof(MSG));

	// Loop until we get a WM_QUIT message.  The
	// function GetMessage will only return 0 (false)
	// when a WM_QUIT message is received, which
	// effectively exits the loop.
	while(::GetMessage(&msg, 0, 0, 0) )
	{
		// Translate the message, and then dispatch it
		// to the appropriate window procedure.
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND   windowHandle,
				         UINT   msg,     
				         WPARAM wParam,
				         LPARAM lParam)
{
	// Handle some specific messages:
	switch( msg )
	{
		// In the case the left mouse button was pressed,
		// then display a message box.
	case WM_LBUTTONDOWN:
		::MessageBox(0, "Hello, World", "Hello", MB_OK);
		return 0;
		
		// In the case the escape key was pressed, then
		// destroy the main application window, which is
		// identified by MainWindowHandle.
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
			::DestroyWindow(MainWindowHandle);
		return 0;

		// In the case of a destroy message, then
		// send a quit message, which will terminate
		// the message loop.
	case WM_DESTROY: 
		::PostQuitMessage(0); 
		return 0;
	}

	// Forward any other messages we didn't handle
	// above to the default window procedure.
	return ::DefWindowProc(windowHandle,
                           msg,
                           wParam,
                           lParam);
}
