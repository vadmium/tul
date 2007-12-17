#ifndef INCLUDED_shield_hpp
#define INCLUDED_shield_hpp

#include <windows.h>
#include <vector>

struct Measurer {
  virtual void on_mouse_move(int x, int y) = 0;
  virtual void dist_reset() = 0;
  virtual void dist_add(double x, double y) = 0;
  virtual void area(double area) = 0;
};

struct pt {int x, y;};

class Shield {
public:
  /* stuff for the window class */
  static void reg();
  static void unreg();
  
  static Shield *create(Measurer *m);
  
  HWND getHWnd() const {return hWnd;}; // not always constant?!?!
  
  void show();
  void hide();
  void destroy() {DestroyWindow(hWnd);};

private:
  HWND hWnd;
  Measurer *m;
  double x_res;
  double y_res;
  std::vector< pt > points;
  
  static const int HORIZ_PT = 3;
  static const int VERT_PT = 3;
  bool is_drawing;
  
  virtual ~Shield() {}; // could zero the "this" pointer in GWL_USERDATA

  static LRESULT
    CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
//  virtual LRESULT on_create(LPCREATESTRUCT lpcs);
//  virtual void on_destroy();
  virtual void on_mouse_move(WORD fwKeys, WORD xPos, WORD yPos) {
    m->on_mouse_move(xPos, yPos);
  };
  virtual void on_l_button_up(WORD fwKeys, WORD xPos, WORD yPos);

  void clear_drawing();
  
protected:
  Shield(HWND hWnd, Measurer *m): hWnd(hWnd), m(m) {
    SetWindowLong(hWnd, GWL_USERDATA, reinterpret_cast< LONG >(this));
  };
};

#endif
