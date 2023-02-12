// cMiniFB.h
#pragma once
#include <cstdint>
#include <functional>

#ifdef _WIN32
  #define NOMINMAX
  #include <windows.h>
#else
  #include <X11/Xlib.h>
  #include <GL/glx.h>
#endif

// enums
//{{{
enum eFlags { 
              WF_RESIZABLE          = 0x01,
              WF_FULLSCREEN         = 0x02,
              WF_FULLSCREEN_DESKTOP = 0x04,
              WF_BORDERLESS         = 0x08,
              WF_ALWAYS_ON_TOP      = 0x10 };
//}}}
//{{{
enum eUpdateState {
                    STATE_OK             =  0,
                    STATE_EXIT           = -1,
                    STATE_INVALID_WINDOW = -2,
                    STATE_INVALID_BUFFER = -3,
                    STATE_INTERNAL_ERROR = -4 };
//}}}

//{{{
enum ePointerButton { 
  MOUSE_BTN_0, 
  MOUSE_BTN_1, 
  MOUSE_BTN_2, 
  MOUSE_BTN_3,
  MOUSE_BTN_4, 
  MOUSE_BTN_5, 
  MOUSE_BTN_6, 
  MOUSE_BTN_7 
  };
//}}}
#define MOUSE_LEFT   MOUSE_BTN_1
#define MOUSE_RIGHT  MOUSE_BTN_2
#define MOUSE_MIDDLE MOUSE_BTN_3

//{{{
enum eKeyModifier { 
  KB_MOD_SHIFT     = 0x0001,
  KB_MOD_CONTROL   = 0x0002,
  KB_MOD_ALT       = 0x0004,
  KB_MOD_SUPER     = 0x0008,
  KB_MOD_CAPS_LOCK = 0x0010,
  KB_MOD_NUM_LOCK  = 0x0020 
  };
//}}}
//{{{
enum eKey {
  KB_KEY_UNKNOWN       = -1,

  KB_KEY_SPACE         = 32,
  KB_KEY_APOSTROPHE    = 39,
  KB_KEY_COMMA         = 44,
  KB_KEY_MINUS         = 45,
  KB_KEY_PERIOD        = 46,
  KB_KEY_SLASH         = 47,
  KB_KEY_0             = 48,
  KB_KEY_1             = 49,
  KB_KEY_2             = 50,
  KB_KEY_3             = 51,
  KB_KEY_4             = 52,
  KB_KEY_5             = 53,
  KB_KEY_6             = 54,
  KB_KEY_7             = 55,
  KB_KEY_8             = 56,
  KB_KEY_9             = 57,
  KB_KEY_SEMICOLON     = 59,
  KB_KEY_EQUAL         = 61,
  KB_KEY_A             = 65,
  KB_KEY_B             = 66,
  KB_KEY_C             = 67,
  KB_KEY_D             = 68,
  KB_KEY_E             = 69,
  KB_KEY_F             = 70,
  KB_KEY_G             = 71,
  KB_KEY_H             = 72,
  KB_KEY_I             = 73,
  KB_KEY_J             = 74,
  KB_KEY_K             = 75,
  KB_KEY_L             = 76,
  KB_KEY_M             = 77,
  KB_KEY_N             = 78,
  KB_KEY_O             = 79,
  KB_KEY_P             = 80,
  KB_KEY_Q             = 81,
  KB_KEY_R             = 82,
  KB_KEY_S             = 83,
  KB_KEY_T             = 84,
  KB_KEY_U             = 85,
  KB_KEY_V             = 86,
  KB_KEY_W             = 87,
  KB_KEY_X             = 88,
  KB_KEY_Y             = 89,
  KB_KEY_Z             = 90,
  KB_KEY_LEFT_BRACKET  = 91,
  KB_KEY_BACKSLASH     = 92,
  KB_KEY_RIGHT_BRACKET = 93,
  KB_KEY_GRAVE_ACCENT  = 96,
  KB_KEY_WORLD_1       = 161,
  KB_KEY_WORLD_2       = 162,

  KB_KEY_ESCAPE        = 256,
  KB_KEY_ENTER         = 257,
  KB_KEY_TAB           = 258,
  KB_KEY_BACKSPACE     = 259,
  KB_KEY_INSERT        = 260,
  KB_KEY_DELETE        = 261,
  KB_KEY_RIGHT         = 262,
  KB_KEY_LEFT          = 263,
  KB_KEY_DOWN          = 264,
  KB_KEY_UP            = 265,
  KB_KEY_PAGE_UP       = 266,
  KB_KEY_PAGE_DOWN     = 267,
  KB_KEY_HOME          = 268,
  KB_KEY_END           = 269,
  KB_KEY_CAPS_LOCK     = 280,
  KB_KEY_SCROLL_LOCK   = 281,
  KB_KEY_NUM_LOCK      = 282,
  KB_KEY_PRINT_SCREEN  = 283,
  KB_KEY_PAUSE         = 284,
  KB_KEY_F1            = 290,
  KB_KEY_F2            = 291,
  KB_KEY_F3            = 292,
  KB_KEY_F4            = 293,
  KB_KEY_F5            = 294,
  KB_KEY_F6            = 295,
  KB_KEY_F7            = 296,
  KB_KEY_F8            = 297,
  KB_KEY_F9            = 298,
  KB_KEY_F10           = 299,
  KB_KEY_F11           = 300,
  KB_KEY_F12           = 301,
  KB_KEY_F13           = 302,
  KB_KEY_F14           = 303,
  KB_KEY_F15           = 304,
  KB_KEY_F16           = 305,
  KB_KEY_F17           = 306,
  KB_KEY_F18           = 307,
  KB_KEY_F19           = 308,
  KB_KEY_F20           = 309,
  KB_KEY_F21           = 310,
  KB_KEY_F22           = 311,
  KB_KEY_F23           = 312,
  KB_KEY_F24           = 313,
  KB_KEY_F25           = 314,
  KB_KEY_KP_0          = 320,
  KB_KEY_KP_1          = 321,
  KB_KEY_KP_2          = 322,
  KB_KEY_KP_3          = 323,
  KB_KEY_KP_4          = 324,
  KB_KEY_KP_5          = 325,
  KB_KEY_KP_6          = 326,
  KB_KEY_KP_7          = 327,
  KB_KEY_KP_8          = 328,
  KB_KEY_KP_9          = 329,
  KB_KEY_KP_DECIMAL    = 330,
  KB_KEY_KP_DIVIDE     = 331,
  KB_KEY_KP_MULTIPLY   = 332,
  KB_KEY_KP_SUBTRACT   = 333,
  KB_KEY_KP_ADD        = 334,
  KB_KEY_KP_ENTER      = 335,
  KB_KEY_KP_EQUAL      = 336,
  KB_KEY_LEFT_SHIFT    = 340,
  KB_KEY_LEFT_CONTROL  = 341,
  KB_KEY_LEFT_ALT      = 342,
  KB_KEY_LEFT_SUPER    = 343,
  KB_KEY_RIGHT_SHIFT   = 344,
  KB_KEY_RIGHT_CONTROL = 345,
  KB_KEY_RIGHT_ALT     = 346,
  KB_KEY_RIGHT_SUPER   = 347,
  KB_KEY_MENU          = 348
  };
//}}}
#define KB_KEY_LAST KB_KEY_MENU

//{{{
class cMiniFB {
public:
  // statics
  static cMiniFB* create (const char* title, unsigned width, unsigned height, unsigned flags);
  static const char* getKeyName (eKey key);

  eUpdateState update (void* buffer);
  eUpdateState updateEx (void* buffer, unsigned width, unsigned height);
  eUpdateState updateEvents();
  void close();

  // gets
  bool isWindowActive() const { return isActive; }
  unsigned getWindowWidth() const { return window_width; }
  unsigned getWindowHeight() const { return window_height; }

  int getPointerX() const { return pointerPosX; }
  int getPointerY() const { return pointerPosY; }
  int getPointerPressure() const { return pointerPressure; }
  int64_t getPointerTimestamp() const { return pointerTimestamp; }

  float getPointerWheelX() { return pointerWheelX; }
  float getPointerWheelY() { return pointerWheelY; }

  const uint8_t* getKeyBuffer() { return keyStatus; }
  const uint8_t* getPointerButtonBuffer() const { return pointerButtonStatus; }

  void* getUserData () { return userData; }
  void getMonitorScale (float* scale_x, float* scale_y);

  // sets
  void setUserData (void* user_data);
  bool setViewport (unsigned offset_x, unsigned offset_y, unsigned width, unsigned height);
  bool setViewportBestFit (unsigned old_width, unsigned old_height);

  // C style callbacks
  void setActiveCallback (void(*callback)(cMiniFB* miniFB));
  void setResizeCallback (void(*callback)(cMiniFB* miniFB));
  void setCloseCallback  (bool(*callback)(cMiniFB* miniFB));
  void setKeyCallback    (void(*callback)(cMiniFB* miniFB));
  void setCharCallback   (void(*callback)(cMiniFB* miniFB));
  void setButtonCallback (void(*callback)(cMiniFB* miniFB));
  void setMoveCallback   (void(*callback)(cMiniFB* miniFB));
  void setWheelCallback  (void(*callback)(cMiniFB* miniFB));
  void setEnterCallback  (void(*callback)(cMiniFB* miniFB));

  // func callbacks
  void setActiveFunc (std::function <void (cMiniFB*)> func);
  void setResizeFunc (std::function <void (cMiniFB*)> func);
  void setCloseFunc  (std::function <bool (cMiniFB*)> func);
  void setKeyFunc    (std::function <void (cMiniFB*)> func);
  void setCharFunc   (std::function <void (cMiniFB*)> func);
  void setButtonFunc (std::function <void (cMiniFB*)> func);
  void setMoveFunc   (std::function <void (cMiniFB*)> func);
  void setWheelFunc  (std::function <void (cMiniFB*)> func);
  void setEnterFunc  (std::function <void (cMiniFB*)> func);

  void calcDstFactor (uint32_t width, uint32_t height);
  void resizeDst (uint32_t width, uint32_t height);

  bool createGLcontext();
  void destroyGLcontext();
  void redrawGL (const void* pixels);
  void resizeGL();

  // vars
  void(*activeFunc)(cMiniFB* miniFB);
  void(*resizeFunc)(cMiniFB* miniFB);
  bool(*closeFunc)(cMiniFB* miniFB);
  void(*keyFunc)(cMiniFB* miniFB);
  void(*charFunc)(cMiniFB* miniFB);
  void(*buttonFunc)(cMiniFB* miniFB);
  void(*moveFunc)(cMiniFB* miniFB);
  void(*wheelFunc)(cMiniFB* miniFB);
  void(*enterFunc)(cMiniFB* miniFB);

  uint32_t window_width;
  uint32_t window_height;
  uint32_t windowScaledWidth;
  uint32_t windowScaledHeight;

  uint32_t dst_offset_x;
  uint32_t dst_offset_y;
  uint32_t dst_width;
  uint32_t dst_height;

  float    factor_x;
  float    factor_y;
  float    factor_width;
  float    factor_height;

  void*    draw_buffer;
  uint32_t bufferWidth;
  uint32_t bufferHeight;
  uint32_t bufferStride;

  uint32_t isPressed;
  bool     isActive;
  bool     isInitialized;
  bool     isDown;
  bool     pointerInside;
  bool     closed;

  uint32_t codepoint;
  eKey     keyCode;
  uint8_t  keyStatus[512];
  uint32_t modifierKeys;

  int32_t  pointerTimestamp;
  uint8_t  pointerButtonStatus[8];
  int32_t  pointerPosX;
  int32_t  pointerPosY;
  int32_t  pointerPressure;
  int32_t  pointerTiltX;
  int32_t  pointerTiltY;
  float    pointerWheelX;
  float    pointerWheelY;

  uint32_t textureId;

  #ifdef _WIN32
    HWND     window;
    WNDCLASS wc;
    HDC      hdc;
    HGLRC    hGLRC;
  #else
    Window   window;
    Display* display;
    int      screen;
    GC       gc;
    GLXContext context;
  #endif

private:
  void initGL();

  void* userData;
  };
//}}}
//{{{
class cStub {
public:
  cStub() {}

  // statics
  static cStub* getInstance (cMiniFB* miniFB);

  static void activeStub (cMiniFB* miniFB);
  static void resizeStub (cMiniFB* miniFB);
  static bool closeStub  (cMiniFB* miniFB);
  static void keyStub    (cMiniFB* miniFB);
  static void charStub   (cMiniFB* miniFB);
  static void buttonStub (cMiniFB* miniFB);
  static void moveStub   (cMiniFB* miniFB);
  static void wheelStub  (cMiniFB* miniFB);
  static void enterStub  (cMiniFB* miniFB);

  // vars
  cMiniFB* mMiniFB = nullptr;

  std::function <void (cMiniFB* miniFB)> mActiveFunc;
  std::function <void (cMiniFB* miniFB)> mResizeFunc;
  std::function <bool (cMiniFB* miniFB)> mCloseFunc;
  std::function <void (cMiniFB* miniFB)> mKeyFunc;
  std::function <void (cMiniFB* miniFB)> mCharFunc;
  std::function <void (cMiniFB* miniFB)> mButtonFunc;
  std::function <void (cMiniFB* miniFB)> mMoveFunc;
  std::function <void (cMiniFB* miniFB)> mWheelFunc;
  std::function <void (cMiniFB* miniFB)> mEnterFunc;
  };
//}}}