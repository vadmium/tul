/*
  shield.cpp
  By Martin Panter
*/

/*#if defined UNICODE && !defined _UNICODE
#  define _UNICODE
#endif*/

#include "shield.hpp"
#include <sstream>
#include <string>

//#define numelem(array) (sizeof (array) / sizeof (array)[0])

void Shield::reg() {
  WNDCLASS wc;
  wc.style = 0;//CS_NOCLOSE;
  wc.lpfnWndProc = Shield::WindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hIcon = NULL;
  wc.hCursor = LoadCursor(NULL, IDC_CROSS);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "shield";
  if(0 == RegisterClass(&wc))
  {
    MessageBox(NULL, "argh failed", "register wndclass", MB_OK);
  }
}

void Shield::unreg() {
  if(!UnregisterClass("shield", GetModuleHandle(NULL)))
  {
    MessageBox(NULL, "!fail!!", "unregister class", MB_OK);
  }
}

Shield *Shield::create(Measurer *m) {
  HWND hWnd = CreateWindowEx(
    WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
    "shield",
    "",
    WS_POPUP, // not visible
    0, 0,
    GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
    NULL,
    NULL,
    GetModuleHandle(NULL),
    m
  );
  if(!hWnd)return 0;
  return reinterpret_cast< Shield * >(GetWindowLong(hWnd, GWL_USERDATA));
}

LRESULT CALLBACK Shield::WindowProc(
  HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
) {
  if(WM_NCCREATE == uMsg) {
    if(!DefWindowProc(hWnd, uMsg, wParam, lParam)) {
      return FALSE;
    }
    /* Cannot find /this/ pointer if a message (e.g. WM_GETMINMAXINFO) is
    sent before this message. */
    new Shield(hWnd, static_cast< Measurer * >(
      reinterpret_cast< LPCREATESTRUCT >(lParam)->lpCreateParams
    ));
    return TRUE;
  }
  
  Shield *wnd =
    reinterpret_cast< Shield * >(GetWindowLong(hWnd, GWL_USERDATA));
  switch(uMsg) {
  case WM_NCDESTROY:
    delete wnd; // if NC create failed, wnd should be NULL so this is OK
    break; // chain to default handler
//  
//  case WM_CREATE:
//    return wnd->on_create(reinterpret_cast< LPCREATESTRUCT >(lParam));
//    
//  case WM_DESTROY:
//    wnd->on_destroy();
//    return 0;

 /* case WM_PAINT:
    wnd->paint();
    return 0;*/
  
  case WM_MOUSEMOVE:
    wnd->on_mouse_move(wParam, LOWORD(lParam), HIWORD(lParam));
    return 0;
    
  case WM_LBUTTONUP:
    wnd->on_l_button_up(wParam, LOWORD(lParam), HIWORD(lParam));
    return 0;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Shield::show() {
  is_drawing = false;
  ShowWindow(hWnd, SW_SHOW);
}

void Shield::hide() {
  clear_drawing();
  ShowWindow(hWnd, SW_HIDE);
}

void Shield::clear_drawing() {
  HDC hDC = GetDC(hWnd);

  // TODO: centralise initialising of the hDC
  SelectObject(hDC, GetStockObject(NULL_BRUSH));
  SetROP2(hDC, R2_NOT);
  if(!points.empty()) {
    pt p = points.front();
    if(is_drawing) {
      Ellipse(hDC, p.x - HORIZ_PT, p.y - VERT_PT, p.x + HORIZ_PT, p.y + VERT_PT);
    }
    MoveToEx(hDC, p.x, p.y, NULL);
    for(unsigned i = 1; i < points.size(); ++i) {
      LineTo(hDC, points[i].x, points[i].y);
    }
    if(!is_drawing) {
      LineTo(hDC, points.front().x, points.front().y);
    }
  }
  ReleaseDC(hWnd, hDC);
  points.clear();
}

void Shield::on_l_button_up(WORD fwKeys, WORD xPos, WORD yPos) {
  HDC hDC = GetDC(hWnd);
  SelectObject(hDC, GetStockObject(NULL_BRUSH));
  SetROP2(hDC, R2_NOT);
  if(is_drawing) {
    pt p = points.back(); // last point is start of edge
    MoveToEx(hDC, p.x, p.y, NULL);
    m->dist_add(xPos - p.x, yPos - p.y);
    p = points.front();
    signed long dx = xPos - p.x;
    signed long dy = yPos - p.y;
    if(-HORIZ_PT <= dx && dx <= HORIZ_PT &&
    -VERT_PT <= dy && dy <= VERT_PT) {
      is_drawing = false;
      Ellipse(hDC, p.x - HORIZ_PT, p.y - VERT_PT, p.x + HORIZ_PT, p.y + VERT_PT);
    } else {
      p.x = xPos;
      p.y = yPos;
    }
    LineTo(hDC, p.x, p.y);
  } else {
    clear_drawing();
    m->area(0); // visual cue that drawing is in progress
    m->dist_reset();
    is_drawing = true;
    Ellipse(hDC, xPos - HORIZ_PT, yPos - VERT_PT, xPos + HORIZ_PT, yPos + VERT_PT);
  };
  ReleaseDC(hWnd, hDC);
  
  if(is_drawing) {
    pt p = {xPos, yPos};
    points.push_back(p); // only add point if it isn't the starting point
  } else {
    // calculate area of polygon.
    double area = 0;
    for(unsigned i = 0; i < points.size() - 1; ++i) {
      area += double(points[i].x) * double(points[i + 1].y);
      area -= double(points[i + 1].x) * double(points[i].y);
    }
    area += double(points.back().x) * double(points.front().y);
    area -= double(points.front().x) * double(points.back().y);
    area /= 2;
    m->area(area);
  }
}
