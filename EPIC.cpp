// EPIC.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "EPIC.h"
#include <math.h>
#include <map>
#include "Extrapolator.h"

#pragma warning(disable: 4996)

#define MAX_LOADSTRING 100

//  90 ms latency
#define LATENCY 0.09
//  30 ms jitter
#define JITTER 0.03
//  1/10 drop
#define DROPRATE 0.1
//  store drawing positions 20 times a second
#define STORERATE 20
//  send data packets 20 times a second
#define SENDRATE 20


double lastReadTime = 0;
struct Pos {
  float x, y;
};
float vel = 80.0f;
float turn = 5;
float dir = 0.0f;
float M_PI = 3.1415927f;
Pos myPos;
double lastRecordedPos = 0;
Pos recordArray[128];
double recordTime[128];

double lastSentTime;
Pos extrapolatedPos[128];
Extrapolator<2, float> intPos;

bool gPointDisplay = false;
bool gPaused = false;
bool gStepping = false;

double pauseDelta = 0;
double lastPauseTime = 0;

HGDIOBJ redPen, bluePen;
HBRUSH redBrush, blueBrush;

void InitRecordArray()
{
  myPos.x = 200;
  myPos.y = 150;
  intPos.Reset(0, 0, (float *)&myPos.x);
  for (int i = 0; i < 128; ++i) {
    recordArray[i] = myPos;
    recordTime[i] = 0;
    extrapolatedPos[i] = myPos;
  }
}

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

void UpdateTime();
void DrawWindow(HWND, HDC);

HWND hWnd;
bool gRunning = true;
bool gActive = true;
bool keyDown[256];

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

  InitRealTime();
  InitRecordArray();
  timeBeginPeriod(3);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_EPIC, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EPIC));

	// Main message loop:
	while (gRunning)
	{
	  if (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE))
	  {
		  if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		  {
			  TranslateMessage(&msg);
			  DispatchMessage(&msg);
		  }
    }
    else
    {
      Sleep(2);
      UpdateTime();
    }
	}

  timeEndPeriod(3);
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EPIC));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_EPIC);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   redPen = ::CreatePen(PS_SOLID, 1, RGB(200,0,0));
   redBrush = ::CreateSolidBrush(RGB(200,0,0));
   bluePen = ::CreatePen(PS_SOLID, 1, RGB(0,0,255));
   blueBrush = ::CreateSolidBrush(RGB(0,0,255));
   ::SelectObject(::GetDC(hWnd), redPen);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
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
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		::TextOut(hdc, 10, 10, L"Arrow keys to turn.", 19);
		::SetTextColor(hdc, RGB(200,0,0));
		::TextOut(hdc, 10, 25, L"Red = Original;", 15);
		::SetTextColor(hdc, RGB(0,0,255));
		::TextOut(hdc, 120, 25, L"Blue = Extrapolated", 19);
		::SetTextColor(hdc, RGB(0,0,0));
		{
		  wchar_t buf[100];
		  _snwprintf(buf, 100, L"Latency %d ms Jitter %d ms Droprate %d%%", (int)(LATENCY*1000), (int)(JITTER*1000),
		      (int)(DROPRATE*100));
		  buf[99] = 0;
		  ::TextOut(hdc, 10, 40, buf, (INT)wcslen(buf));
		}
		::TextOut(hdc, 10, 55, L"F2: change draw mode", 20);
		::TextOut(hdc, 10, 70, L"F3: pause/go", 12);
		::TextOut(hdc, 10, 85, L"F4: single step", 15);
		if (gPaused) {
		  ::SetTextColor(hdc, RGB(255,0,0));
		  ::TextOut(hdc, 300, 10, L"PAUSED", 6);
		  ::SetTextColor(hdc, RGB(0,0,0));
		}
		if (gPointDisplay) {
		  ::TextOut(hdc, 300, 25, L"POINTS", 6);
		}
		else {
		  ::TextOut(hdc, 300, 25, L"LINES", 5);
		}
    DrawWindow(hWnd, hdc);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		gRunning = false;
		break;

  case WM_SYSKEYDOWN:
  case WM_KEYDOWN:
    keyDown[wParam&0xff] = true;
    break;
  case WM_SYSKEYUP:
  case WM_KEYUP:
    keyDown[wParam&0xff] = false;
    break;
  case WM_ACTIVATEAPP:
    gActive = (wParam != 0);
    memset(keyDown, 0, sizeof(keyDown));
    break;
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


long long baseReading;
long long lastRead;
double tickMultiply;
long long maxDelta;

void InitRealTime()
{
  long long tps;
  QueryPerformanceFrequency((LARGE_INTEGER *)&tps);
  tickMultiply = 1.0 / (double)tps;
  maxDelta = (long long)(tps * 0.1);
  QueryPerformanceCounter((LARGE_INTEGER *)&baseReading);
  lastRead = baseReading;
}

double GetRealTime()
{
  long long now;
  QueryPerformanceCounter((LARGE_INTEGER *)&now);
  //  work around dual-core bug
  if (now < lastRead) {
    now = lastRead + 1;
  }
  if (now - lastRead > maxDelta) {
    //  don't advance time too much all at once
    baseReading += now - lastRead - maxDelta;
  }
  lastRead = now;
  return (now - baseReading) * tickMultiply;
}

struct Packet {
  double time;
  Pos pos;
};

std::map<double, Packet> packetQueue;

double Latency()
{
  //  there might be some jitter!
  return LATENCY + (rand() & 0x7fff) / (32768.0 / (JITTER + 1e-6));
}

void SendPacket(Pos const &pos, double time)
{
  if ((rand() & 0x7fff) / 32768.0 < DROPRATE) {
    return;
  }
  Packet p;
  p.time = time;
  p.pos = pos;
  packetQueue[time + Latency()] = p;
}

void DeliverPacket(Packet const &packet, double now)
{
  intPos.AddSample(packet.time, now, &packet.pos.x);
}

void ReceivePackets(double now)
{
  while (packetQueue.size() > 0) {
    std::map<double, Packet>::iterator ptr = packetQueue.begin();
    if ((*ptr).first < now) {
      DeliverPacket((*ptr).second, now);
      packetQueue.erase(ptr);
    }
    else {
      break;
    }
  }
}

void UpdateTime()
{
  RECT r;
  ::GetClientRect(hWnd, &r);
  double now = GetRealTime();
  if (keyDown[VK_F3]) {
    gPaused = !gPaused;
    if (gPaused) {
      lastPauseTime = now;
    }
    keyDown[VK_F3] = false;
    ::InvalidateRect(hWnd, 0, true);
  }
  if (keyDown[VK_F4]) {
    ::InvalidateRect(hWnd, 0, true);
    gPaused = false;
    gStepping = true;
    keyDown[VK_F4] = false;
  }
  if (keyDown[VK_F2]) {
    gPointDisplay = !gPointDisplay;
    keyDown[VK_F2] = false;
    ::InvalidateRect(hWnd, 0, true);
  }

  if (gPaused) {
    pauseDelta += (now - lastPauseTime);
    lastPauseTime = now;
    return;
  }
  
  now -= pauseDelta;
  ReceivePackets(now);
  double dt = now - lastReadTime;
  lastReadTime = now;
  if (keyDown[VK_RIGHT]) {
    dir -= float(dt * turn);
  }
  if (keyDown[VK_LEFT]) {
    dir += float(dt * turn);
  }
  if (dir < -M_PI) {
    dir += 2 * M_PI;
  }
  if (dir > M_PI) {
    dir -= 2 * M_PI;
  }
  myPos.x += float(cos(dir) * vel * dt);
  myPos.y -= float(sin(dir) * vel * dt);
  if (myPos.x > r.right) {
    myPos.x = (float)r.right;
    //  bounce
    if (fabs(dir) < M_PI / 2) {
      if (dir > 0) {
        dir = M_PI - dir;
      }
      else {
        dir = -M_PI - dir;
      }
    }
  }
  if (myPos.x < 0) {
    myPos.x = 0;
    if (fabs(dir) > M_PI / 2) {
      if (dir > 0) {
        dir = M_PI - dir;
      }
      else {
        dir = -M_PI - dir;
      }
    }
  }
  if (myPos.y > r.bottom) {
    myPos.y = (float)r.bottom;
    if (dir < 0) {
      dir = -dir;
    }
  }
  if (myPos.y < 0) {
    myPos.y = 0;
    if (dir > 0) {
      dir = -dir;
    }
  }

  recordArray[0] = myPos;
  intPos.ReadPosition(now, &extrapolatedPos[0].x);
  if (now >= lastRecordedPos + 1.0/STORERATE) {
    memmove(&extrapolatedPos[1], &extrapolatedPos[0], sizeof(extrapolatedPos) - sizeof(extrapolatedPos[0]));
    memmove(&recordArray[1], &recordArray[0], sizeof(recordArray) - sizeof(recordArray[0]));
    lastRecordedPos = now;
    if (gStepping) {
      gStepping = false;
      gPaused = true;
      lastPauseTime = GetRealTime();
      ::InvalidateRect(hWnd, 0, true);
    }
  }
  if (now >= lastSentTime + 1.0/SENDRATE) {
    SendPacket(myPos, now);
    lastSentTime = now;
  }
  DrawWindow(hWnd, GetDC(hWnd));
}

void DrawWindow(HWND, HDC dc)
{
  POINT pt;
  RECT r;

  ::SelectObject(dc, bluePen);
  if (gPointDisplay) {
    for (int i = 0; i < 128; ++i) {
      if (!(i & 3)) {
        r.left = (int)extrapolatedPos[127-i].x;
        r.top = (int)extrapolatedPos[127-i].y;
        r.right = r.left + 1;
        r.bottom = r.top + 1;
        ::FrameRect(dc, &r, blueBrush);
      }
    }
  }
  else {
    ::MoveToEx(dc, (int)extrapolatedPos[127].x, (int)extrapolatedPos[127].y, &pt);
    for (int i = 0; i < 128; ++i) {
      ::LineTo(dc, (int)extrapolatedPos[127-i].x, (int)extrapolatedPos[127-i].y);
    }
  }

  ::SelectObject(dc, redPen);
  if (gPointDisplay) {
    for (int i = 0; i < 128; ++i) {
      if (!(i & 3)) {
        r.left = (int)recordArray[127-i].x;
        r.top = (int)recordArray[127-i].y;
        r.right = r.left + 1;
        r.bottom = r.top + 1;
        ::FrameRect(dc, &r, redBrush);
      }
    }
  }
  else {
    ::MoveToEx(dc, (int)recordArray[127].x, (int)recordArray[127].y, &pt);
    for (int i = 1; i < 128; ++i) {
      ::LineTo(dc, (int)recordArray[127-i].x, (int)recordArray[127-i].y);
    }
  }
}


#if !defined(NDEBUG)
class ExtrapolatorTest {
  public:
    ExtrapolatorTest() {
      Extrapolator<1, float> i;
      float f = 0, p;
      i.Reset(0.1, 0.1, &f);
      assert(i.ReadPosition(0.1, &p));
      assert(p == 0);
      assert(!i.AddSample(0, 1, &p));
      f = 1;
      assert(i.AddSample(1, 1.5, &f));
      assert(i.EstimateLatency() < 0.5f);
      assert(i.EstimateLatency() > 0.1f);
      assert(i.EstimateUpdateTime() > 0.4f);
      assert(i.EstimateUpdateTime() < 1.0f);
      f = 2;
      assert(i.AddSample(1.5, 2, &f));
      assert(i.ReadPosition(2, &p));
      assert(fabs(2.5 - p) < 0.25);
      assert(i.EstimateLatency() < 0.5f);
      assert(i.EstimateLatency() > 0.3f);
      assert(i.EstimateUpdateTime() > 0.4f);
      assert(i.EstimateUpdateTime() < 0.6f);
      f = 3;
      assert(i.AddSample(2, 2.5, &f));
      assert(i.ReadPosition(2.5, &p));
      assert(fabs(4 - p) < 0.125);
      f = 4;
      assert(i.AddSample(2.5, 3, &f));
      assert(i.ReadPosition(3, &p));
      assert(fabs(5 - p) < 0.07);
      assert(i.ReadPosition(3.25, &p));
      assert(fabs(5.5 - p) < 0.07);
      //  don't allow extrapolation too far forward
      assert(!i.ReadPosition(4, &p));
    }
};

ExtrapolatorTest sTest;
#endif
