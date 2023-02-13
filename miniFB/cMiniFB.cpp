// cMiniFB.cpp
//#define USE_WINTAB // enables wintab WT_PACKET, WT_PROXIMITY, but pen WM_POINTER* messages id as mouse not pen
//{{{  includes
#include "cMiniFB.h"
#include <vector>

#ifdef _WIN32
  #include <windowsx.h>
  #include <gl/gl.h>

  #ifdef USE_WINTAB
    #include "winTab.h"
    #define PACKETDATA PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TIME
    #include "pktDef.h"
  #endif
#else
  #include <X11/Xlib.h>
  #include <X11/Xutil.h>
  #include <X11/XKBlib.h>
  #include <X11/keysym.h>
  #include <X11/Xatom.h>
  #include <X11/cursorfont.h>
  #include <xkbcommon/xkbcommon.h>
  #include <X11/extensions/XInput.h>
  #include <X11/extensions/XInput2.h>

  #include <GL/gl.h>
  #include <GL/glx.h>
#endif

#define RGBA 0x1908  // [ Core in gl 1.0, gles1 1.0, gles2 2.0, glsc2 2.0 ]

#include "../common/cLog.h"

using namespace std;
//}}}

namespace {
  #ifdef USE_WINTAB
    //{{{  use wintab
    //{{{
    // Use this enum in conjunction with winTab->Buttons to check for tablet button presses.
    //  e.g. To check for lower pen button press, use:
    //    if (winTab->Buttons & eWinTabButtons_Pen_Lower) {
    //      Lower button is pressed
    enum eWinTabButtons_ {
      eWinTabButtons_Pen_Touch = 1, // Pen touching tablet
      eWinTabButtons_Pen_Lower = 2, // Lower pen button pressed
      eWinTabButtons_Pen_Upper = 4, // Upper pen button pressed
      };
    //}}}
    //{{{  WTfunction addresses
    typedef UINT (WINAPI* WTINFOA) (UINT, UINT, LPVOID);
    typedef HCTX (WINAPI* WTOPENA) (HWND, LPLOGCONTEXTA, BOOL);

    typedef bool (WINAPI* WTGETA) (HCTX, LPLOGCONTEXTA);
    typedef bool (WINAPI* WTSETA) (HCTX, LPLOGCONTEXTA);

    typedef bool (WINAPI* WTCLOSE) (HCTX);
    typedef bool (WINAPI* WTPACKET) (HCTX, UINT, LPVOID);
    typedef bool (WINAPI* WTENABLE) (HCTX, BOOL);
    typedef bool (WINAPI* WTOVERLAP) (HCTX, BOOL);

    typedef bool (WINAPI* WTSAVE) (HCTX, LPVOID);
    typedef bool (WINAPI* WTCONFIG) (HCTX, HWND);
    typedef HCTX (WINAPI* WTRESTORE) (HWND, LPVOID, BOOL);

    typedef bool (WINAPI* WTEXTSET) (HCTX, UINT, LPVOID);
    typedef bool (WINAPI* WTEXTGET) (HCTX, UINT, LPVOID);

    typedef bool (WINAPI* WTQUEUESIZESET) (HCTX, int);
    typedef int  (WINAPI* WTDATAPEEK) (HCTX, UINT, UINT, int, LPVOID, LPINT);
    typedef int  (WINAPI* WTPACKETSGET) (HCTX, int, LPVOID);

    typedef HMGR (WINAPI* WTMGROPEN) (HWND, UINT);
    typedef bool (WINAPI* WTMGRCLOSE) (HMGR);
    typedef HCTX (WINAPI* WTMGRDEFCONTEXT) (HMGR, BOOL);
    typedef HCTX (WINAPI* WTMGRDEFCONTEXTEX) (HMGR, UINT, BOOL);
    //}}}
    //{{{
    struct sWinTabInfo {
      int32_t mPosX;
      int32_t mPosY;
      float mPressure; // Range: 0.0f to 1.0f
      int32_t mButtons;  // Bit field. Use with the eWinTabButtons_ enum.

      DWORD mTime;

      int32_t mRangeX;
      int32_t mRangeY;
      int32_t mMaxPressure;

      HINSTANCE mDll;
      HCTX mContext;

      WTINFOA           mWTInfoA;
      WTOPENA           mWTOpenA;

      WTGETA            mWTGetA;
      WTSETA            mWTSetA;

      WTCLOSE           mWTClose;
      WTPACKET          mWTPacket;
      WTENABLE          mWTEnable;

      WTOVERLAP         mWTOverlap;
      WTSAVE            mWTSave;
      WTCONFIG          mWTConfig;
      WTRESTORE         mWTRestore;

      WTEXTSET          mWTExtSet;
      WTEXTGET          mWTExtGet;

      WTQUEUESIZESET    mWTQueueSizeSet;
      WTDATAPEEK        mWTDataPeek;
      WTPACKETSGET      mWTPacketsGet;

      WTMGROPEN         mWTMgrOpen;
      WTMGRCLOSE        mWTMgrClose;
      WTMGRDEFCONTEXT   mWTMgrDefContext;
      WTMGRDEFCONTEXTEX mWTMgrDefContextEx;
      };
    //}}}

    sWinTabInfo* gWinTab = nullptr;

    //{{{
    bool winTabLoad (HWND window) {

      gWinTab = (sWinTabInfo*)calloc (1, sizeof(sWinTabInfo));
      if (!gWinTab)
        return false;

      // load wintab32.dll, get function addresses
      gWinTab->mDll = LoadLibraryA ("Wintab32.dll");
      if (!gWinTab->mDll) {
        //{{{  error, return
        cLog::log (LOGERROR, fmt::format ("Wintab32.dll not found"));
        return false;
        }
        //}}}

      //{{{  get WT function addresses
      gWinTab->mWTInfoA = (WTINFOA)GetProcAddress (gWinTab->mDll, "WTInfoA");
      gWinTab->mWTOpenA = (WTOPENA)GetProcAddress (gWinTab->mDll, "WTOpenA");

      gWinTab->mWTGetA = (WTGETA)GetProcAddress (gWinTab->mDll, "WTGetA");
      gWinTab->mWTSetA = (WTSETA)GetProcAddress (gWinTab->mDll, "WTSetA");

      gWinTab->mWTClose = (WTCLOSE)GetProcAddress (gWinTab->mDll, "WTClose");
      gWinTab->mWTPacket = (WTPACKET)GetProcAddress (gWinTab->mDll, "WTPacket");
      gWinTab->mWTEnable = (WTENABLE)GetProcAddress (gWinTab->mDll, "WTEnable");
      gWinTab->mWTOverlap = (WTOVERLAP)GetProcAddress (gWinTab->mDll, "WTOverlap");

      gWinTab->mWTSave = (WTSAVE)GetProcAddress (gWinTab->mDll, "WTSave");
      gWinTab->mWTConfig = (WTCONFIG)GetProcAddress (gWinTab->mDll, "WTConfig");
      gWinTab->mWTRestore = (WTRESTORE)GetProcAddress (gWinTab->mDll, "WTRestore");

      gWinTab->mWTExtSet = (WTEXTSET)GetProcAddress (gWinTab->mDll, "WTExtSet");
      gWinTab->mWTExtGet = (WTEXTGET)GetProcAddress (gWinTab->mDll, "WTExtGet");

      gWinTab->mWTQueueSizeSet = (WTQUEUESIZESET)GetProcAddress (gWinTab->mDll, "WTQueueSizeSet");
      gWinTab->mWTDataPeek = (WTDATAPEEK)GetProcAddress (gWinTab->mDll, "WTDataPeek");
      gWinTab->mWTPacketsGet = (WTPACKETSGET)GetProcAddress (gWinTab->mDll, "WTPacketsGet");

      gWinTab->mWTMgrOpen = (WTMGROPEN)GetProcAddress (gWinTab->mDll, "WTMgrOpen");
      gWinTab->mWTMgrClose = (WTMGRCLOSE)GetProcAddress (gWinTab->mDll, "WTMgrClose");

      gWinTab->mWTMgrDefContext = (WTMGRDEFCONTEXT)GetProcAddress (gWinTab->mDll, "WTMgrDefContext");
      gWinTab->mWTMgrDefContextEx = (WTMGRDEFCONTEXTEX)GetProcAddress (gWinTab->mDll, "WTMgrDefContextEx");                \
      //}}}

      if (!gWinTab->mWTInfoA (0, 0, NULL)) {
        //{{{  error, return
        cLog::log (LOGERROR, fmt::format ("winTab services not available"));
        return false;
        }
        //}}}

      LOGCONTEXTA logContext = {0};
      gWinTab->mWTInfoA (WTI_DEFSYSCTX, 0, &logContext);
      logContext.lcPktData = PACKETDATA;
      logContext.lcOptions |= CXO_MESSAGES | CXO_SYSTEM; // | CXO_PEN;
      logContext.lcPktMode = 0;   // absolute mode
      logContext.lcMoveMask = PACKETDATA;
      logContext.lcBtnUpMask = logContext.lcBtnDnMask;

      logContext.lcOutOrgX = 0;
      logContext.lcOutOrgY = 0;
      logContext.lcOutExtX = GetSystemMetrics (SM_CXSCREEN);
      logContext.lcOutExtY = -GetSystemMetrics (SM_CYSCREEN);

      logContext.lcSysOrgX = 0;
      logContext.lcSysOrgY = 0;
      logContext.lcSysExtX = GetSystemMetrics (SM_CXSCREEN);
      logContext.lcSysExtY = GetSystemMetrics (SM_CYSCREEN);

      gWinTab->mContext = gWinTab->mWTOpenA (window, &logContext, TRUE);
      if (!gWinTab->mContext) {
        //{{{  error, return
        cLog::log (LOGERROR, fmt::format ("winTab failed to open"));
        return false;
        }
        //}}}

      AXIS rangeX = {0};
      gWinTab->mWTInfoA (WTI_DEVICES, DVC_X, &rangeX);
      gWinTab->mRangeX = rangeX.axMax;

      AXIS rangeY = {0};
      gWinTab->mWTInfoA (WTI_DEVICES, DVC_Y, &rangeY);
      gWinTab->mRangeY = rangeY.axMax;

      AXIS pressure = {0};
      gWinTab->mWTInfoA (WTI_DEVICES, DVC_NPRESSURE, &pressure);
      gWinTab->mMaxPressure = pressure.axMax;

      return true;
      }
    //}}}
    //{{{
    void winTabUnload() {

      if (gWinTab->mContext)
        gWinTab->mWTClose (gWinTab->mContext);

      if (gWinTab->mDll)
        FreeLibrary (gWinTab->mDll);

      free (gWinTab);

      gWinTab = nullptr;
      }
    //}}}
    //}}}
  #endif

  int16_t gKeycodes[512] = { 0 };
  #ifdef _WIN32
    //{{{  windows
    typedef BOOL(WINAPI* tGlSwapIntervalProc)(int);
    //{{{  dpi
    // Copied (and modified) from Windows Kit 10 to avoid setting _WIN32_WINNT to a higher version
    enum mfb_PROCESS_DPI_AWARENESS {
      mfb_PROCESS_DPI_UNAWARE = 0,
      mfb_PROCESS_SYSTEM_DPI_AWARE = 1,
      mfb_PROCESS_PER_MONITOR_DPI_AWARE = 2
      };

    enum mfb_MONITOR_DPI_TYPE {
      mfb_MDT_EFFECTIVE_DPI = 0,
      mfb_MDT_ANGULAR_DPI   = 1,
      mfb_MDT_RAW_DPI       = 2,
      mfb_MDT_DEFAULT       = mfb_MDT_EFFECTIVE_DPI
      };

    #define mfb_DPI_AWARENESS_CONTEXT_UNAWARE              ((HANDLE) -1)
    #define mfb_DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((HANDLE) -2)
    #define mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((HANDLE) -3)
    #define mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE) -4)
    #define mfb_DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((HANDLE) -5)

    // user32.dll
    typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
    typedef BOOL(WINAPI* PFN_EnableNonClientDpiScaling)(HWND);
    typedef BOOL(WINAPI* PFN_SetProcessDPIAware)(void);
    typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(HANDLE);

    HMODULE gUser32dll = 0;
    PFN_GetDpiForWindow gGetDpiForWindow = 0;
    PFN_EnableNonClientDpiScaling gEnableNonClientDpiScaling = 0;
    PFN_SetProcessDPIAware gSetProcessDPIAware = 0;
    PFN_SetProcessDpiAwarenessContext gSetProcessDpiAwarenessContext = 0;

    // shcore.dll
    typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(mfb_PROCESS_DPI_AWARENESS);
    typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, mfb_MONITOR_DPI_TYPE, UINT *, UINT *);

    HMODULE gShCoreDll = 0;
    PFN_GetDpiForMonitor gGetDpiForMonitor = 0;
    PFN_SetProcessDpiAwareness gSetProcessDpiAwareness = 0;

    //{{{
    // NOT Thread safe. Just convenient (Don't do this at home guys)
    char* getErrorMessage() {

      static char buffer[256];

      buffer[0] = 0;
      FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL,  // Not used with FORMAT_MESSAGE_FROM_SYSTEM
                     GetLastError(),
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     buffer,
                     sizeof(buffer),
                     NULL);
      return buffer;
      }
    //}}}

    //{{{
    void dpiAware() {

      if (gSetProcessDpiAwarenessContext) {
        if (!gSetProcessDpiAwarenessContext (mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
          uint32_t error = GetLastError();
          if (error == ERROR_INVALID_PARAMETER) {
            error = NO_ERROR;
            if (!gSetProcessDpiAwarenessContext (mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
              error = GetLastError();
            }
          if (error != NO_ERROR)
            cLog::log (LOGERROR, fmt::format ("SetProcessDpiAwarenessContext failed {}", getErrorMessage()));
          }
        }

      else if (gSetProcessDpiAwareness) {
        if (gSetProcessDpiAwareness(mfb_PROCESS_PER_MONITOR_DPI_AWARE) != S_OK)
          cLog::log (LOGERROR, fmt::format ("SetProcessDpiAwareness failed {}", getErrorMessage()));
        }

      else if (gSetProcessDPIAware) {
        if (!gSetProcessDPIAware())
          cLog::log (LOGERROR, fmt::format ("SetProcessDPIAware failed {}", getErrorMessage()));
        }

      }
    //}}}
    //{{{
    void getWindowsMonitorScale (HWND hWnd, float* scale_x, float* scale_y) {

      UINT x, y;
      if (gGetDpiForMonitor) {
        HMONITOR monitor = MonitorFromWindow (hWnd, MONITOR_DEFAULTTONEAREST);
        gGetDpiForMonitor (monitor, mfb_MDT_EFFECTIVE_DPI, &x, &y);
        }

      else {
        const HDC dc = GetDC (hWnd);
        x = GetDeviceCaps (dc, LOGPIXELSX);
        y = GetDeviceCaps (dc, LOGPIXELSY);
        ReleaseDC (NULL, dc);
        }

      if (scale_x) {
        *scale_x = x / (float)USER_DEFAULT_SCREEN_DPI; // 96dpi
        if (*scale_x == 0)
          *scale_x = 1;
        }

      if (scale_y) {
        *scale_y = y / (float)USER_DEFAULT_SCREEN_DPI;  // 96dpi
        if (*scale_y == 0)
          *scale_y = 1;
        }
      }
    //}}}

    //{{{
    void loadFunctions() {

      if (!gUser32dll) {
        gUser32dll = LoadLibraryA ("user32.dll");
        if (gUser32dll) {
          gSetProcessDPIAware = (PFN_SetProcessDPIAware)GetProcAddress (gUser32dll, "SetProcessDPIAware");
          gSetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)GetProcAddress (gUser32dll, "SetProcessDpiAwarenessContext");
          gGetDpiForWindow = (PFN_GetDpiForWindow)GetProcAddress (gUser32dll, "GetDpiForWindow");
          gEnableNonClientDpiScaling = (PFN_EnableNonClientDpiScaling)GetProcAddress (gUser32dll, "EnableNonClientDpiScaling");
          }
        }

      if (gShCoreDll) {
        gShCoreDll = LoadLibraryA ("shcore.dll");
        if (!gShCoreDll) {
          gSetProcessDpiAwareness = (PFN_SetProcessDpiAwareness)GetProcAddress (gShCoreDll, "SetProcessDpiAwareness");
          gGetDpiForMonitor = (PFN_GetDpiForMonitor)GetProcAddress (gShCoreDll, "GetDpiForMonitor");
          }
        }
      }
    //}}}
    //}}}

    //{{{
    uint32_t translateMod() {

      uint32_t mods = 0;

      if (GetKeyState (VK_SHIFT) & 0x8000)
        mods |= KB_MOD_SHIFT;

      if (GetKeyState (VK_CONTROL) & 0x8000)
        mods |= KB_MOD_CONTROL;

      if (GetKeyState (VK_MENU) & 0x8000)
        mods |= KB_MOD_ALT;

      if ((GetKeyState (VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
        mods |= KB_MOD_SUPER;

      if (GetKeyState (VK_CAPITAL) & 1)
        mods |= KB_MOD_CAPS_LOCK;

      if (GetKeyState (VK_NUMLOCK) & 1)
        mods |= KB_MOD_NUM_LOCK;

      return mods;
      }
    //}}}
    //{{{
    eMiniKey translateKey (WPARAM wParam, LPARAM lParam) {

      if (wParam == VK_CONTROL) {
        if (lParam & 0x01000000)
          return KB_KEY_RIGHT_CONTROL;

        DWORD time = GetMessageTime();

        MSG next;
        if (PeekMessageW (&next, 0x0, 0, 0, PM_NOREMOVE))
          if (next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN ||
              next.message == WM_KEYUP || next.message == WM_SYSKEYUP)
            if (next.wParam == VK_MENU && (next.lParam & 0x01000000) && next.time == time)
              return KB_KEY_UNKNOWN;

        return KB_KEY_LEFT_CONTROL;
        }

      if (wParam == VK_PROCESSKEY)
        return KB_KEY_UNKNOWN;

      return (eMiniKey)gKeycodes[HIWORD(lParam) & 0x1FF];
      }
    //}}}

    //{{{
    LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

      cMiniFB* miniFB = (cMiniFB*)GetWindowLongPtr (hWnd, GWLP_USERDATA);

      if (!miniFB) {
        // messages before miniFB create
        if (message == WM_NCCREATE)
          if (gEnableNonClientDpiScaling)
            gEnableNonClientDpiScaling (hWnd);
        return DefWindowProc (hWnd, message, wParam, lParam);
        }

      return miniFB->processMessage (hWnd, message, wParam, lParam);
      }
    //}}}
    //}}}
  #else
    //{{{  X11
    typedef void (*tGlSwapIntervalProc)(Display*,GLXDrawable,int);

    Atom gDeleteWindowAtom;

    XDevice* gDevice = nullptr;
    uint32_t gMotionType = 0;
    uint32_t gNumEventClasses = 0;
    XEventClass gEventClasses[16];

    int32_t gRangeX = 0;
    int32_t gRangeY = 0;
    int32_t gMaxPressure = 0;

    //{{{
    int translateKeyCodeB (int keySym) {

      switch (keySym) {
        case XK_KP_0:           return KB_KEY_KP_0;
        case XK_KP_1:           return KB_KEY_KP_1;
        case XK_KP_2:           return KB_KEY_KP_2;
        case XK_KP_3:           return KB_KEY_KP_3;
        case XK_KP_4:           return KB_KEY_KP_4;
        case XK_KP_5:           return KB_KEY_KP_5;
        case XK_KP_6:           return KB_KEY_KP_6;
        case XK_KP_7:           return KB_KEY_KP_7;
        case XK_KP_8:           return KB_KEY_KP_8;
        case XK_KP_9:           return KB_KEY_KP_9;
        case XK_KP_Separator:
        case XK_KP_Decimal:     return KB_KEY_KP_DECIMAL;
        case XK_KP_Equal:       return KB_KEY_KP_EQUAL;
        case XK_KP_Enter:       return KB_KEY_KP_ENTER;
        }

      return KB_KEY_UNKNOWN;
      }
    //}}}
    //{{{
    int translateKeyCodeA (int keySym) {

      switch (keySym) {
        //{{{  control keys
        case XK_Escape:         return KB_KEY_ESCAPE;
        case XK_Tab:            return KB_KEY_TAB;
        case XK_Shift_L:        return KB_KEY_LEFT_SHIFT;
        case XK_Shift_R:        return KB_KEY_RIGHT_SHIFT;
        case XK_Control_L:      return KB_KEY_LEFT_CONTROL;
        case XK_Control_R:      return KB_KEY_RIGHT_CONTROL;

        case XK_Meta_L:
        case XK_Alt_L:          return KB_KEY_LEFT_ALT;
        case XK_Mode_switch:      // Mapped to Alt_R on many keyboards
        case XK_ISO_Level3_Shift: // AltGr on at least some machines
        case XK_Meta_R:
        case XK_Alt_R:          return KB_KEY_RIGHT_ALT;

        case XK_Super_L:        return KB_KEY_LEFT_SUPER;
        case XK_Super_R:        return KB_KEY_RIGHT_SUPER;

        case XK_Menu:           return KB_KEY_MENU;

        case XK_Num_Lock:       return KB_KEY_NUM_LOCK;
        case XK_Caps_Lock:      return KB_KEY_CAPS_LOCK;

        case XK_Print:          return KB_KEY_PRINT_SCREEN;
        case XK_Scroll_Lock:    return KB_KEY_SCROLL_LOCK;
        case XK_Pause:          return KB_KEY_PAUSE;

        case XK_Delete:         return KB_KEY_DELETE;
        case XK_BackSpace:      return KB_KEY_BACKSPACE;
        case XK_Return:         return KB_KEY_ENTER;

        case XK_Home:           return KB_KEY_HOME;
        case XK_End:            return KB_KEY_END;

        case XK_Page_Up:        return KB_KEY_PAGE_UP;
        case XK_Page_Down:      return KB_KEY_PAGE_DOWN;

        case XK_Insert:         return KB_KEY_INSERT;
        case XK_Left:           return KB_KEY_LEFT;
        case XK_Right:          return KB_KEY_RIGHT;
        case XK_Down:           return KB_KEY_DOWN;
        case XK_Up:             return KB_KEY_UP;
        //}}}
        //{{{  function keys
        case XK_F1:             return KB_KEY_F1;
        case XK_F2:             return KB_KEY_F2;
        case XK_F3:             return KB_KEY_F3;
        case XK_F4:             return KB_KEY_F4;
        case XK_F5:             return KB_KEY_F5;
        case XK_F6:             return KB_KEY_F6;
        case XK_F7:             return KB_KEY_F7;
        case XK_F8:             return KB_KEY_F8;
        case XK_F9:             return KB_KEY_F9;

        case XK_F10:            return KB_KEY_F10;
        case XK_F11:            return KB_KEY_F11;
        case XK_F12:            return KB_KEY_F12;
        case XK_F13:            return KB_KEY_F13;
        case XK_F14:            return KB_KEY_F14;
        case XK_F15:            return KB_KEY_F15;
        case XK_F16:            return KB_KEY_F16;
        case XK_F17:            return KB_KEY_F17;
        case XK_F18:            return KB_KEY_F18;
        case XK_F19:            return KB_KEY_F19;

        case XK_F20:            return KB_KEY_F20;
        case XK_F21:            return KB_KEY_F21;
        case XK_F22:            return KB_KEY_F22;
        case XK_F23:            return KB_KEY_F23;
        case XK_F24:            return KB_KEY_F24;
        case XK_F25:            return KB_KEY_F25;
        //}}}
        //{{{  Numeric keypad
        case XK_KP_Divide:      return KB_KEY_KP_DIVIDE;
        case XK_KP_Multiply:    return KB_KEY_KP_MULTIPLY;
        case XK_KP_Subtract:    return KB_KEY_KP_SUBTRACT;
        case XK_KP_Add:         return KB_KEY_KP_ADD;
        //}}}
        //{{{  These should have been detected in secondary keysym test above!
        case XK_KP_Insert:      return KB_KEY_KP_0;
        case XK_KP_End:         return KB_KEY_KP_1;
        case XK_KP_Down:        return KB_KEY_KP_2;
        case XK_KP_Page_Down:   return KB_KEY_KP_3;
        case XK_KP_Left:        return KB_KEY_KP_4;
        case XK_KP_Right:       return KB_KEY_KP_6;
        case XK_KP_Home:        return KB_KEY_KP_7;
        case XK_KP_Up:          return KB_KEY_KP_8;
        case XK_KP_Page_Up:     return KB_KEY_KP_9;
        case XK_KP_Delete:      return KB_KEY_KP_DECIMAL;
        case XK_KP_Equal:       return KB_KEY_KP_EQUAL;
        case XK_KP_Enter:       return KB_KEY_KP_ENTER;
        //}}}
        //{{{  Last resort: Check for printable keys (should not happen if the XK extension is available)
        // This will give a layout dependent mapping (which is wrong, and we may miss some keys
        // especially on non-US keyboards), but it's better than nothing...
        case XK_a:              return KB_KEY_A;
        case XK_b:              return KB_KEY_B;
        case XK_c:              return KB_KEY_C;
        case XK_d:              return KB_KEY_D;
        case XK_e:              return KB_KEY_E;
        case XK_f:              return KB_KEY_F;
        case XK_g:              return KB_KEY_G;
        case XK_h:              return KB_KEY_H;
        case XK_i:              return KB_KEY_I;
        case XK_j:              return KB_KEY_J;
        case XK_k:              return KB_KEY_K;
        case XK_l:              return KB_KEY_L;
        case XK_m:              return KB_KEY_M;
        case XK_n:              return KB_KEY_N;
        case XK_o:              return KB_KEY_O;
        case XK_p:              return KB_KEY_P;
        case XK_q:              return KB_KEY_Q;
        case XK_r:              return KB_KEY_R;
        case XK_s:              return KB_KEY_S;
        case XK_t:              return KB_KEY_T;
        case XK_u:              return KB_KEY_U;
        case XK_v:              return KB_KEY_V;
        case XK_w:              return KB_KEY_W;
        case XK_x:              return KB_KEY_X;
        case XK_y:              return KB_KEY_Y;
        case XK_z:              return KB_KEY_Z;

        case XK_1:              return KB_KEY_1;
        case XK_2:              return KB_KEY_2;
        case XK_3:              return KB_KEY_3;
        case XK_4:              return KB_KEY_4;
        case XK_5:              return KB_KEY_5;
        case XK_6:              return KB_KEY_6;
        case XK_7:              return KB_KEY_7;
        case XK_8:              return KB_KEY_8;
        case XK_9:              return KB_KEY_9;
        case XK_0:              return KB_KEY_0;

        case XK_space:          return KB_KEY_SPACE;
        case XK_minus:          return KB_KEY_MINUS;
        case XK_equal:          return KB_KEY_EQUAL;
        case XK_bracketleft:    return KB_KEY_LEFT_BRACKET;
        case XK_bracketright:   return KB_KEY_RIGHT_BRACKET;
        case XK_backslash:      return KB_KEY_BACKSLASH;
        case XK_semicolon:      return KB_KEY_SEMICOLON;
        case XK_apostrophe:     return KB_KEY_APOSTROPHE;
        case XK_grave:          return KB_KEY_GRAVE_ACCENT;
        case XK_comma:          return KB_KEY_COMMA;
        case XK_period:         return KB_KEY_PERIOD;
        case XK_slash:          return KB_KEY_SLASH;
        case XK_less:           return KB_KEY_WORLD_1; // At least in some layouts...
        //}}}
        default: break;
        }

      return KB_KEY_UNKNOWN;
      }
    //}}}
    //{{{
    int translateKey (int scancode) {

      if (scancode < 0 || scancode > 255)
        return KB_KEY_UNKNOWN;

      return gKeycodes[scancode];
      }
    //}}}

    //{{{
    int translateMod (int state) {

      int modifierKeys = 0;

      if (state & ShiftMask)
        modifierKeys |= KB_MOD_SHIFT;
      if (state & ControlMask)
        modifierKeys |= KB_MOD_CONTROL;
      if (state & Mod1Mask)
        modifierKeys |= KB_MOD_ALT;
      if (state & Mod4Mask)
        modifierKeys |= KB_MOD_SUPER;
      if (state & LockMask)
        modifierKeys |= KB_MOD_CAPS_LOCK;
      if (state & Mod2Mask)
        modifierKeys |= KB_MOD_NUM_LOCK;

      return modifierKeys;
      }
    //}}}
    //{{{
    int translateModEx (int key, int state, int is_pressed) {

      int modifierKeys = translateMod (state);

      switch (key) {
        case KB_KEY_LEFT_SHIFT:
        //{{{
        case KB_KEY_RIGHT_SHIFT:
          if (is_pressed)
            modifierKeys |= KB_MOD_SHIFT;
          else
            modifierKeys &= ~KB_MOD_SHIFT;
          break;
        //}}}

        case KB_KEY_LEFT_CONTROL:
        //{{{
        case KB_KEY_RIGHT_CONTROL:
          if (is_pressed)
            modifierKeys |= KB_MOD_CONTROL;
          else
            modifierKeys &= ~KB_MOD_CONTROL;
          break;
        //}}}

        case KB_KEY_LEFT_ALT:
        //{{{
        case KB_KEY_RIGHT_ALT:
          if (is_pressed)
            modifierKeys |= KB_MOD_ALT;
          else
            modifierKeys &= ~KB_MOD_ALT;
          break;
        //}}}

        case KB_KEY_LEFT_SUPER:
        //{{{
        case KB_KEY_RIGHT_SUPER:
          if (is_pressed)
            modifierKeys |= KB_MOD_SUPER;
          else
            modifierKeys &= ~KB_MOD_SUPER;
          break;
        //}}}
        }

      return modifierKeys;
      }
    //}}}
    //}}}
  #endif

  tGlSwapIntervalProc gSwapInterval = 0;
  //{{{
  bool checkGLExtension (const char* name) {
  // TODO: This is deprecated on OpenGL 3+.
  // Use glGetIntegerv (GL_NUM_EXTENSIONS, &n) and glGetStringi (GL_EXTENSIONS, index)

    #ifdef _WIN32
      const char* extensions = (const char*)glGetString (GL_EXTENSIONS);
    #else
      const char* extensions = glXQueryExtensionsString (glXGetCurrentDisplay(), DefaultScreen(glXGetCurrentDisplay()));
    #endif

    if (!extensions) {
      const char* start = extensions;
      const char* end, *where;
      while(1) {
        where = strstr (start, name);
        if (where == 0x0)
          return false;

        end = where + strlen(name);
        if (where == start || *(where - 1) == ' ')
          if (*end == ' ' || *end == 0)
            break;

        start = end;
        }
      }

    return true;
    }
  //}}}
  }
//{{{
//{{{
cMiniCallbackStub* cMiniCallbackStub::getInstance (cMiniFB* miniFB) {

  //{{{
  struct stub_vector {
    vector<cMiniCallbackStub*> instances;

    stub_vector() = default;
    //{{{
    ~stub_vector() {
      for (cMiniCallbackStub* instance : instances)
        delete instance;
      }
    //}}}

    cMiniCallbackStub* Get (cMiniFB *miniFB) {
      for (cMiniCallbackStub *instance : instances) {
        if( instance->mMiniFB == miniFB) {
          return instance;
          }
        }
      instances.push_back (new cMiniCallbackStub);
      instances.back()->mMiniFB = miniFB;
      return instances.back();
      }
    };
  //}}}
  static stub_vector gInstances;

  return gInstances.Get (miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::activeStub (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mActiveFunc (miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::resizeStub (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mResizeFunc (miniFB);
  }
//}}}
//{{{
bool cMiniCallbackStub::closeStub  (cMiniFB* miniFB) {
  return cMiniCallbackStub::getInstance (miniFB)->mCloseFunc (miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::keyStub    (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mKeyFunc(miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::charStub   (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mCharFunc(miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::buttonStub (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mButtonFunc(miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::moveStub   (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mMoveFunc(miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::wheelStub  (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mWheelFunc(miniFB);
  }
//}}}
//{{{
void cMiniCallbackStub::enterStub  (cMiniFB* miniFB) {
  cMiniCallbackStub::getInstance (miniFB)->mEnterFunc(miniFB);
  }
//}}}
//}}}

// cMiniFB static create
//{{{
cMiniFB* cMiniFB::create (const char* title, uint32_t width, uint32_t height, uint32_t flags) {

  cMiniFB* miniFB = new cMiniFB();
  if (!miniFB) {
    // error, return
    cLog::log (LOGERROR, fmt::format ("failed to create miniFB"));
    return nullptr;
    }

  return miniFB->init (title, width, height, flags) ? miniFB : nullptr;
  }
//}}}

// cMiniFB
//{{{
eMiniState cMiniFB::update (void* buffer) {

  if (closed) {
    freeResources();
    return STATE_EXIT;
    }

  if (!buffer)
    return STATE_INVALID_BUFFER;

  drawBuffer = buffer;
  redrawGL (buffer);

  return STATE_OK;
  }
//}}}
//{{{
eMiniState cMiniFB::updateEvents() {

  if (closed) {
    freeResources();
    return STATE_EXIT;
    }

  #ifdef _WIN32
    MSG msg;
    while (!closed && PeekMessage (&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
      }
  #else
    XFlush (display);
    while (!closed && XPending (display)) {
      XEvent event;
      XNextEvent (display, &event);
      processEvent (&event);
      }
  #endif

  return STATE_OK;
  }
//}}}
//{{{
void cMiniFB::close() {
  closed = true;
  }
//}}}

// static get
//{{{
const char* cMiniFB::getKeyName (eMiniKey key) {

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

// gets
//{{{
void cMiniFB::getMonitorScale (float* scale_x, float* scale_y) {

  #ifdef _WIN32
    getWindowsMonitorScale (window, scale_x, scale_y);
  #else
    float x = 96.0;
    float y = 96.0;

    if (scale_x) {
      *scale_x = x / 96.0f;
      if (*scale_x == 0)
        *scale_x = 1.0f;
      }

    if (scale_y) {
      *scale_y = y / 96.0f;
      if (*scale_y == 0)
        *scale_y = 1.0f;
      }
  #endif
  }
//}}}

// sets
//{{{
bool cMiniFB::setViewportBestFit (uint32_t oldWidth, uint32_t oldHeight) {

  uint32_t newWidth  = windowWidth;
  uint32_t newHeight = windowHeight;

  float scale_x = newWidth  / (float) oldWidth;
  float scale_y = newHeight / (float) oldHeight;
  if (scale_x >= scale_y)
    scale_x = scale_y;
  else
    scale_y = scale_x;

  uint32_t finalWidth  = (uint32_t)((oldWidth  * scale_x) + 0.5f);
  uint32_t finalHeight = (uint32_t)((oldHeight * scale_y) + 0.5f);

  uint32_t offset_x = (newWidth  - finalWidth)  >> 1;
  uint32_t offset_y = (newHeight - finalHeight) >> 1;

  getMonitorScale (&scale_x, &scale_y);
  return setViewport ((uint32_t)(offset_x / scale_x), (uint32_t)(offset_y / scale_y),
                      (uint32_t)(finalWidth / scale_x), (uint32_t)(finalHeight / scale_y));
  }
//}}}
//{{{
bool cMiniFB::setViewport (uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height) {

  if (offset_x + width > windowWidth)
    return false;
  if (offset_y + height > windowHeight)
    return false;

  #ifdef _WIN32
    float scale_x, scale_y;
    getWindowsMonitorScale (window, &scale_x, &scale_y);
    dstOffsetX = (uint32_t)(offset_x * scale_x);
    dstOffsetY = (uint32_t)(offset_y * scale_y);
    dstWidth = (uint32_t)(width  * scale_x);
    dstHeight = (uint32_t)(height * scale_y);
  #else
    if (offset_x + width > windowWidth)
      return false;
    if (offset_y + height > windowHeight)
      return false;
    dstOffsetX = offset_x;
    dstOffsetY = offset_y;
    dstWidth = width;
    dstHeight = height;
  #endif

  calcDstFactor (windowWidth, windowHeight);
  return true;
  }
//}}}
//{{{  set C style callbacks
void cMiniFB::setActiveCallback (void(*callback)(cMiniFB* miniFB)) { activeFunc = callback; }
void cMiniFB::setResizeCallback (void(*callback)(cMiniFB* miniFB)) { resizeFunc = callback; }
void cMiniFB::setCloseCallback  (bool(*callback)(cMiniFB* miniFB)) { closeFunc = callback; }
void cMiniFB::setKeyCallback    (void(*callback)(cMiniFB* miniFB)) { keyFunc = callback; }
void cMiniFB::setCharCallback   (void(*callback)(cMiniFB* miniFB)) { charFunc = callback; }
void cMiniFB::setButtonCallback (void(*callback)(cMiniFB* miniFB)) { buttonFunc = callback; }
void cMiniFB::setMoveCallback   (void(*callback)(cMiniFB* miniFB)) { moveFunc = callback; }
void cMiniFB::setWheelCallback  (void(*callback)(cMiniFB* miniFB)) { wheelFunc = callback; }
void cMiniFB::setEnterCallback  (void(*callback)(cMiniFB* miniFB)) { enterFunc = callback; }
//}}}
//{{{  set function style callbacks
//{{{
void cMiniFB::setActiveFunc (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mActiveFunc = bind (func, placeholders::_1);
  setActiveCallback (cMiniCallbackStub::activeStub);
  }
//}}}
//{{{
void cMiniFB::setResizeFunc (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mResizeFunc = bind(func, placeholders::_1);
  setResizeCallback (cMiniCallbackStub::resizeStub);
  }
//}}}
//{{{
void cMiniFB::setCloseFunc  (function <bool (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mCloseFunc = bind(func, placeholders::_1);
  setCloseCallback (cMiniCallbackStub::closeStub);
  }
//}}}
//{{{
void cMiniFB::setKeyFunc    (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mKeyFunc = bind (func, placeholders::_1);
  setKeyCallback (cMiniCallbackStub::keyStub);
  }
//}}}
//{{{
void cMiniFB::setCharFunc   (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mCharFunc = bind (func, placeholders::_1);
  setCharCallback (cMiniCallbackStub::charStub);
  }
//}}}
//{{{
void cMiniFB::setButtonFunc (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mButtonFunc = bind (func, placeholders::_1);
  setButtonCallback (cMiniCallbackStub::buttonStub);
  }
//}}}
//{{{
void cMiniFB::setMoveFunc   (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mMoveFunc = bind (func, placeholders::_1);
  setMoveCallback (cMiniCallbackStub::moveStub);
  }
//}}}
//{{{
void cMiniFB::setWheelFunc  (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mWheelFunc = bind (func, placeholders::_1);
  setWheelCallback (cMiniCallbackStub::wheelStub);
  }
//}}}
//{{{
void cMiniFB::setEnterFunc  (function <void (cMiniFB*)> func) {
  cMiniCallbackStub::getInstance (this)->mEnterFunc = bind (func, placeholders::_1);
  setEnterCallback (cMiniCallbackStub::enterStub);
  }
//}}}
//}}}

// message handler
#ifdef _WIN32
  //{{{
  LRESULT CALLBACK cMiniFB::processMessage (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message) {
      //{{{
      case WM_SIZE: {
        if (wParam == SIZE_MINIMIZED)
          return 0;

        float scale_x, scale_y;
        getWindowsMonitorScale (hWnd, &scale_x, &scale_y);
        windowWidth = GET_X_LPARAM(lParam);
        windowHeight =  GET_Y_LPARAM(lParam);
        resizeDst (windowWidth, windowHeight);

        resizeGL();
        if (windowWidth && windowHeight) {
          windowScaledWidth  = (uint32_t)(windowWidth  / scale_x);
          windowScaledHeight = (uint32_t)(windowHeight / scale_y);
          if (resizeFunc)
            resizeFunc (this);
          }
        break;
        }
      //}}}

      //{{{
      case WM_CLOSE:
        // Obtain a confirmation of close
        if (!closeFunc || closeFunc (this)) {
          closed = true;
          DestroyWindow (window);
          }
        break;
      //}}}
      //{{{
      case WM_DESTROY:
        closed = true;
        break;
      //}}}

      //{{{
      case WM_SETFOCUS:
         isActive = true;
        if (activeFunc)
          activeFunc (this);
        break;
      //}}}
      //{{{
      case WM_KILLFOCUS:
        isActive = false;
        if (activeFunc)
          activeFunc (this);
        break;
      //}}}

      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      //{{{
      case WM_KEYUP: {
        keyCode = translateKey (wParam, lParam);
        isPressed = !((lParam >> 31) & 1);
        modifierKeys = translateMod();

        if (keyCode == KB_KEY_UNKNOWN)
          return 0;

        keyStatus[keyCode] = (uint8_t)isPressed;
        if (keyFunc)
          keyFunc (this);
        break;
        }
      //}}}

      case WM_CHAR:
      //{{{
      case WM_SYSCHAR:
        {
        static WCHAR highSurrogate = 0;

        if (wParam >= 0xd800 && wParam <= 0xdbff)
          highSurrogate = (WCHAR) wParam;
        else {
          codepoint = 0;
          if (wParam >= 0xdc00 && wParam <= 0xdfff) {
            if (highSurrogate != 0) {
              codepoint += (highSurrogate - 0xd800) << 10;
              codepoint += (WCHAR) wParam - 0xdc00;
              codepoint += 0x10000;
              }
            }
          else
            codepoint = (WCHAR) wParam;

          highSurrogate = 0;
          if (charFunc)
            charFunc (this);
          }
        }
        break;
      //}}}

      #ifdef USE_WINTAB
        //{{{
        case WT_PACKET:
          if ((HCTX)lParam == gWinTab->mContext) {
            PACKET packet = {0};
            if (gWinTab->mWTPacket (gWinTab->mContext, (UINT)wParam, &packet)) {
              POINT point = { 0 };
              point.x = packet.pkX;
              point.y = packet.pkY;
              ScreenToClient (hWnd, &point);

              gWinTab->mTime = packet.pkTime;
              gWinTab->mPosX = point.x;
              gWinTab->mPosY = point.y;
              gWinTab->mPressure = (float)packet.pkNormalPressure / (float)gWinTab->mMaxPressure;
              gWinTab->mButtons = packet.pkButtons;
              cLog::log (LOGINFO, fmt::format ("WT_PACKET press:{} time:{}", gWinTab->mPressure, gWinTab->mTime));
              }
            else
              cLog::log (LOGERROR, fmt::format ("WT_PACKET no packet"));
            }
          else
            cLog::log (LOGERROR, fmt::format ("WT_PACKET wrong context"));

          break;
        //}}}
        //{{{
        case WT_PROXIMITY:
          if (lParam & 0xFFFF)
            cLog::log (LOGINFO, fmt::format ("WT_PROXIMITY in {:x}", lParam));
          else
            cLog::log (LOGINFO, fmt::format ("WT_PROXIMITY out {:x}", lParam));

          break;
        //}}}
      #endif

      //{{{  POINTER_INFO
      //POINTER_INPUT_TYPE         pointerType;
      //UINT32                     pointerId;
      //UINT32                     frameId;
      //POINTER_FLAGS              pointerFlags;
      //HANDLE                     sourceDevice;
      //HWND                       hwndTarget;
      //POINT                      ptPixelLocation;
      //POINT                      ptHimetricLocation;
      //POINT                      ptPixelLocationRaw;
      //POINT                      ptHimetricLocationRaw;
      //DWORD                      dwTime;
      //UINT32                     historyCount;
      //INT32                      InputData;
      //DWORD                      dwKeyStates;
      //UINT64                     PerformanceCount;
      //POINTER_BUTTON_CHANGE_TYPE ButtonChangeType;
      //}}}
      //{{{  POINTER_INPUT
        //PT_POINTER = 1,
        //PT_TOUCH = 2,
        //PT_PEN = 3,
        //PT_MOUSE = 4,
        //PT_TOUCHPAD = 5
      //}}}
      //{{{  POINTER_FLAG
      //POINTER_FLAG_NONE 0x00000000
      //POINTER_FLAG_NEW 0x00000001
      //POINTER_FLAG_INRANGE 0x00000002
      //POINTER_FLAG_INCONTACT 0x00000004
      //POINTER_FLAG_FIRSTBUTTON 0x00000010
      //POINTER_FLAG_SECONDBUTTON 0x00000020
      //POINTER_FLAG_THIRDBUTTON 0x00000040
      //POINTER_FLAG_FOURTHBUTTON 0x00000080
      //POINTER_FLAG_FIFTHBUTTON 0x00000100
      //POINTER_FLAG_PRIMARY 0x00002000
      //POINTER_FLAG_CONFIDENCE 0x000004000
      //POINTER_FLAG_CANCELED 0x000008000
      //POINTER_FLAG_DOWN 0x00010000
      //POINTER_FLAG_UPDATE 0x00020000
      //POINTER_FLAG_UP 0x00040000
      //POINTER_FLAG_WHEEL 0x00080000
      //POINTER_FLAG_HWHEEL 0x00100000
      //POINTER_FLAG_CAPTURECHANGED 0x00200000
      //POINTER_FLAG_HASTRANSFORM 0x00400000
      //}}}
      //{{{  POINTER_BUTTON_CHANGE_TYPE {
        //POINTER_CHANGE_NONE,
        //POINTER_CHANGE_FIRSTBUTTON_DOWN,
        //POINTER_CHANGE_FIRSTBUTTON_UP,
        //POINTER_CHANGE_SECONDBUTTON_DOWN,
        //POINTER_CHANGE_SECONDBUTTON_UP,
        //POINTER_CHANGE_THIRDBUTTON_DOWN,
        //POINTER_CHANGE_THIRDBUTTON_UP,
        //POINTER_CHANGE_FOURTHBUTTON_DOWN,
        //POINTER_CHANGE_FOURTHBUTTON_UP,
        //POINTER_CHANGE_FIFTHBUTTON_DOWN,
        //POINTER_CHANGE_FIFTHBUTTON_UP
      //}}}
      //{{{  POINTER_PEN_INFO
        //POINTER_INFO pointerInfo;
        //PEN_FLAGS    penFlags;
        //PEN_MASK     penMask;
        //UINT32       pressure;
        //UINT32       rotation;
        //INT32        tiltX;
        //INT32        tiltY;
      //}}}
      //{{{
      case WM_POINTERENTER:
        pointerInside = true;
        if (enterFunc)
          enterFunc (this);
        break;
      //}}}
      //{{{
      case WM_POINTERLEAVE:
        pointerInside = false;
        if (enterFunc)
          enterFunc (this);
        break;
      //}}}
      //{{{
      case WM_POINTERDOWN: {
        POINTER_INFO pointerInfo;
        if (GetPointerInfo (GET_POINTERID_WPARAM (wParam), &pointerInfo)) {
          if (pointerInfo.pointerType == PT_MOUSE) {
            cLog::log (LOGINFO, fmt::format ("WM_POINTERDOWN mouseDown"));
            }
          else if (pointerInfo.pointerType == PT_PEN) {
            cLog::log (LOGINFO, fmt::format ("WM_POINTERDOWN penDown"));
            }
          else // unused PT_TOUCH, PT_TOUCHPAD
            cLog::log (LOGERROR, fmt::format ("WM_POINTERDOWN - unknown type:{}", pointerInfo.pointerType));

          pointerButtonStatus[MOUSE_BTN_1] = 1;
          modifierKeys = translateMod();
          isDown = 1;
          if (buttonFunc)
            buttonFunc (this);
          }
        else
          cLog::log (LOGERROR, fmt::format ("WM_POINTERDOWN - no miniFB"));
        break;
        }
      //}}}
      //{{{
      case WM_POINTERUP: {
        POINTER_INFO pointerInfo;
        if (GetPointerInfo (GET_POINTERID_WPARAM (wParam), &pointerInfo)) {
          if (pointerInfo.pointerType == PT_MOUSE) {
            cLog::log (LOGINFO, fmt::format ("WM_POINTERUP mouseUp"));
            }
          else if (pointerInfo.pointerType == PT_PEN) {
            cLog::log (LOGINFO, fmt::format ("WM_POINTERUP penUp"));
            }
          else // unused PT_TOUCH, PT_TOUCHPAD
            cLog::log (LOGERROR, fmt::format ("WM_POINTERUP - unknown type:{}", pointerInfo.pointerType));

          modifierKeys = translateMod();
          pointerButtonStatus[MOUSE_BTN_1] = 0;
          isDown = 0;
          if (buttonFunc)
            buttonFunc (this);
          }
        else
          cLog::log (LOGERROR, fmt::format ("WM_POINTERUP - no info"));
        break;
        }
      //}}}
      //{{{
      case WM_POINTERUPDATE: {
        POINTER_INFO pointerInfo;
        if (GetPointerInfo (GET_POINTERID_WPARAM (wParam), &pointerInfo)) {
          if (pointerInfo.pointerType == PT_MOUSE) {
            #ifdef USE_WINTAB
              cLog::log (LOGINFO, fmt::format ("WM_POINTERUPDATE mouse @{} flag:{:x}", pointerInfo.dwTime, pointerInfo.pointerFlags));
            #endif
            pointerInside = true;
            pointerTimestamp = pointerInfo.dwTime;
            POINT clientPos = pointerInfo.ptPixelLocation;
            ScreenToClient (hWnd, &clientPos);
            pointerPosX = clientPos.x;
            pointerPosY = clientPos.y;
            pointerPressure = pointerButtonStatus[MOUSE_BTN_1] * 1024;
            if (moveFunc)
              moveFunc (this);
            }
          else if (pointerInfo.pointerType == PT_PEN) {
            POINTER_PEN_INFO pointerPenInfos[10];
            uint32_t entriesCount = 10;
            if (GetPointerPenInfoHistory (GET_POINTERID_WPARAM (wParam), &entriesCount, pointerPenInfos)) {
              pointerInside = true;
              for (uint32_t i = entriesCount; i > 0; i--) {
                cLog::log (LOGINFO, fmt::format ("WM_POINTERUPDATE pen i:{} @{}", i, pointerPenInfos[i-1].pointerInfo.dwTime));
                pointerTimestamp = pointerPenInfos[i-1].pointerInfo.dwTime;
                ScreenToClient (hWnd, &pointerPenInfos[i-1].pointerInfo.ptPixelLocation);
                pointerPosX = pointerPenInfos[i-1].pointerInfo.ptPixelLocation.x;
                pointerPosY = pointerPenInfos[i-1].pointerInfo.ptPixelLocation.y;
                pointerPressure = pointerPenInfos[i-1].pressure;
                pointerTiltX = 0;
                pointerTiltY = 0;
                if (moveFunc)
                  moveFunc (this);
                }
              }
            }
          else
            cLog::log (LOGERROR, fmt::format ("WM_POINTERUPDATE - unknown type:{}", pointerInfo.pointerType));
          }
        else
          cLog::log (LOGERROR, fmt::format ("WM_POINTERUPDATE - no info"));
        break;
        }
      //}}}
      //{{{
      case WM_POINTERWHEEL:
        cLog::log (LOGINFO, fmt::format ("pointerWheel"));
        pointerWheelX = 0;
        pointerWheelY = (SHORT)HIWORD(wParam) / (float)WHEEL_DELTA;
        if (wheelFunc)
          wheelFunc (this);
        break;
      //}}}

      //{{{
      //case 0x02E4://WM_GETDPISCALEDSIZE: {
      //  SIZE* size = (SIZE*)lParam;
      //  WORD dpi = (wParam);
      //  return true;
      //  break;
      //  }
      //}}}
      //{{{
      //case WM_DPICHANGED: //
      //  const float xscale = HIWORD(wParam);
      //  const float yscale = LOWORD(wParam);
      //  break;
      //}}}
      case WM_POINTERACTIVATE:
      case WM_POINTERCAPTURECHANGED:
      case WM_POINTERDEVICECHANGE:
      case WM_POINTERDEVICEINRANGE:
      case WM_POINTERDEVICEOUTOFRANGE:
      case WM_TOUCHHITTESTING:
      case WM_INPUT:
      case WM_NCPOINTERUPDATE:
      case WM_NCPOINTERUP:
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_XBUTTONUP:
      case WM_LBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONDBLCLK:
      case WM_MOUSEMOVE:
      case WM_MOUSELEAVE:
      case WM_MOUSEWHEEL:
      case WM_MOUSEHWHEEL:
      default:
        return DefWindowProc (hWnd, message, wParam, lParam);
      }

    return 0;
    }
  //}}}
#else
  //{{{
  void cMiniFB::processEvent (XEvent* event) {

    switch (event->type) {
      case KeyPress:
      //{{{
      case KeyRelease:
        keyCode = (eMiniKey)translateKey (event->xkey.keycode);
        isDown = (event->type == KeyPress);
        modifierKeys = translateModEx (keyCode, event->xkey.state, isDown);
        keyStatus[keyCode] = isDown;
        if (keyFunc)
          keyFunc (this);

        if (isDown) {
          KeySym keysym;
          XLookupString (&event->xkey, NULL, 0, &keysym, NULL);
          if ((keysym >= 0x0020 && keysym <= 0x007e) ||
              (keysym >= 0x00a0 && keysym <= 0x00ff)) {
            codepoint = keysym;
            if (charFunc)
              charFunc (this);
            }
          else if ((keysym & 0xff000000) == 0x01000000)
            keysym = keysym & 0x00ffffff;

          codepoint = keysym;
          if (charFunc)
            charFunc (this);
          // This does not seem to be working properly
          // unsigned int codepoint = xkb_state_key_get_utf32(state, keysym);
          // if (codepoint != 0)
          //    kCall(char_inputFunc, codepoint);
          }

        break;
      //}}}

      case ButtonPress:
      //{{{
      case ButtonRelease:
        {
        isDown = (event->type == ButtonPress);
        modifierKeys = translateMod (event->xkey.state);

        // swap 2 & 3 ?
        eMiniPointerButton button = (eMiniPointerButton)event->xbutton.button;
        switch (button) {
          case Button1:
          case Button3:
            pointerButtonStatus[button & 0x07] = isDown;
            if (buttonFunc)
              buttonFunc (this);
            break;

          // wheel
          case Button4:
            pointerWheelY = 1.0f;
            if (wheelFunc)
              wheelFunc (this);
            break;

          case Button5:
            pointerWheelY = -1.0f;
            if (wheelFunc)
              wheelFunc (this);
            break;

          case 6:
            pointerWheelX = 1.0f;
            if (wheelFunc)
              wheelFunc (this);
            break;

          case 7:
            pointerWheelX = -1.0f;
            if (wheelFunc)
              wheelFunc (this);
            break;

          default:
            break;
          }
        }

        break;
      //}}}

      //{{{
      case MotionNotify: {
        XDeviceMotionEvent* motionEvent = (XDeviceMotionEvent*)(event);
        cLog::log (LOGINFO, fmt::format ("motionNotify {},{} {}",
                                         event->xmotion.x, event->xmotion.y, motionEvent->serial));
        pointerTimestamp = 0;
        pointerPosX = event->xmotion.x;
        pointerPosY = event->xmotion.y;
        pointerPressure = pointerButtonStatus[Button1] * 1024;
        if (moveFunc)
          moveFunc (this);
        }
        break;
      //}}}

      //{{{
      case ConfigureNotify:
        windowWidth  = event->xconfigure.width;
        windowHeight = event->xconfigure.height;
        windowScaledWidth  = windowWidth;
        windowScaledHeight = windowHeight;

        resizeDst (event->xconfigure.width, event->xconfigure.height);
        resizeGL();

        if (resizeFunc)
          resizeFunc (this);

        break;
      //}}}

      //{{{
      case EnterNotify:
        pointerInside = true;
        if (activeFunc)
          activeFunc (this);

        break;
      //}}}
      //{{{
      case LeaveNotify:
        pointerInside = false;
        if (activeFunc)
          activeFunc (this);

        break;
      //}}}

      //{{{
      case FocusIn:
        isActive = true;
        if (activeFunc)
          activeFunc (this);

        break;
      //}}}
      //{{{
      case FocusOut:
        isActive = false;
        if (activeFunc)
          activeFunc (this);

        break;
      //}}}

      //{{{
      case DestroyNotify:
        closed = true;
        return;
      //}}}
      //{{{
      case ClientMessage:
        if ((Atom)event->xclient.data.l[0] == gDeleteWindowAtom) {
          bool destroy = false;
          if (!closeFunc || closeFunc (this))
            destroy = true;
          if (destroy) {
            closed = true;
            return;
            }
          }

        break;
      //}}}

      default:
        if (event->type == (int)gMotionType) {
          XDeviceMotionEvent* motionEvent = (XDeviceMotionEvent*)(event);
          int posX = motionEvent->x;
          int posY = motionEvent->y;
          //int posX = motionEvent->axis_data[0];
          //int posY = motionEvent->axis_data[1];
          int pressure = motionEvent->axis_data[2];
          cLog::log (LOGINFO, fmt::format ("tablet motionEvent {},{} {} {}",
                                           posX, posY, pressure, motionEvent->serial, event->type));
          }
        else
          cLog::log (LOGINFO, fmt::format ("unused event {}", event->type));
        break;
      }
    }
  //}}}
#endif

// private
//{{{
bool cMiniFB::init (const char* title, uint32_t width, uint32_t height, uint32_t flags) {

  #ifdef _WIN32
    //{{{  windows
    loadFunctions();
    dpiAware();
    initKeycodes();

    bufferWidth  = width;
    bufferHeight = height;
    bufferStride = width * 4;

    int x = 0;
    int y = 0;
    RECT rect = { 0 };
    long windowStyle = WS_POPUP | WS_SYSMENU | WS_CAPTION;
    windowStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
    if (flags & WF_FULLSCREEN) {
      //{{{  fullscreen
      flags = WF_FULLSCREEN;  // Remove all other flags
      rect.right  = GetSystemMetrics (SM_CXSCREEN);
      rect.bottom = GetSystemMetrics (SM_CYSCREEN);
      windowStyle = WS_POPUP & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

      DEVMODE settings = { 0 };
      EnumDisplaySettings (0, 0, &settings);
      settings.dmPelsWidth  = GetSystemMetrics(SM_CXSCREEN);
      settings.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN);
      settings.dmBitsPerPel = 32;
      settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

      if (ChangeDisplaySettings (&settings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        flags = WF_FULLSCREEN_DESKTOP;
      }
      //}}}
    if (flags & WF_BORDERLESS)
      windowStyle = WS_POPUP;
    if (flags & WF_RESIZABLE)
      windowStyle |= WS_MAXIMIZEBOX | WS_SIZEBOX;
    if (flags & WF_FULLSCREEN_DESKTOP) {
      //{{{  desktop
      windowStyle = WS_OVERLAPPEDWINDOW;

      width  = GetSystemMetrics (SM_CXFULLSCREEN);
      height = GetSystemMetrics (SM_CYFULLSCREEN);

      rect.right  = width;
      rect.bottom = height;
      AdjustWindowRect (&rect, windowStyle, 0);
      if (rect.left < 0) {
        width += rect.left * 2;
        rect.right += rect.left;
        rect.left = 0;
        }
      if (rect.bottom > (LONG) height) {
        height -= (rect.bottom - height);
        rect.bottom += (rect.bottom - height);
        rect.top = 0;
        }
      }
      //}}}
    else if (!(flags & WF_FULLSCREEN)) {
      //{{{  desktop fullscreen
      float scale_x, scale_y;
      getMonitorScale (&scale_x, &scale_y);

      rect.right  = (LONG) (width  * scale_x);
      rect.bottom = (LONG) (height * scale_y);
      AdjustWindowRect(&rect, windowStyle, 0);

      rect.right  -= rect.left;
      rect.bottom -= rect.top;
      x = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
      y = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom + rect.top) / 2;
      }
      //}}}

    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.lpszClassName = title;
    RegisterClass (&wc);

    calcDstFactor (width, height);
    windowWidth  = rect.right;
    windowHeight = rect.bottom;
    window = CreateWindowEx (0, title, title, windowStyle, x, y, windowWidth, windowHeight, 0, 0, 0, 0);
    if (!window) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("failed to create X11 window"));
      return false;
      }
      //}}}

    SetWindowLongPtr (window, GWLP_USERDATA, (LONG_PTR)this);
    if (flags & WF_ALWAYS_ON_TOP)
      SetWindowPos (window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow (window, SW_NORMAL);

    hdc = GetDC (window);
    createGLcontext();
    cLog::log (LOGINFO, "using windows OpenGL");

    #ifdef USE_WINTAB
      // enable winTab, mainly for WT_PROXIMITY, get WT_PACKET
      if (!winTabLoad (window))
        cLog::log (LOGERROR, fmt::format ("winTab load failed"));
    #endif

    // enable WM_POINTER wndProc messaging, to tell mouse from pen
    EnableMouseInPointer (true);
    //}}}
  #else
    //{{{  X11
    display = XOpenDisplay (0);
    if (!display) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("failed to create X11 display"));
      return false;
      }
      //}}}
    initKeycodes();
    XAutoRepeatOff (display);

    screen = DefaultScreen (display);
    Visual* visual = DefaultVisual (display, screen);
    //{{{  set format
    int formatCount;
    int convDepth = -1;
    XPixmapFormatValues* formats = XListPixmapFormats (display, &formatCount);
    int depth = DefaultDepth (display, screen);
    Window defaultRootWindow = DefaultRootWindow (display);
    for (int i = 0; i < formatCount; ++i) {
      if (depth == formats[i].depth) {
        convDepth = formats[i].bits_per_pixel;
        break;
        }
      }
    XFree (formats);

    // We only support 32-bit right now
    if (convDepth != 32) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("failed to create 32 bir depth"));
      XCloseDisplay (display);
      return false;
      }
      //}}}
    //}}}
    //{{{  set width, height
    int screenWidth = DisplayWidth (display, screen);
    int screenHeight = DisplayHeight (display, screen);

    XSetWindowAttributes windowAttributes;
    windowAttributes.border_pixel = BlackPixel (display, screen);
    windowAttributes.background_pixel = BlackPixel (display, screen);
    windowAttributes.backing_store = NotUseful;

    windowWidth  = width;
    windowHeight = height;
    bufferWidth  = width;
    bufferHeight = height;
    bufferStride = width * 4;
    calcDstFactor (width, height);

    int posX, posY;
    int windowWidth, windowHeight;
    if (flags & WF_FULLSCREEN_DESKTOP) {
      //{{{  full screen desktop
      posX = 0;
      posY = 0;

      windowWidth  = screenWidth;
      windowHeight = screenHeight;
      }
      //}}}
    else {
      //{{{  window
      posX = (screenWidth  - width)  / 2;
      posY = (screenHeight - height) / 2;

      windowWidth  = width;
      windowHeight = height;
      }
      //}}}
    //}}}

    window = XCreateWindow (display, defaultRootWindow,
                            posX, posY, windowWidth, windowHeight,
                            0, depth, InputOutput, visual,
                            CWBackPixel | CWBorderPixel | CWBackingStore,
                            &windowAttributes);
    if (!window) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("failed to create X11 window"));
      return false;
      }
      //}}}

    XSelectInput (display, window,
                  StructureNotifyMask | ExposureMask |
                  FocusChangeMask |
                  KeyPressMask | KeyReleaseMask |
                  ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                  EnterWindowMask | LeaveWindowMask);
    XStoreName (display, window, title);

    if (flags & WF_BORDERLESS) {
      //{{{  borderless
      struct StyleHints {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long          inputMode;
        unsigned long status;
        } sh = {2, 0, 0, 0, 0};

      Atom sh_p = XInternAtom (display, "_MOTIF_WM_HINTS", True);
      XChangeProperty (display, window, sh_p, sh_p, 32,
                       PropModeReplace, (unsigned char*)&sh, 5);
      }
      //}}}
    if (flags & WF_ALWAYS_ON_TOP) {
      //{{{  always on top
      Atom sa_p = XInternAtom (display, "_NET_WM_STATE_ABOVE", False);
      XChangeProperty (display, window,
                       XInternAtom (display, "_NET_WM_STATE", False), XA_ATOM, 32,
                       PropModeReplace, (unsigned char*)&sa_p, 1);
      }
      //}}}
    if (flags & WF_FULLSCREEN) {
      //{{{  full screen
      Atom sf_p = XInternAtom (display, "_NET_WM_STATE_FULLSCREEN", True);
      XChangeProperty (display, window,
                       XInternAtom (display, "_NET_WM_STATE", True), XA_ATOM, 32,
                       PropModeReplace, (unsigned char*)&sf_p, 1);
      }
      //}}}

    gDeleteWindowAtom = XInternAtom (display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols (display, window, &gDeleteWindowAtom, 1);

    if (!createGLcontext()) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("failed to create X11 GL context"));
      return false;
      }
      //}}}

    //{{{  set sizeHints
    XSizeHints sizeHints;
    sizeHints.flags = PPosition | PMinSize | PMaxSize;
    sizeHints.x = 0;
    sizeHints.y = 0;
    sizeHints.min_width  = width;
    sizeHints.min_height = height;
    if (flags & WF_RESIZABLE) {
      //{{{  resizable
      sizeHints.max_width  = screenWidth;
      sizeHints.max_height = screenHeight;
      }
      //}}}
    else {
      //{{{  not resizable
      sizeHints.max_width  = width;
      sizeHints.max_height = height;
      }
      //}}}
    XSetWMNormalHints (display, window, &sizeHints);
    //}}}
    XClearWindow (display, window);
    XMapRaised (display, window);
    XFlush (display);

    gc = DefaultGC (display, screen);
    cLog::log (LOGINFO, "using X11 API");

    int32_t count;
    XDeviceInfoPtr devices = (XDeviceInfoPtr)XListInputDevices (display, &count);
    if (!devices) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("failed to find X11 input device list"));
      return false;
      }
      //}}}
    //{{{  search device list for "stylus"
    cLog::log (LOGINFO, fmt::format ("X11 input devices"));

    for (int32_t i = 0; i < count; i++) {
      cLog::log (LOGINFO, fmt::format ("- device:{} name:{} id:{}", i, devices[i].name, devices[i].id));
      if (strstr (devices[i].name, "stylus")) { // "eraser"
        gDevice = XOpenDevice (display, devices[i].id);
        XAnyClassPtr classPtr = devices[i].inputclassinfo;
        for (int32_t j = 0; j < devices[i].num_classes; j++) {
          switch (classPtr->c_class) {
            case ValuatorClass: {
              XValuatorInfo* valuatorInfo = (XValuatorInfo*)classPtr;
              if (valuatorInfo->num_axes > 0) {
                // x
                int32_t minX = valuatorInfo->axes[0].min_value;
                gRangeX = valuatorInfo->axes[0].max_value;
                cLog::log (LOGINFO, fmt::format ("- stylus xRange {}:{}", minX, gRangeX));
                }

              if (valuatorInfo->num_axes > 1) {
                // y
                int32_t minY = valuatorInfo->axes[1].min_value;
                gRangeY = valuatorInfo->axes[1].max_value;
                cLog::log (LOGINFO, fmt::format ("- stylus yRange {}:{}", minY, gRangeY));
                }

              if (valuatorInfo->num_axes > 2) {
                // pressure
                int32_t minPressure = valuatorInfo->axes[2].min_value;
                gMaxPressure = valuatorInfo->axes[2].max_value;
                cLog::log (LOGINFO, fmt::format ("- stylus pressureRange {}:{}", minPressure, gMaxPressure));
                }

              XEventClass eventClass;
              DeviceMotionNotify (gDevice, gMotionType, eventClass);
              if (eventClass) {
                gEventClasses[gNumEventClasses] = eventClass;
                gNumEventClasses++;
                }
              }
              break;

            default:
              cLog::log (LOGINFO, fmt::format ("- unused class:{}", classPtr->c_class));
              break;
            }
          classPtr = (XAnyClassPtr)((uint8_t*)classPtr + classPtr->length);
          }
        }
      XSelectExtensionEvent (display, window, gEventClasses, gNumEventClasses);
      }
    XFreeDeviceList (devices);
    //}}}
    //}}}
  #endif

  isInitialized = true;
  return true;
  }
//}}}
//{{{
void cMiniFB::initKeycodes() {

  #ifdef _WIN32
    if (gKeycodes[0x00B] != KB_KEY_0) {
      //{{{  alpha numeric
      gKeycodes[0x00B] = KB_KEY_0;
      gKeycodes[0x002] = KB_KEY_1;
      gKeycodes[0x003] = KB_KEY_2;
      gKeycodes[0x004] = KB_KEY_3;
      gKeycodes[0x005] = KB_KEY_4;
      gKeycodes[0x006] = KB_KEY_5;
      gKeycodes[0x007] = KB_KEY_6;
      gKeycodes[0x008] = KB_KEY_7;
      gKeycodes[0x009] = KB_KEY_8;
      gKeycodes[0x00A] = KB_KEY_9;
      gKeycodes[0x01E] = KB_KEY_A;
      gKeycodes[0x030] = KB_KEY_B;
      gKeycodes[0x02E] = KB_KEY_C;
      gKeycodes[0x020] = KB_KEY_D;
      gKeycodes[0x012] = KB_KEY_E;
      gKeycodes[0x021] = KB_KEY_F;
      gKeycodes[0x022] = KB_KEY_G;
      gKeycodes[0x023] = KB_KEY_H;
      gKeycodes[0x017] = KB_KEY_I;
      gKeycodes[0x024] = KB_KEY_J;
      gKeycodes[0x025] = KB_KEY_K;
      gKeycodes[0x026] = KB_KEY_L;
      gKeycodes[0x032] = KB_KEY_M;
      gKeycodes[0x031] = KB_KEY_N;
      gKeycodes[0x018] = KB_KEY_O;
      gKeycodes[0x019] = KB_KEY_P;
      gKeycodes[0x010] = KB_KEY_Q;
      gKeycodes[0x013] = KB_KEY_R;
      gKeycodes[0x01F] = KB_KEY_S;
      gKeycodes[0x014] = KB_KEY_T;
      gKeycodes[0x016] = KB_KEY_U;
      gKeycodes[0x02F] = KB_KEY_V;
      gKeycodes[0x011] = KB_KEY_W;
      gKeycodes[0x02D] = KB_KEY_X;
      gKeycodes[0x015] = KB_KEY_Y;
      gKeycodes[0x02C] = KB_KEY_Z;
      //}}}
      //{{{  punctuation
      gKeycodes[0x028] = KB_KEY_APOSTROPHE;
      gKeycodes[0x02B] = KB_KEY_BACKSLASH;
      gKeycodes[0x033] = KB_KEY_COMMA;
      gKeycodes[0x00D] = KB_KEY_EQUAL;
      gKeycodes[0x029] = KB_KEY_GRAVE_ACCENT;
      gKeycodes[0x01A] = KB_KEY_LEFT_BRACKET;
      gKeycodes[0x00C] = KB_KEY_MINUS;
      gKeycodes[0x034] = KB_KEY_PERIOD;
      gKeycodes[0x01B] = KB_KEY_RIGHT_BRACKET;
      gKeycodes[0x027] = KB_KEY_SEMICOLON;
      gKeycodes[0x035] = KB_KEY_SLASH;
      gKeycodes[0x056] = KB_KEY_WORLD_2;
      //}}}
      //{{{  other
      gKeycodes[0x00E] = KB_KEY_BACKSPACE;
      gKeycodes[0x153] = KB_KEY_DELETE;
      gKeycodes[0x14F] = KB_KEY_END;
      gKeycodes[0x01C] = KB_KEY_ENTER;
      gKeycodes[0x001] = KB_KEY_ESCAPE;
      gKeycodes[0x147] = KB_KEY_HOME;
      gKeycodes[0x152] = KB_KEY_INSERT;
      gKeycodes[0x15D] = KB_KEY_MENU;
      gKeycodes[0x151] = KB_KEY_PAGE_DOWN;
      gKeycodes[0x149] = KB_KEY_PAGE_UP;
      gKeycodes[0x045] = KB_KEY_PAUSE;
      gKeycodes[0x146] = KB_KEY_PAUSE;
      gKeycodes[0x039] = KB_KEY_SPACE;
      gKeycodes[0x00F] = KB_KEY_TAB;
      gKeycodes[0x03A] = KB_KEY_CAPS_LOCK;
      gKeycodes[0x145] = KB_KEY_NUM_LOCK;
      gKeycodes[0x046] = KB_KEY_SCROLL_LOCK;
      gKeycodes[0x03B] = KB_KEY_F1;
      gKeycodes[0x03C] = KB_KEY_F2;
      gKeycodes[0x03D] = KB_KEY_F3;
      gKeycodes[0x03E] = KB_KEY_F4;
      gKeycodes[0x03F] = KB_KEY_F5;
      gKeycodes[0x040] = KB_KEY_F6;
      gKeycodes[0x041] = KB_KEY_F7;
      gKeycodes[0x042] = KB_KEY_F8;
      gKeycodes[0x043] = KB_KEY_F9;
      gKeycodes[0x044] = KB_KEY_F10;
      gKeycodes[0x057] = KB_KEY_F11;
      gKeycodes[0x058] = KB_KEY_F12;
      gKeycodes[0x064] = KB_KEY_F13;
      gKeycodes[0x065] = KB_KEY_F14;
      gKeycodes[0x066] = KB_KEY_F15;
      gKeycodes[0x067] = KB_KEY_F16;
      gKeycodes[0x068] = KB_KEY_F17;
      gKeycodes[0x069] = KB_KEY_F18;
      gKeycodes[0x06A] = KB_KEY_F19;
      gKeycodes[0x06B] = KB_KEY_F20;
      gKeycodes[0x06C] = KB_KEY_F21;
      gKeycodes[0x06D] = KB_KEY_F22;
      gKeycodes[0x06E] = KB_KEY_F23;
      gKeycodes[0x076] = KB_KEY_F24;
      gKeycodes[0x038] = KB_KEY_LEFT_ALT;
      gKeycodes[0x01D] = KB_KEY_LEFT_CONTROL;
      gKeycodes[0x02A] = KB_KEY_LEFT_SHIFT;
      gKeycodes[0x15B] = KB_KEY_LEFT_SUPER;
      gKeycodes[0x137] = KB_KEY_PRINT_SCREEN;
      gKeycodes[0x138] = KB_KEY_RIGHT_ALT;
      gKeycodes[0x11D] = KB_KEY_RIGHT_CONTROL;
      gKeycodes[0x036] = KB_KEY_RIGHT_SHIFT;
      gKeycodes[0x15C] = KB_KEY_RIGHT_SUPER;
      gKeycodes[0x150] = KB_KEY_DOWN;
      gKeycodes[0x14B] = KB_KEY_LEFT;
      gKeycodes[0x14D] = KB_KEY_RIGHT;
      gKeycodes[0x148] = KB_KEY_UP;
      //}}}
      //{{{  numpad
      gKeycodes[0x052] = KB_KEY_KP_0;
      gKeycodes[0x04F] = KB_KEY_KP_1;
      gKeycodes[0x050] = KB_KEY_KP_2;
      gKeycodes[0x051] = KB_KEY_KP_3;
      gKeycodes[0x04B] = KB_KEY_KP_4;
      gKeycodes[0x04C] = KB_KEY_KP_5;
      gKeycodes[0x04D] = KB_KEY_KP_6;
      gKeycodes[0x047] = KB_KEY_KP_7;
      gKeycodes[0x048] = KB_KEY_KP_8;
      gKeycodes[0x049] = KB_KEY_KP_9;
      gKeycodes[0x04E] = KB_KEY_KP_ADD;
      gKeycodes[0x053] = KB_KEY_KP_DECIMAL;
      gKeycodes[0x135] = KB_KEY_KP_DIVIDE;
      gKeycodes[0x11C] = KB_KEY_KP_ENTER;
      gKeycodes[0x037] = KB_KEY_KP_MULTIPLY;
      gKeycodes[0x04A] = KB_KEY_KP_SUBTRACT;
      //}}}
      }
  #else
    // clear keys
    for (size_t i = 0; i < sizeof(gKeycodes) / sizeof(gKeycodes[0]); ++i)
      gKeycodes[i] = KB_KEY_UNKNOWN;

    // valid key code range is  [8,255], according to the Xlib manual
    for (size_t i = 8; i <= 255; ++i) {
      // try secondary keysym, for numeric keypad keys
      int keySym  = XkbKeycodeToKeysym (display, i, 0, 1);
      gKeycodes[i] = translateKeyCodeB (keySym);
      if (gKeycodes[i] == KB_KEY_UNKNOWN) {
        keySym = XkbKeycodeToKeysym (display, i, 0, 0);
        gKeycodes[i] = translateKeyCodeA (keySym);
        }
      }
  #endif
  }
//}}}
//{{{
void cMiniFB::freeResources() {

  #ifdef _WIN32
    destroyGLcontext();
    if (window && hdc) {
      ReleaseDC (window, hdc);
      DestroyWindow (window);
      }
    window = 0;
    hdc = 0;

    #ifdef USE_WINTAB
      winTabUnload();
    #endif
  #else
    if (gDevice)
      XCloseDevice (display, gDevice);
    destroyGLcontext();
  #endif

  // not sure why ?
  drawBuffer = nullptr;
  closed = true;
  }
//}}}

//{{{
bool cMiniFB::createGLcontext() {

  #ifdef _WIN32
    //{{{  setup windows pixel format for openGL
    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR), // size
                                 1,                             // version
                                 PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER, // support double-buffering
                                 PFD_TYPE_RGBA,                 // color type
                                 24,                            // preferred color depth
                                 0, 0, 0, 0, 0, 0,              // color and shift bits (ignored)
                                 0,                             // no alpha buffer
                                 0,                             // alpha bits (ignored)
                                 0,                             // no accumulation buffer
                                 0, 0, 0, 0,                    // accum bits (ignored)
                                 24,                            // depth buffer
                                 8,                             // no stencil buffer
                                 0,                             // no auxiliary buffers
                                 PFD_MAIN_PLANE,                // main layer
                                 0,                             // reserved
                                 0, 0, 0,                       // no layer, visible, damage masks
                                 };

    int pixelFormat = ChoosePixelFormat (hdc, &pfd);
    if (!pixelFormat) {
      cLog::log (LOGERROR, fmt::format ("ChoosePixelFormat failed {}", MB_ICONERROR | MB_OK));
      return false;
      }

    if (!SetPixelFormat (hdc, pixelFormat, &pfd)) {
      cLog::log (LOGERROR, fmt::format ("SetPixelFormat failed {}", MB_ICONERROR | MB_OK));
      return false;
      }
    //}}}
    hGLRC = wglCreateContext (hdc);
    wglMakeCurrent (hdc, hGLRC);

    cLog::log (LOGINFO, (const char*)glGetString (GL_VENDOR));
    cLog::log (LOGINFO, (const char*)glGetString (GL_RENDERER));
    cLog::log (LOGINFO, (const char*)glGetString (GL_VERSION));

    // get extensions
    gSwapInterval = (tGlSwapIntervalProc)wglGetProcAddress ("wglSwapIntervalEXT");
  #else
    // check openGL version
    GLint majorGLX = 0;
    GLint minorGLX = 0;
    glXQueryVersion (display, &majorGLX, &minorGLX);
    if ((majorGLX <= 1) && (minorGLX < 2)) {
      //{{{  error, return
      cLog::log (LOGERROR, "GLX 1.2 or greater is required");
      XCloseDisplay (display);

      return false;
      }
      //}}}
    else
      cLog::log (LOGINFO, fmt::format ("GLX version:{}.{}", majorGLX, minorGLX));

    //{{{  setup pixel format for X11 openGL
    GLint glxAttribs[] = {
      GLX_RGBA,
      GLX_DOUBLEBUFFER,
      GLX_DEPTH_SIZE,     24,
      GLX_STENCIL_SIZE,   8,
      GLX_RED_SIZE,       8,
      GLX_GREEN_SIZE,     8,
      GLX_BLUE_SIZE,      8,
      GLX_DEPTH_SIZE,     24,
      GLX_STENCIL_SIZE,   8,
      GLX_SAMPLE_BUFFERS, 0,
      GLX_SAMPLES,        0,
      None };

    XVisualInfo* visualInfo = glXChooseVisual (display, screen, glxAttribs);
    if (!visualInfo) {
      cLog::log (LOGERROR, "Could not create correct visual window");
      XCloseDisplay (display);
      return false;
      }
    //}}}
    context = glXCreateContext (display, visualInfo, NULL, GL_TRUE);
    glXMakeCurrent (display, window, context);

    cLog::log (LOGINFO, (const char*)glGetString (GL_VENDOR));
    cLog::log (LOGINFO, (const char*)glGetString (GL_RENDERER));
    cLog::log (LOGINFO, (const char*)glGetString (GL_VERSION));
    cLog::log (LOGINFO, (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION));

    if (checkGLExtension ("GLX_EXT_swap_control"))
      gSwapInterval = (tGlSwapIntervalProc)glXGetProcAddress ((const GLubyte*)"glXSwapIntervalEXT");
  #endif

  initGL();
  return true;
  }
//}}}
//{{{
void cMiniFB::initGL() {

  glViewport (0, 0, windowWidth, windowHeight);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();
  glOrtho (0, windowWidth, windowHeight, 0, 2048, -2048);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity();

  glDisable (GL_DEPTH_TEST);
  glDisable (GL_STENCIL_TEST);

  glEnable (GL_TEXTURE_2D);

  glGenTextures (1, &textureId);
  glBindTexture (GL_TEXTURE_2D, textureId);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glEnableClientState (GL_VERTEX_ARRAY);
  glEnableClientState (GL_TEXTURE_COORD_ARRAY);

  glDisableClientState (GL_TEXTURE_COORD_ARRAY);
  glDisableClientState (GL_VERTEX_ARRAY);
  glBindTexture (GL_TEXTURE_2D, 0);

  if (!gSwapInterval) {
    //{{{  error, return
    cLog::log (LOGERROR, "no swapInterval extension");
    return;
    }
    //}}}
  #ifdef _WIN32
    gSwapInterval (1);
  #else
    gSwapInterval (glXGetCurrentDisplay(), glXGetCurrentDrawable(), 1);
  #endif
  }
//}}}
//{{{
void cMiniFB::resizeGL() {

  if (isInitialized) {
    #ifdef _WIN32
      wglMakeCurrent (hdc, hGLRC);
    #else
      glXMakeCurrent (display, window, context);
    #endif

    glViewport (0,0, windowWidth,windowHeight);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho (0, windowWidth, windowHeight, 0, 2048, -2048);

    glClear (GL_COLOR_BUFFER_BIT);
    }
  }
//}}}
//{{{
void cMiniFB::redrawGL (const void* pixels) {

  #ifdef _WIN32
    wglMakeCurrent (hdc, hGLRC);
  #else
    glXMakeCurrent (display, window, context);
  #endif

  GLenum format = RGBA;

  // clear
  //glClear (GL_COLOR_BUFFER_BIT);

  glBindTexture (GL_TEXTURE_2D, textureId);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, format, GL_UNSIGNED_BYTE, pixels);
  //glTexSubImage2D (GL_TEXTURE_2D, 0,
  //                 0, 0, buffer_width, buffer_height,
  //                 format, GL_UNSIGNED_BYTE, pixels);

  // draw single texture
  glEnableClientState (GL_VERTEX_ARRAY);
  glEnableClientState (GL_TEXTURE_COORD_ARRAY);

  // vertices
  float x = (float)dstOffsetX;
  float y = (float)dstOffsetY;
  float w = (float)dstOffsetX + dstWidth;
  float h = (float)dstOffsetY + dstHeight;
  float vertices[] = { x, y, 0, 0,
                       w, y, 1, 0,
                       x, h, 0, 1,
                       w, h, 1, 1 };
  glVertexPointer (2, GL_FLOAT, 4 * sizeof(float), vertices);
  glTexCoordPointer (2, GL_FLOAT, 4 * sizeof(float), vertices + 2);
  glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

  glDisableClientState (GL_TEXTURE_COORD_ARRAY);
  glDisableClientState (GL_VERTEX_ARRAY);
  glBindTexture (GL_TEXTURE_2D, 0);

  // swap buffer
  #ifdef _WIN32
    SwapBuffers (hdc);
  #else
    glXSwapBuffers (display, window);
  #endif
  }
//}}}
//{{{
void cMiniFB::destroyGLcontext() {

  #ifdef _WIN32
    if (hGLRC) {
      wglMakeCurrent (NULL, NULL);
      wglDeleteContext (hGLRC);
      hGLRC = 0;
      }
  #else
    glXDestroyContext (display, context);
  #endif
  }
//}}}

//{{{
void cMiniFB::resizeDst (uint32_t width, uint32_t height) {

  dstOffsetX = (uint32_t) (width  * factorX);
  dstOffsetY = (uint32_t) (height * factorY);
  dstWidth    = (uint32_t) (width  * factorWidth);
  dstHeight   = (uint32_t) (height * factorHeight);
  }
//}}}
//{{{
void cMiniFB::calcDstFactor (uint32_t width, uint32_t height) {

  if (dstWidth == 0)
    dstWidth = width;

  factorX     = (float)dstOffsetX / (float)width;
  factorWidth = (float)dstWidth    / (float)width;

  if (dstHeight == 0)
    dstHeight = height;

  factorY      = (float)dstOffsetY / (float)height;
  factorHeight = (float)dstHeight   / (float)height;
  }
//}}}
