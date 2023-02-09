// miniFB.cpp
#include "miniFB.h"

#include <vector>
#include "miniFBinternal.h"
#include "sInfo.h"

short int gKeycodes[512] = { 0 };

//{{{
sWindow* open (const char* title, unsigned width, unsigned height) {

  return openEx (title, width, height, 0);
  }
//}}}
//{{{
eUpdateState update (sWindow* window, void *buffer) {

  if (!window)
    return STATE_INVALID_WINDOW;

  return updateEx (window, buffer, ((sInfo*)(window))->bufferWidth, ((sInfo*)(window))->bufferHeight);
  }
//}}}
//{{{
void close (sWindow* window) {

  if (window)
    ((sInfo*)(window))->closed = true;
  }
//}}}

// gets
//{{{
cStub* cStub::GetInstance (sWindow *window) {

  //{{{
  struct stub_vector {
    std::vector<cStub*> instances;

    stub_vector() = default;
    //{{{
    ~stub_vector() {
      for (cStub* instance : instances)
        delete instance;
      }
    //}}}

    cStub* Get (sWindow *window) {
      for(cStub *instance : instances) {
        if(instance->m_window == window) {
          return instance;
          }
        }
      instances.push_back (new cStub);
      instances.back()->m_window = window;
      return instances.back();
      }
    };
  //}}}
  static stub_vector gInstances;

  return gInstances.Get (window);
  }
//}}}
void* getUserData (sWindow* window) { return window ? ((sInfo*)(window))->userData : 0; }

//{{{
bool isWindowActive (sWindow* window)  {
  return window ? ((sInfo*)(window))->isActive : 0; }
//}}}
//{{{
unsigned getWindowWidth (sWindow* window)  {
  return window ? ((sInfo*)(window))->window_width : 0; }
//}}}
//{{{
unsigned getWindowHeight (sWindow* window) {
  return window ? ((sInfo*)(window))->window_height : 0; }
//}}}

//{{{
int getPointerX (sWindow* window) {
  return window ? ((sInfo*)(window))->pointerPosX : 0; }
//}}}
//{{{
int getPointerY (sWindow* window) {
  return window ? ((sInfo*)(window))->pointerPosY : 0; }
//}}}
//{{{
int getPointerPressure (sWindow* window) {
  return window ? ((sInfo*)(window))->pointerPressure : 0; }
//}}}
//{{{
int64_t getPointerTimestamp (sWindow* window) {
  return window ? ((sInfo*)(window))->timestamp : 0; }
//}}}

//{{{
float getPointerWheelX (sWindow* window) {
  return window ? ((sInfo*)(window))->pointerWheelX : 0; }
//}}}
//{{{
float getPointerWheelY (sWindow* window) {
  return window ? ((sInfo*)(window))->pointerWheelY : 0; }
//}}}

//{{{
const uint8_t* getPointerButtonBuffer (sWindow* window) {
  return window ? ((sInfo*)(window))->pointerButtonStatus : 0; }
//}}}
//{{{
const uint8_t* getKeyBuffer (sWindow* window)  {
  return window ? ((sInfo*)(window))->keyStatus : 0; }
//}}}
//{{{
const char* getKeyName (eKey key) {

  switch (key) {
    case KB_KEY_SPACE: return "Space";
    case KB_KEY_APOSTROPHE: return "Apostrophe";
    case KB_KEY_COMMA: return "Comma";
    case KB_KEY_MINUS: return "Minus";
    case KB_KEY_PERIOD: return "Period";
    case KB_KEY_SLASH: return "Slash";

    //{{{  numbers
    case KB_KEY_0: return "0";
    case KB_KEY_1: return "1";
    case KB_KEY_2: return "2";
    case KB_KEY_3: return "3";
    case KB_KEY_4: return "4";
    case KB_KEY_5: return "5";
    case KB_KEY_6: return "6";
    case KB_KEY_7: return "7";
    case KB_KEY_8: return "8";
    case KB_KEY_9: return "9";
    //}}}

    case KB_KEY_SEMICOLON: return "Semicolon";
    case KB_KEY_EQUAL: return "Equal";

    //{{{  alpha
    case KB_KEY_A: return "A";
    case KB_KEY_B: return "B";
    case KB_KEY_C: return "C";
    case KB_KEY_D: return "D";
    case KB_KEY_E: return "E";
    case KB_KEY_F: return "F";
    case KB_KEY_G: return "G";
    case KB_KEY_H: return "H";
    case KB_KEY_I: return "I";
    case KB_KEY_J: return "J";
    case KB_KEY_K: return "K";
    case KB_KEY_L: return "L";
    case KB_KEY_M: return "M";
    case KB_KEY_N: return "N";
    case KB_KEY_O: return "O";
    case KB_KEY_P: return "P";
    case KB_KEY_Q: return "Q";
    case KB_KEY_R: return "R";
    case KB_KEY_S: return "S";
    case KB_KEY_T: return "T";
    case KB_KEY_U: return "U";
    case KB_KEY_V: return "V";
    case KB_KEY_W: return "W";
    case KB_KEY_X: return "X";
    case KB_KEY_Y: return "Y";
    case KB_KEY_Z: return "Z";
    //}}}
    //{{{  punctuation
    case KB_KEY_LEFT_BRACKET: return "Left_Bracket";
    case KB_KEY_BACKSLASH: return "Backslash";
    case KB_KEY_RIGHT_BRACKET: return "Right_Bracket";
    case KB_KEY_GRAVE_ACCENT: return "Grave_Accent";
    case KB_KEY_WORLD_1: return "World_1";
    case KB_KEY_WORLD_2: return "World_2";
    case KB_KEY_ESCAPE: return "Escape";
    case KB_KEY_ENTER: return "Enter";
    case KB_KEY_TAB: return "Tab";
    case KB_KEY_BACKSPACE: return "Backspace";
    case KB_KEY_INSERT: return "Insert";
    case KB_KEY_DELETE: return "Delete";
    case KB_KEY_RIGHT: return "Right";
    case KB_KEY_LEFT: return "Left";
    case KB_KEY_DOWN: return "Down";
    case KB_KEY_UP: return "Up";
    case KB_KEY_PAGE_UP: return "Page_Up";
    case KB_KEY_PAGE_DOWN: return "Page_Down";
    case KB_KEY_HOME: return "Home";
    case KB_KEY_END: return "End";
    case KB_KEY_CAPS_LOCK: return "Caps_Lock";
    case KB_KEY_SCROLL_LOCK: return "Scroll_Lock";
    case KB_KEY_NUM_LOCK: return "Num_Lock";
    case KB_KEY_PRINT_SCREEN: return "Print_Screen";
    case KB_KEY_PAUSE: return "Pause";
    //}}}
    //{{{  function
    case KB_KEY_F1: return "F1";
    case KB_KEY_F2: return "F2";
    case KB_KEY_F3: return "F3";
    case KB_KEY_F4: return "F4";
    case KB_KEY_F5: return "F5";
    case KB_KEY_F6: return "F6";
    case KB_KEY_F7: return "F7";
    case KB_KEY_F8: return "F8";
    case KB_KEY_F9: return "F9";
    case KB_KEY_F10: return "F10";
    case KB_KEY_F11: return "F11";
    case KB_KEY_F12: return "F12";
    case KB_KEY_F13: return "F13";
    case KB_KEY_F14: return "F14";
    case KB_KEY_F15: return "F15";
    case KB_KEY_F16: return "F16";
    case KB_KEY_F17: return "F17";
    case KB_KEY_F18: return "F18";
    case KB_KEY_F19: return "F19";
    case KB_KEY_F20: return "F20";
    case KB_KEY_F21: return "F21";
    case KB_KEY_F22: return "F22";
    case KB_KEY_F23: return "F23";
    case KB_KEY_F24: return "F24";
    case KB_KEY_F25: return "F25";
    //}}}
    //{{{  numpad
    case KB_KEY_KP_0: return "KP_0";
    case KB_KEY_KP_1: return "KP_1";
    case KB_KEY_KP_2: return "KP_2";
    case KB_KEY_KP_3: return "KP_3";
    case KB_KEY_KP_4: return "KP_4";
    case KB_KEY_KP_5: return "KP_5";
    case KB_KEY_KP_6: return "KP_6";
    case KB_KEY_KP_7: return "KP_7";
    case KB_KEY_KP_8: return "KP_8";
    case KB_KEY_KP_9: return "KP_9";
    case KB_KEY_KP_DECIMAL: return "KP_Decimal";
    case KB_KEY_KP_DIVIDE: return "KP_Divide";
    case KB_KEY_KP_MULTIPLY: return "KP_Multiply";
    case KB_KEY_KP_SUBTRACT: return "KP_Subtract";
    case KB_KEY_KP_ADD: return "KP_Add";
    case KB_KEY_KP_ENTER: return "KP_Enter";
    case KB_KEY_KP_EQUAL: return "KP_Equal";
    //}}}

    case KB_KEY_LEFT_SHIFT: return "Left_Shift";
    case KB_KEY_LEFT_CONTROL: return "Left_Control";
    case KB_KEY_LEFT_ALT: return "Left_Alt";
    case KB_KEY_LEFT_SUPER: return "Left_Super";

    case KB_KEY_RIGHT_CONTROL: return "Right_Control";
    case KB_KEY_RIGHT_SHIFT: return "Right_Shift";
    case KB_KEY_RIGHT_ALT: return "Right_Alt";
    case KB_KEY_RIGHT_SUPER: return "Right_Super";

    case KB_KEY_MENU: return "Menu";

    case KB_KEY_UNKNOWN: return "Unknown";
    }

  return "Unknown";
  }
//}}}

// sets
//{{{
void setUserData (sWindow* window, void* user_data) {

  if (window)
    ((sInfo*)(window))->userData = user_data;
  }
//}}}
//{{{
bool setViewportBestFit (sWindow* window, unsigned old_width, unsigned old_height) {

  if (window) {
    unsigned new_width  = ((sInfo*)(window))->window_width;
    unsigned new_height = ((sInfo*)(window))->window_height;

    float scale_x = new_width  / (float) old_width;
    float scale_y = new_height / (float) old_height;
    if (scale_x >= scale_y)
      scale_x = scale_y;
    else
      scale_y = scale_x;

    unsigned finalWidth  = (unsigned)((old_width  * scale_x) + 0.5f);
    unsigned finalHeight = (unsigned)((old_height * scale_y) + 0.5f);

    unsigned offset_x = (new_width  - finalWidth)  >> 1;
    unsigned offset_y = (new_height - finalHeight) >> 1;

    getMonitorScale (window, &scale_x, &scale_y);
    return setViewport (window, (unsigned)(offset_x / scale_x), (unsigned)(offset_y / scale_y),
                                   (unsigned)(finalWidth / scale_x), (unsigned)(finalHeight / scale_y));
    }

  return false;
  }
//}}}

//{{{
void setActiveCallback (sWindow* window, activeFuncType callback) {

  if (window)
    ((sInfo*)(window))->activeFunc = callback;
  }
//}}}
//{{{
void setResizeCallback (sWindow* window, resizeFuncType callback) {

  if (window)
    ((sInfo*)(window))->resizeFunc = callback;
  }
//}}}
//{{{
void setCloseCallback (sWindow* window, closeFuncType callback) {

  if (window)
    ((sInfo*)(window))->closeFunc = callback;
  }
//}}}

//{{{
void setKeyCallback (sWindow* window, keyFuncType callback) {

  if (window)
    ((sInfo*)(window))->keyFunc = callback;
  }
//}}}
//{{{
void setCharCallback (sWindow* window, charFuncType callback) {

  if (window)
    ((sInfo*)(window))->charFunc = callback;
  }
//}}}

//{{{
void setPointerButtonCallback (sWindow* window, pointerButtonFuncType callback) {

  if (window)
    ((sInfo*)(window))->pointerButtonFunc = callback;
  }
//}}}
//{{{
void setPointerMoveCallback (sWindow* window, pointerMoveFuncType callback) {

  if (window)
    ((sInfo*)(window))->pointerMoveFunc = callback;
  }
//}}}
//{{{
void setPointerWheelCallback (sWindow* window, pointerWheelFuncType callback) {

  if (window)
    ((sInfo*)(window))->pointerWheelFunc = callback;
  }
//}}}
//{{{
void setPointerEnterCallback (sWindow* window, pointerEnterFuncType callback) {

  if (window)
    ((sInfo*)(window))->pointerEnterFunc = callback;
  }
//}}}

//{{{
void keyDefault (sWindow* window, eKey key, eKeyModifier mod, bool isPressed) {

  (void)(mod);
  (void)(isPressed);

  if (key == KB_KEY_ESCAPE) {
    if (!((sInfo*)(window))->closeFunc ||
         ((sInfo*)(window))->closeFunc ((sWindow*)window))
      ((sInfo*)(window))->closed = true;
    }
  }
//}}}

// set callbacks
//{{{
void setActiveCallback (std::function <void (sWindow*, bool)> func, sWindow* window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance (window);
  stub->m_active = std::bind (func, _1, _2);
  setActiveCallback (window, cStub::activeStub);
  }
//}}}
//{{{
void setResizeCallback (std::function <void (sWindow*, int, int)> func, sWindow* window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance(window);
  stub->m_resize = std::bind(func, _1, _2, _3);
  setResizeCallback(window, cStub::resizeStub);
  }
//}}}
//{{{
void setCloseCallback (std::function <bool (sWindow*)> func, sWindow* window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance(window);
  stub->m_close = std::bind(func, _1);
  setCloseCallback(window, cStub::closeStub);
  }
//}}}

//{{{
void setKeyCallback (std::function <void (sWindow*, eKey, eKeyModifier, bool)> func, sWindow *window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance(window);
  stub->m_key = std::bind (func, _1, _2, _3, _4);
  setKeyCallback (window, cStub::keyStub);
  }
//}}}
//{{{
void setCharCallback (std::function <void (sWindow*, unsigned int)> func, sWindow* window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance (window);
  stub->m_char = std::bind (func, _1, _2);
  setCharCallback (window, cStub::charStub);
  }
//}}}

//{{{
void setPointerButtonCallback (std::function <void (sWindow*, ePointerButton, eKeyModifier, bool)> func, sWindow *window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance (window);
  stub->m_pointer_button = std::bind (func, _1, _2, _3, _4);
  setPointerButtonCallback (window, cStub::pointerButtonStub);
  }
//}}}
//{{{
void setPointerMoveCallback (std::function <void (sWindow*, int, int, int, int)> func, sWindow* window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance (window);
  stub->m_pointer_move = std::bind (func, _1, _2, _3, _4, _5);
  setPointerMoveCallback (window, cStub::pointerMoveStub);
  }
//}}}
//{{{
void setPointerWheelCallback (std::function <void (sWindow*, eKeyModifier, float, float)> func, sWindow *window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance (window);
  stub->m_pointer_wheel = std::bind (func, _1, _2, _3, _4);
  setPointerWheelCallback (window, cStub::pointerWheelStub);
  }
//}}}
//{{{
void setPointerEnterCallback (std::function <void (sWindow*, bool)> func, sWindow *window) {

  using namespace std::placeholders;

  cStub* stub = cStub::GetInstance (window);
  stub->mPointerEnter = std::bind (func, _1, _2);
  setPointerEnterCallback (window, cStub::pointerEnterStub);
  }
//}}}

// stubs
//{{{
void cStub::activeStub (sWindow* window, bool isActive) {

  cStub* stub = cStub::GetInstance (window);
  stub->m_active (window, isActive);
  }
//}}}
//{{{
void cStub::resizeStub (sWindow* window, int width, int height) {

  cStub* stub = cStub::GetInstance (window);
  stub->m_resize (window, width, height);
  }
//}}}
//{{{
bool cStub::closeStub (sWindow* window) {

  cStub* stub = cStub::GetInstance (window);
  return stub->m_close (window);
  }
//}}}

//{{{
void cStub::keyStub (sWindow* window, eKey key, eKeyModifier mod, bool isPressed) {

  cStub* stub = cStub::GetInstance (window);
  stub->m_key (window, key, mod, isPressed);
  }
//}}}
//{{{
void cStub::charStub (sWindow* window, unsigned int code) {

  cStub* stub = cStub::GetInstance (window);
  stub->m_char (window, code);
  }
//}}}

//{{{
void cStub::pointerButtonStub (sWindow* window, ePointerButton button, eKeyModifier mod, bool isPressed) {

  cStub* stub = cStub::GetInstance (window);
  stub->m_pointer_button (window, button, mod, isPressed);
  }
//}}}
//{{{
void cStub::pointerMoveStub (sWindow* window, int x, int y, int pressure, int timestamp) {

  cStub* stub = cStub::GetInstance (window);
  stub->m_pointer_move (window, x, y, pressure, timestamp);
  }
//}}}
//{{{
void cStub::pointerWheelStub (sWindow* window, eKeyModifier mod, float deltaX, float deltaY) {

  cStub* stub = cStub::GetInstance (window);
  stub->m_pointer_wheel (window, mod, deltaX, deltaY);
  }
//}}}
//{{{
void cStub::pointerEnterStub (sWindow* window, bool enter) {

  cStub* stub = cStub::GetInstance (window);
  stub->mPointerEnter (window, enter);
  }
//}}}
