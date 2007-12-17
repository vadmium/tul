/*
  main.cpp
  By Martin Panter
*/

/*#if defined UNICODE && !defined _UNICODE
#  define _UNICODE
#endif*/

#include "main.hpp"
#include "resource.h"
#include "shield.hpp"
#include <sstream>
#include <string>

//#define numelem(array) (sizeof (array) / sizeof (array)[0])

class MainWnd: Measurer {
public:
  /* stuff for the window class */
  static void reg();
  static void unreg();
  
  static MainWnd *create();
  static LRESULT
    CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
  HWND getHWnd() const {return hWnd;}; // not always constant?!?!

private:
  HWND hWnd;
  virtual ~MainWnd() {}; // could zero the "this" pointer in GWL_USERDATA
  
  static const int IDC_MEASURE = 1;
  static const int IDC_X = 2;
  static const int IDC_Y = 3;
  static const int IDC_AREA = 4;
  static const int IDC_XRES = 5;
  static const int IDC_YRES = 6;
  static const int IDC_DIST = 7;
  
  Shield *shield;
  double x_res;
  double y_res;
  bool mouse_valid;     // set when we have a valid mouse position
  int x_mouse;
  int y_mouse;
  double dist;
  double pixel_area;
  
  void update_x();
  void update_y();
  void update_dist();
  void update_area();
  
  virtual LRESULT on_create(LPCREATESTRUCT lpcs);
  virtual void on_destroy();

  virtual void paint();  
  virtual void on_command(WORD wNotifyCode, WORD wID, HWND hwndCtl);
  
  virtual void on_mouse_move(int x, int y);
  virtual void dist_reset() {dist = 0; update_dist();};
  virtual void dist_add(double x, double y);
  virtual void area(double area) {pixel_area = area; update_area();};

protected:
  MainWnd(HWND hWnd): hWnd(hWnd) {
    SetWindowLong(hWnd, GWL_USERDATA, reinterpret_cast< LONG >(this));
  };
};

//HINSTANCE hInstance;

int WINAPI WinMain(
  HINSTANCE hInstance,
  HINSTANCE hInstPrev,
  LPSTR szCmdLine,
  int nShowCmd
) {
  MainWnd::reg();
  /* Create window. Once created, the window looks after its own
  destruction. */
  MainWnd *main_wnd = MainWnd::create();
  MSG msg;
  if(main_wnd){
  ShowWindow(main_wnd->getHWnd(), nShowCmd);


  while(GetMessage(&msg, NULL, 0, 0))
    /* Remember to quit when GetMessage gets a quit message. */
    //TODO: figure out this binary/trinary thing with getmessage return value (on error)
  {
//    if(TranslateAccelerator(cal_wnd->getHWnd(), hAccel, &msg)) {
//      continue;
//    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}
  MainWnd::unreg();

  return msg.wParam;
}

void MainWnd::reg() {
  WNDCLASS wc;
  wc.style = 0;
  wc.lpfnWndProc = MainWnd::WindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_MAIN));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast< HBRUSH >(COLOR_BTNFACE + 1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "main";
  if(0 == RegisterClass(&wc))
  {
    MessageBox(NULL, "argh failed", "register wndclass", MB_OK);
  }
}

void MainWnd::unreg() {
  if(!UnregisterClass("main", GetModuleHandle(NULL)))
  {
    MessageBox(NULL, "!fail!!", "unregister class", MB_OK);
  }
}

MainWnd *MainWnd::create() {
  DWORD dwStyle = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU |
    WS_MINIMIZEBOX | WS_THICKFRAME | WS_VISIBLE;
  DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_CONTROLPARENT | WS_EX_TOPMOST | WS_EX_WINDOWEDGE;
  RECT rc = {0, 0, 192, 120};
  AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
  HWND hWnd = CreateWindowEx(
    dwExStyle,
    "main",
    "tul",
    dwStyle,
    CW_USEDEFAULT, 0,
    rc.right - rc.left, rc.bottom - rc.top,
    NULL,
    NULL,
    GetModuleHandle(NULL),
    0
  );
  if(!hWnd)return 0;
  return reinterpret_cast< MainWnd * >(GetWindowLong(hWnd, GWL_USERDATA));
}

LRESULT CALLBACK MainWnd::WindowProc(
  HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
) {
  if(WM_NCCREATE == uMsg) {
    if(!DefWindowProc(hWnd, uMsg, wParam, lParam)) {
      return FALSE;
    }
    /* Cannot find /this/ pointer if a message (e.g. WM_GETMINMAXINFO) is
    sent before this message. */
    new MainWnd(hWnd);
    return TRUE;
  }
  
  MainWnd *wnd =
    reinterpret_cast< MainWnd * >(GetWindowLong(hWnd, GWL_USERDATA));
  switch(uMsg) {
  case WM_NCDESTROY:
    delete wnd; // if NC create failed, wnd should be NULL so this is OK
    break; // chain to default handler
  
  case WM_CREATE:
    return wnd->on_create(reinterpret_cast< LPCREATESTRUCT >(lParam));
    
  case WM_DESTROY:
    wnd->on_destroy();
    return 0;
    
  case WM_PAINT:
    wnd->paint();
    return 0;
    
  case WM_COMMAND:
    wnd->on_command(HIWORD(wParam), LOWORD(wParam), reinterpret_cast< HWND >(lParam));
    return 0;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT MainWnd::on_create(LPCREATESTRUCT lpcs) {
  if(0 != DefWindowProc(hWnd, WM_CREATE, 0, reinterpret_cast< LPARAM >(lpcs))) {
    return -1;
  }
  Shield::reg();
  
  CreateWindow(
    "BUTTON",
    "&Measure",
    WS_CHILD | WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX | BS_PUSHLIKE |
    WS_VISIBLE,
    0, 0,
    96, 24,
    hWnd,
    reinterpret_cast< HMENU >(IDC_MEASURE),
    lpcs->hInstance,
    0
  );
  CreateWindowEx(
    WS_EX_STATICEDGE,
    "EDIT",
    "x",
    WS_CHILD | WS_TABSTOP | WS_GROUP | ES_READONLY | ES_LEFT | WS_VISIBLE,
    0, 24,
    64, 24,
    hWnd,
    reinterpret_cast< HMENU >(IDC_X),
    lpcs->hInstance,
    0
  );
  CreateWindowEx(
    WS_EX_STATICEDGE,
    "EDIT",
    "y",
    WS_CHILD | WS_TABSTOP | WS_GROUP | ES_READONLY | ES_LEFT | WS_VISIBLE,
    64, 24,
    64, 24,
    hWnd,
    reinterpret_cast< HMENU >(IDC_Y),
    lpcs->hInstance,
    0
  );
  CreateWindowEx(
    WS_EX_STATICEDGE,
    "EDIT",
    "distance",
    WS_CHILD | WS_TABSTOP | WS_GROUP | ES_READONLY | ES_LEFT | ES_NUMBER |
    WS_VISIBLE,
    0, 48,
    128, 24,
    hWnd,
    reinterpret_cast< HMENU >(IDC_DIST),
    lpcs->hInstance,
    0
  );
  CreateWindowEx(
    WS_EX_STATICEDGE,
    "EDIT",
    "area",
    WS_CHILD | WS_TABSTOP | WS_GROUP | ES_READONLY | ES_LEFT | ES_NUMBER |
    WS_VISIBLE,
    0, 72,
    128, 24,
    hWnd,
    reinterpret_cast< HMENU >(IDC_AREA),
    lpcs->hInstance,
    0
  );
  CreateWindowEx(
    WS_EX_CLIENTEDGE,
    "EDIT",
    "x res",
    WS_CHILD | WS_TABSTOP | WS_GROUP | ES_LEFT | WS_VISIBLE,
    0, 96,
    64, 24,
    hWnd,
    reinterpret_cast< HMENU >(IDC_XRES),
    lpcs->hInstance,
    0
  );
  CreateWindowEx(
    WS_EX_CLIENTEDGE,
    "EDIT",
    "y res",
    WS_CHILD | WS_TABSTOP | WS_GROUP | ES_LEFT | WS_VISIBLE,
    64, 96,
    64, 24,
    hWnd,
    reinterpret_cast< HMENU >(IDC_YRES),
    lpcs->hInstance,
    0
  );
  
  dist = 0;
  pixel_area = 0;
  x_res = 1;
  y_res = 1;
  
  shield = Shield::create(this);
  return 0;
}

void MainWnd::on_destroy() {
  shield->destroy();
  // should destroy measure button window and other controls
  Shield::unreg();
  PostQuitMessage(0);
  DefWindowProc(hWnd, WM_DESTROY, 0, 0);
}

void MainWnd::paint() {
  PAINTSTRUCT ps;
  BeginPaint(hWnd, &ps);
  HBRUSH hBr = static_cast< HBRUSH >(GetStockObject(BLACK_BRUSH));
  RECT rc = {136 - 1, 24 - 1, 136 + 5*8 + 1, 24 + 5*8 + 1};
  FrameRect(ps.hdc, &rc, hBr);
  EndPaint(hWnd, &ps);
}

void MainWnd::on_command(WORD wNotifyCode, WORD wID, HWND hwndCtl) {
  if(NULL != hwndCtl) {
    // message is from a control (not menu item nor accelerator)
    switch(wID) {
    case IDC_MEASURE:
      if(BN_CLICKED != wNotifyCode) {
        break;
      }
      if(BST_CHECKED == SendMessage(hwndCtl, BM_GETCHECK, 0, 0)) {
        shield->show();
      } else {
        shield->hide();
      }
      break;
      
    case IDC_XRES:
      if(EN_CHANGE == wNotifyCode) {
        int text_length = GetWindowTextLength(hwndCtl);
        char *text = new char[text_length + 1];
        GetWindowText(hwndCtl, text, text_length + 1);
        double new_res = atof(text);
        delete[] text;
        if(0 == new_res) {
          break;
        }
        x_res = new_res;
        update_x();
        update_area();
      }
      break;
      
    case IDC_YRES:
      if(EN_CHANGE == wNotifyCode) {
        int text_length = GetWindowTextLength(hwndCtl);
        char *text = new char[text_length + 1];
        GetWindowText(hwndCtl, text, text_length + 1);
        double new_res = atof(text);
        delete[] text;
        if(0 == new_res) {
          break;
        }
        y_res = new_res;
        update_y();
        update_area();
      }
      break;
    }
  }
}

void MainWnd::on_mouse_move(int x, int y) {
  x_mouse = x;
  y_mouse = y;
  mouse_valid = true;
  update_x();
  update_y();
  
  // copy 5 * 5 pixels from the mouse position for the "mouse cam"
  HDC hdc_screen = GetDC(shield->getHWnd());
  HDC hdc_main_wnd = GetDC(hWnd);
  StretchBlt(
    hdc_main_wnd, 136, 24, 5*8, 5*8,
    hdc_screen, x - 2, y - 2, 5, 5,
    SRCCOPY
  );
  ReleaseDC(hWnd, hdc_main_wnd);
  ReleaseDC(shield->getHWnd(), hdc_screen);
}

void MainWnd::update_x() {
  if(!mouse_valid) {
    SetDlgItemText(hWnd, IDC_X, "x");
    return;
  }
  std::ostringstream ost;
  ost << (x_mouse / x_res);
  SetDlgItemText(hWnd, IDC_X, ost.str().c_str());
}

void MainWnd::update_y() {
  if(!mouse_valid) {
    SetDlgItemText(hWnd, IDC_Y, "y");
    return;
  }
  std::ostringstream ost;
  ost << (y_mouse / y_res);
  SetDlgItemText(hWnd, IDC_Y, ost.str().c_str());
}

void MainWnd::update_dist() {
  if(0 == dist) {
    SetDlgItemText(hWnd, IDC_DIST, "dist");
    return;
  }
  std::ostringstream ost;
  ost << dist;
  SetDlgItemText(hWnd, IDC_DIST, ost.str().c_str());
}

void MainWnd::update_area() {
  if(0 == pixel_area) {
    SetDlgItemText(hWnd, IDC_AREA, "area");
    return;
  }
  std::ostringstream ost;
  ost << (pixel_area / x_res / y_res);
  SetDlgItemText(hWnd, IDC_AREA, ost.str().c_str());
}

void MainWnd::dist_add(double x, double y) {
  x /= x_res;
  y /= y_res;
  dist += std::sqrt(x * x + y * y);
  update_dist();
}
