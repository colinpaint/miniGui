// WinMiniFB.cpp
//{{{  includes
#include <cstdint>

#include "../../include/MiniFB.h"

#include "../MiniFB_internal.h"
#include "../WindowData.h"
#include "../windows/WindowData_Win.h"

#include "../gl/MiniFB_GL.h"

#include "../../../common/cLog.h"

using namespace std;
//}}}
//{{{  typedefs
// Copied (and modified) from Windows Kit 10 to avoid setting _WIN32_WINNT to a higher version
typedef enum mfb_PROCESS_DPI_AWARENESS {
  mfb_PROCESS_DPI_UNAWARE = 0,
  mfb_PROCESS_SYSTEM_DPI_AWARE = 1,
  mfb_PROCESS_PER_MONITOR_DPI_AWARE = 2
  } mfb_PROCESS_DPI_AWARENESS;

typedef enum mfb_MONITOR_DPI_TYPE {
  mfb_MDT_EFFECTIVE_DPI = 0,
  mfb_MDT_ANGULAR_DPI   = 1,
  mfb_MDT_RAW_DPI       = 2,
  mfb_MDT_DEFAULT       = mfb_MDT_EFFECTIVE_DPI
  } mfb_MONITOR_DPI_TYPE;

#define mfb_DPI_AWARENESS_CONTEXT_UNAWARE              ((HANDLE) -1)
#define mfb_DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((HANDLE) -2)
#define mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((HANDLE) -3)
#define mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE) -4)
#define mfb_DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((HANDLE) -5)

// user32.dll
typedef BOOL(WINAPI *PFN_SetProcessDPIAware)(void);
typedef BOOL(WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
typedef UINT(WINAPI *PFN_GetDpiForWindow)(HWND);
typedef BOOL(WINAPI *PFN_EnableNonClientDpiScaling)(HWND);

HMODULE                           mfb_user32_dll                    = 0x0;
PFN_SetProcessDPIAware            mfb_SetProcessDPIAware            = 0x0;
PFN_SetProcessDpiAwarenessContext mfb_SetProcessDpiAwarenessContext = 0x0;
PFN_GetDpiForWindow               mfb_GetDpiForWindow               = 0x0;
PFN_EnableNonClientDpiScaling     mfb_EnableNonClientDpiScaling     = 0x0;

// shcore.dll
typedef HRESULT(WINAPI *PFN_SetProcessDpiAwareness)(mfb_PROCESS_DPI_AWARENESS);
typedef HRESULT(WINAPI *PFN_GetDpiForMonitor)(HMONITOR, mfb_MONITOR_DPI_TYPE, UINT *, UINT *);
//}}}

#include "winTab.h"
#define PACKETDATA PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TIME | PK_SERIAL_NUMBER
#include "pktDef.h"

extern short int g_keycodes[512];
extern bool g_use_hardware_sync;
extern double g_time_for_frame;
extern double g_timer_frequency;
extern double g_timer_resolution;

namespace {
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
    UINT  mSerialNumber;

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
    //gWinTab->mWTInfoA (WTI_DDCTXS, 0, &logContext);
    gWinTab->mWTInfoA (WTI_DEFSYSCTX, 0, &logContext);

    logContext.lcPktData = PACKETDATA;
    logContext.lcOptions |= CXO_MESSAGES | CXO_SYSTEM;
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

  HMODULE mfb_shcore_dll = 0x0;
  PFN_GetDpiForMonitor mfb_GetDpiForMonitor = 0x0;
  PFN_SetProcessDpiAwareness mfb_SetProcessDpiAwareness = 0x0;

  //{{{
  // NOT Thread safe. Just convenient (Don't do this at home guys)
  char* GetErrorMessage() {

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
  void load_functions() {

    if (mfb_user32_dll == 0x0) {
      mfb_user32_dll = LoadLibraryA ("user32.dll");
      if (mfb_user32_dll != 0x0) {
        mfb_SetProcessDPIAware =
          (PFN_SetProcessDPIAware)GetProcAddress (mfb_user32_dll, "SetProcessDPIAware");

        mfb_SetProcessDpiAwarenessContext =
          (PFN_SetProcessDpiAwarenessContext)GetProcAddress (mfb_user32_dll, "SetProcessDpiAwarenessContext");

        mfb_GetDpiForWindow =
          (PFN_GetDpiForWindow)GetProcAddress (mfb_user32_dll, "GetDpiForWindow");

        mfb_EnableNonClientDpiScaling =
          (PFN_EnableNonClientDpiScaling)GetProcAddress (mfb_user32_dll, "EnableNonClientDpiScaling");
        }
      }

    if (mfb_shcore_dll == 0x0) {
      mfb_shcore_dll = LoadLibraryA ("shcore.dll");
      if (mfb_shcore_dll != 0x0) {
        mfb_SetProcessDpiAwareness =
          (PFN_SetProcessDpiAwareness)GetProcAddress (mfb_shcore_dll, "SetProcessDpiAwareness");
        mfb_GetDpiForMonitor =
          (PFN_GetDpiForMonitor)GetProcAddress (mfb_shcore_dll, "GetDpiForMonitor");
        }
      }
    }
  //}}}
  //{{{
  void dpi_aware() {

    if (mfb_SetProcessDpiAwarenessContext) {
      if (!mfb_SetProcessDpiAwarenessContext (mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        uint32_t error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER) {
          error = NO_ERROR;
          if (!mfb_SetProcessDpiAwarenessContext (mfb_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
            error = GetLastError();
          }
        if (error != NO_ERROR)
          cLog::log (LOGERROR, fmt::format ("SetProcessDpiAwarenessContext failed {}", GetErrorMessage()));
        }
      }

    else if (mfb_SetProcessDpiAwareness) {
      if (mfb_SetProcessDpiAwareness(mfb_PROCESS_PER_MONITOR_DPI_AWARE) != S_OK)
        cLog::log (LOGERROR, fmt::format ("SetProcessDpiAwareness failed {}", GetErrorMessage()));
      }

    else if (mfb_SetProcessDPIAware) {
      if (!mfb_SetProcessDPIAware())
        cLog::log (LOGERROR, fmt::format ("SetProcessDPIAware failed {}", GetErrorMessage()));
      }

    }
  //}}}
  //{{{
  void get_monitor_scale (HWND hWnd, float* scale_x, float* scale_y) {

    UINT x, y;

    if (mfb_GetDpiForMonitor) {
      HMONITOR monitor = MonitorFromWindow (hWnd, MONITOR_DEFAULTTONEAREST);
      mfb_GetDpiForMonitor (monitor, mfb_MDT_EFFECTIVE_DPI, &x, &y);
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
  void init_keycodes() {

    if (g_keycodes[0x00B] != KB_KEY_0) {
      //{{{  alpha numeric
      g_keycodes[0x00B] = KB_KEY_0;
      g_keycodes[0x002] = KB_KEY_1;
      g_keycodes[0x003] = KB_KEY_2;
      g_keycodes[0x004] = KB_KEY_3;
      g_keycodes[0x005] = KB_KEY_4;
      g_keycodes[0x006] = KB_KEY_5;
      g_keycodes[0x007] = KB_KEY_6;
      g_keycodes[0x008] = KB_KEY_7;
      g_keycodes[0x009] = KB_KEY_8;
      g_keycodes[0x00A] = KB_KEY_9;
      g_keycodes[0x01E] = KB_KEY_A;
      g_keycodes[0x030] = KB_KEY_B;
      g_keycodes[0x02E] = KB_KEY_C;
      g_keycodes[0x020] = KB_KEY_D;
      g_keycodes[0x012] = KB_KEY_E;
      g_keycodes[0x021] = KB_KEY_F;
      g_keycodes[0x022] = KB_KEY_G;
      g_keycodes[0x023] = KB_KEY_H;
      g_keycodes[0x017] = KB_KEY_I;
      g_keycodes[0x024] = KB_KEY_J;
      g_keycodes[0x025] = KB_KEY_K;
      g_keycodes[0x026] = KB_KEY_L;
      g_keycodes[0x032] = KB_KEY_M;
      g_keycodes[0x031] = KB_KEY_N;
      g_keycodes[0x018] = KB_KEY_O;
      g_keycodes[0x019] = KB_KEY_P;
      g_keycodes[0x010] = KB_KEY_Q;
      g_keycodes[0x013] = KB_KEY_R;
      g_keycodes[0x01F] = KB_KEY_S;
      g_keycodes[0x014] = KB_KEY_T;
      g_keycodes[0x016] = KB_KEY_U;
      g_keycodes[0x02F] = KB_KEY_V;
      g_keycodes[0x011] = KB_KEY_W;
      g_keycodes[0x02D] = KB_KEY_X;
      g_keycodes[0x015] = KB_KEY_Y;
      g_keycodes[0x02C] = KB_KEY_Z;
      //}}}
      //{{{  punctuation
      g_keycodes[0x028] = KB_KEY_APOSTROPHE;
      g_keycodes[0x02B] = KB_KEY_BACKSLASH;
      g_keycodes[0x033] = KB_KEY_COMMA;
      g_keycodes[0x00D] = KB_KEY_EQUAL;
      g_keycodes[0x029] = KB_KEY_GRAVE_ACCENT;
      g_keycodes[0x01A] = KB_KEY_LEFT_BRACKET;
      g_keycodes[0x00C] = KB_KEY_MINUS;
      g_keycodes[0x034] = KB_KEY_PERIOD;
      g_keycodes[0x01B] = KB_KEY_RIGHT_BRACKET;
      g_keycodes[0x027] = KB_KEY_SEMICOLON;
      g_keycodes[0x035] = KB_KEY_SLASH;
      g_keycodes[0x056] = KB_KEY_WORLD_2;
      //}}}
      //{{{  other
      g_keycodes[0x00E] = KB_KEY_BACKSPACE;
      g_keycodes[0x153] = KB_KEY_DELETE;
      g_keycodes[0x14F] = KB_KEY_END;
      g_keycodes[0x01C] = KB_KEY_ENTER;
      g_keycodes[0x001] = KB_KEY_ESCAPE;
      g_keycodes[0x147] = KB_KEY_HOME;
      g_keycodes[0x152] = KB_KEY_INSERT;
      g_keycodes[0x15D] = KB_KEY_MENU;
      g_keycodes[0x151] = KB_KEY_PAGE_DOWN;
      g_keycodes[0x149] = KB_KEY_PAGE_UP;
      g_keycodes[0x045] = KB_KEY_PAUSE;
      g_keycodes[0x146] = KB_KEY_PAUSE;
      g_keycodes[0x039] = KB_KEY_SPACE;
      g_keycodes[0x00F] = KB_KEY_TAB;
      g_keycodes[0x03A] = KB_KEY_CAPS_LOCK;
      g_keycodes[0x145] = KB_KEY_NUM_LOCK;
      g_keycodes[0x046] = KB_KEY_SCROLL_LOCK;
      g_keycodes[0x03B] = KB_KEY_F1;
      g_keycodes[0x03C] = KB_KEY_F2;
      g_keycodes[0x03D] = KB_KEY_F3;
      g_keycodes[0x03E] = KB_KEY_F4;
      g_keycodes[0x03F] = KB_KEY_F5;
      g_keycodes[0x040] = KB_KEY_F6;
      g_keycodes[0x041] = KB_KEY_F7;
      g_keycodes[0x042] = KB_KEY_F8;
      g_keycodes[0x043] = KB_KEY_F9;
      g_keycodes[0x044] = KB_KEY_F10;
      g_keycodes[0x057] = KB_KEY_F11;
      g_keycodes[0x058] = KB_KEY_F12;
      g_keycodes[0x064] = KB_KEY_F13;
      g_keycodes[0x065] = KB_KEY_F14;
      g_keycodes[0x066] = KB_KEY_F15;
      g_keycodes[0x067] = KB_KEY_F16;
      g_keycodes[0x068] = KB_KEY_F17;
      g_keycodes[0x069] = KB_KEY_F18;
      g_keycodes[0x06A] = KB_KEY_F19;
      g_keycodes[0x06B] = KB_KEY_F20;
      g_keycodes[0x06C] = KB_KEY_F21;
      g_keycodes[0x06D] = KB_KEY_F22;
      g_keycodes[0x06E] = KB_KEY_F23;
      g_keycodes[0x076] = KB_KEY_F24;
      g_keycodes[0x038] = KB_KEY_LEFT_ALT;
      g_keycodes[0x01D] = KB_KEY_LEFT_CONTROL;
      g_keycodes[0x02A] = KB_KEY_LEFT_SHIFT;
      g_keycodes[0x15B] = KB_KEY_LEFT_SUPER;
      g_keycodes[0x137] = KB_KEY_PRINT_SCREEN;
      g_keycodes[0x138] = KB_KEY_RIGHT_ALT;
      g_keycodes[0x11D] = KB_KEY_RIGHT_CONTROL;
      g_keycodes[0x036] = KB_KEY_RIGHT_SHIFT;
      g_keycodes[0x15C] = KB_KEY_RIGHT_SUPER;
      g_keycodes[0x150] = KB_KEY_DOWN;
      g_keycodes[0x14B] = KB_KEY_LEFT;
      g_keycodes[0x14D] = KB_KEY_RIGHT;
      g_keycodes[0x148] = KB_KEY_UP;
      //}}}
      //{{{  numpad
      g_keycodes[0x052] = KB_KEY_KP_0;
      g_keycodes[0x04F] = KB_KEY_KP_1;
      g_keycodes[0x050] = KB_KEY_KP_2;
      g_keycodes[0x051] = KB_KEY_KP_3;
      g_keycodes[0x04B] = KB_KEY_KP_4;
      g_keycodes[0x04C] = KB_KEY_KP_5;
      g_keycodes[0x04D] = KB_KEY_KP_6;
      g_keycodes[0x047] = KB_KEY_KP_7;
      g_keycodes[0x048] = KB_KEY_KP_8;
      g_keycodes[0x049] = KB_KEY_KP_9;
      g_keycodes[0x04E] = KB_KEY_KP_ADD;
      g_keycodes[0x053] = KB_KEY_KP_DECIMAL;
      g_keycodes[0x135] = KB_KEY_KP_DIVIDE;
      g_keycodes[0x11C] = KB_KEY_KP_ENTER;
      g_keycodes[0x037] = KB_KEY_KP_MULTIPLY;
      g_keycodes[0x04A] = KB_KEY_KP_SUBTRACT;
      //}}}
      }
    }
  //}}}
  //{{{
  uint32_t translate_mod() {

    uint32_t mods = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000)
      mods |= KB_MOD_SHIFT;

    if (GetKeyState(VK_CONTROL) & 0x8000)
      mods |= KB_MOD_CONTROL;

    if (GetKeyState(VK_MENU) & 0x8000)
      mods |= KB_MOD_ALT;

    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
      mods |= KB_MOD_SUPER;

    if (GetKeyState(VK_CAPITAL) & 1)
      mods |= KB_MOD_CAPS_LOCK;

    if (GetKeyState(VK_NUMLOCK) & 1)
      mods |= KB_MOD_NUM_LOCK;

    return mods;
    }
  //}}}
  //{{{
  mfb_key translate_key (unsigned int wParam, unsigned long lParam) {

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

    return (mfb_key)g_keycodes[HIWORD(lParam) & 0x1FF];
    }
  //}}}

  //{{{
  void destroy_window_data (SWindowData* window_data) {

    if (window_data == 0x0)
      return;

    SWindowData_Win* window_data_win = (SWindowData_Win*)window_data->specific;

    destroy_GL_context (window_data);

    if (window_data_win->window != 0 && window_data_win->hdc != 0) {
      ReleaseDC (window_data_win->window, window_data_win->hdc);
      DestroyWindow (window_data_win->window);
      winTabUnload();
      }

    window_data_win->window = 0;
    window_data_win->hdc = 0;

    mfb_timer_destroy(window_data_win->timer);
    window_data_win->timer = 0x0;

    window_data->draw_buffer = 0x0;
    window_data->close = true;
    }
  //}}}
  //{{{
  LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;

    SWindowData* window_data = (SWindowData*) GetWindowLongPtr (hWnd, GWLP_USERDATA);
    SWindowData_Win* window_data_win = window_data ? (SWindowData_Win*)window_data->specific : nullptr;

    switch (message) {
      //{{{
      case WM_NCCREATE:
        if (mfb_EnableNonClientDpiScaling)
          mfb_EnableNonClientDpiScaling(hWnd);

        return DefWindowProc(hWnd, message, wParam, lParam);
      //}}}
      //{{{
      //case 0x02E4://WM_GETDPISCALEDSIZE:
      //{
      //    SIZE* size = (SIZE*) lParam;
      //    WORD dpi = LOWORD(wParam);
      //    return true;
      //    break;
              //}
      //}}}
      //{{{
      //case WM_DPICHANGED:
      //{
      //    const float xscale = HIWORD(wParam);
      //    const float yscale = LOWORD(wParam);
      //    break;
      //}
      //}}}

      //{{{
      case WM_CLOSE:
        if (window_data) {
          bool destroy = false;

          // Obtain a confirmation of close
          if (!window_data->close_func || window_data->close_func ((struct mfb_window*)window_data))
            destroy = true;

          if (destroy) {
            window_data->close = true;
            if (window_data_win)
              DestroyWindow (window_data_win->window);
            }
          }
        break;
      //}}}
      //{{{
      case WM_DESTROY:
        if (window_data)
          window_data->close = true;
        break;
      //}}}

      //{{{
      case WM_SETFOCUS:

        if (window_data) {
          window_data->is_active = true;
          kCall (active_func, true);
          }

        break;
      //}}}
      //{{{
      case WM_KILLFOCUS:

        if (window_data) {
          window_data->is_active = false;
          kCall (active_func, false);
          }

        break;
      //}}}

      //{{{
      case WM_SIZE:

        if (window_data) {
          if (wParam == SIZE_MINIMIZED)
            return result;

          float scale_x, scale_y;
          get_monitor_scale (hWnd, &scale_x, &scale_y);
          window_data->window_width = LOWORD(lParam);
          window_data->window_height = HIWORD(lParam);
          resize_dst (window_data, window_data->window_width, window_data->window_height);

          resize_GL (window_data);
          if (window_data->window_width != 0 && window_data->window_height != 0) {
            uint32_t width, height;
            width  = (uint32_t)(window_data->window_width  / scale_x);
            height = (uint32_t)(window_data->window_height / scale_y);
            kCall (resize_func, width, height);
            }
          }

        break;
      //}}}

      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      case WM_KEYUP:
      //{{{
      case WM_SYSKEYUP:
        if (window_data) {
          mfb_key key_code = translate_key ((unsigned int)wParam, (unsigned long)lParam);
          int is_pressed = !((lParam >> 31) & 1);
          window_data->mod_keys = translate_mod();

          if (key_code == KB_KEY_UNKNOWN)
            return FALSE;

          window_data->key_status[key_code] = (uint8_t)is_pressed;
          kCall (keyboard_func, key_code, (mfb_key_mod)window_data->mod_keys, is_pressed);
          }

        break;
      //}}}

      case WM_CHAR:
      //{{{
      case WM_SYSCHAR: {
        static WCHAR highSurrogate = 0;

        if (window_data) {
          if (wParam >= 0xd800 && wParam <= 0xdbff) {
            highSurrogate = (WCHAR) wParam;
            }
          else {
            unsigned int codepoint = 0;
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
            kCall(char_input_func, codepoint);
            }
          }
        }

        break;
      //}}}
      //{{{
      case WM_UNICHAR:
        {
        if (window_data) {
          if (wParam == UNICODE_NOCHAR) {
            // WM_UNICHAR is not sent by Windows, but is sent by some third-party input method engine
            // Returning TRUE here announces support for this message
            return TRUE;
            }

          kCall(char_input_func, (unsigned int) wParam);
          }
        break;
        }
      //}}}

      //{{{
      case WT_PACKET:
        if ((HCTX)lParam == gWinTab->mContext) {
          PACKET packet = {0};
          if (gWinTab->mWTPacket (gWinTab->mContext, (UINT)wParam, &packet)) {
            POINT point = { 0 };
            point.x = packet.pkX;
            point.y = packet.pkY;
            ScreenToClient (hWnd, &point);

            gWinTab->mPosX = point.x;
            gWinTab->mPosY = point.y;
            gWinTab->mPressure = (float)packet.pkNormalPressure / (float)gWinTab->mMaxPressure;
            gWinTab->mButtons = packet.pkButtons;
            gWinTab->mTime = packet.pkTime;
            gWinTab->mSerialNumber = packet.pkSerialNumber;

            //cLog::log (LOGINFO, fmt::format ("WT_PACKET {}:{} press:{} but:{} time:{} no:{}",
            //                                 gWinTab->mPosX, gWinTab->mPosY,
            //                                 gWinTab->mPressure, gWinTab->mButtons,
            //                                 gWinTab->mTime, gWinTab->mSerialNumber));
            }
          else
            cLog::log (LOGINFO, fmt::format ("WT_PACKET no packet {:x} {:x}", wParam, lParam));
          }
        else
          cLog::log (LOGINFO, fmt::format ("WT_PACKET wrong context {:x} {:x}", wParam, lParam));

        break;
      //}}}
      //{{{
      case WT_PROXIMITY:
        cLog::log (LOGINFO, fmt::format ("WT_PROXIMITY {:x} {:x}", wParam, lParam));
        break;
      //}}}

      //{{{  pointer info
      //POINTER_INFO
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

      ////POINTER_INPUT
        //PT_POINTER = 1,
        //PT_TOUCH = 2,
        //PT_PEN = 3,
        //PT_MOUSE = 4,
        //PT_TOUCHPAD = 5

      ////POINTER_FLAG
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

      ////POINTER_BUTTON_CHANGE_TYPE {
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
      //{{{  pointerPenInfo
      //BOOL GetPointerPenInfo ([in]UINT32 pointerId, [out] POINTER_PEN_INFO* penInfo)

      ////POINTER_PEN_INFO
        //POINTER_INFO pointerInfo;
        //PEN_FLAGS    penFlags;
        //PEN_MASK     penMask;
        //UINT32       pressure;
        //UINT32       rotation;
        //INT32        tiltX;
        //INT32        tiltY;
      //}}}
      //{{{
      case WM_POINTERWHEEL  :
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERWHEEL {:x} {:x}", wParam, lParam));
        if (window_data) {
          window_data->mouse_wheel_y = (SHORT)HIWORD(wParam) / (float)WHEEL_DELTA;
          kCall (mouse_wheel_func, (mfb_key_mod)translate_mod(), 0.0f, window_data->mouse_wheel_y);
          }
        break;
      //}}}
      //{{{
      case WM_POINTERUPDATE: 
        if (window_data) {
          POINTER_INFO pointerInfo;
          if (GetPointerInfo (GET_POINTERID_WPARAM (wParam), &pointerInfo)) {
            //cLog::log (LOGINFO, fmt::format ("WM_POINTERUPDATE info {} {} {}",
            //                                 pointerInfo.pointerType, pointerInfo.pointerFlags, pointerInfo.dwTime));
            if (pointerInfo.pointerType == PT_MOUSE) {
              }
            else if (pointerInfo.pointerType == PT_PEN) {
              }

            window_data_win->mouse_inside = true;

            POINT clientPos = pointerInfo.ptPixelLocation;
            ScreenToClient (hWnd, &clientPos);
            window_data->mouse_pos_x = clientPos.x;
            window_data->mouse_pos_y = clientPos.y;

            kCall (mouse_move_func, window_data->mouse_pos_x, window_data->mouse_pos_y);
            }
          }
        break;
      //}}}
      //{{{
      case WM_POINTERDOWN:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERDOWN {:x} {:x}", wParam, lParam));
        if (window_data) {
          POINTER_INFO pointerInfo;
          if (GetPointerInfo (GET_POINTERID_WPARAM (wParam), &pointerInfo)) {
            if (pointerInfo.pointerType == PT_MOUSE) { // unused PT_TOUCH, PT_TOUCHPAD
              cLog::log (LOGINFO, fmt::format ("mouseDown"));
              }
            else if (pointerInfo.pointerType == PT_PEN) {
              cLog::log (LOGINFO, fmt::format ("penDown"));
              }

            window_data->mod_keys = translate_mod();
            window_data->mouse_button_status[MOUSE_BTN_1] = 1;
            kCall (mouse_btn_func, MOUSE_BTN_1, (mfb_key_mod)window_data->mod_keys, 1);
            }
          }
        break;
      //}}}
      //{{{
      case WM_POINTERUP:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERUP {:x} {:x}", wParam, lParam));
        if (window_data) {
          POINTER_INFO pointerInfo;
          if (GetPointerInfo (GET_POINTERID_WPARAM (wParam), &pointerInfo)) {
            if (pointerInfo.pointerType == PT_MOUSE) {
              cLog::log (LOGINFO, fmt::format ("mouseUp"));
              }
            else if (pointerInfo.pointerType == PT_PEN) {
              cLog::log (LOGINFO, fmt::format ("penUp"));
              }

            window_data->mod_keys = translate_mod();
            window_data->mouse_button_status[MOUSE_BTN_1] = 0;
            kCall (mouse_btn_func, MOUSE_BTN_1, (mfb_key_mod)window_data->mod_keys, 0);
            }
          }
        break;
      //}}}
      //{{{
      case WM_POINTERLEAVE:
        cLog::log (LOGINFO, fmt::format ("WM_POINTERLEAVE {:x} {:x}", wParam, lParam));
        window_data_win->mouse_inside = false;
        break;
      //}}}
      //{{{
      //case WM_MOUSEHWHEEL:
        //// This message is only sent on Windows Vista and later
        //// NOTE: The X-axis is inverted for consistency with macOS and X11

        //if (window_data) {
          //window_data->mouse_wheel_x = -((SHORT)HIWORD(wParam) / (float)WHEEL_DELTA);
          //kCall (mouse_wheel_func, (mfb_key_mod)translate_mod(), window_data->mouse_wheel_x, 0.0f);
          //}

        //break;
      //}}}

      //{{{
      //case WM_INPUT:
        //cLog::log (LOGINFO, fmt::format ("WM_INPUT {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_NCPOINTERUPDATE:
        //cLog::log (LOGINFO, fmt::format ("WM_NCPOINTERUPDATE  {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_NCPOINTERUP:
        //cLog::log (LOGINFO, fmt::format ("WM_NCPOINTERUP  {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_POINTERENTER:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERENTER {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_POINTERACTIVATE:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERACTIVATE   {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_POINTERCAPTURECHANGED:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERCAPTURECHANGED {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_POINTERDEVICECHANGE:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERDEVICECHANGE {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_POINTERDEVICEINRANGE:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERDEVICEINRANGE {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_POINTERDEVICEOUTOFRANGE:
        //cLog::log (LOGINFO, fmt::format ("WM_POINTERDEVICEOUTOFRANGE {:x} {:x}", wParam, lParam));
        //break;
      //}}}
      //{{{
      //case WM_TOUCHHITTESTING:
        //cLog::log (LOGINFO, fmt::format ("WM_TOUCHHITTESTING  {:x} {:x}", wParam, lParam));
        //break;
      //}}}

      default:
        result = DefWindowProc (hWnd, message, wParam, lParam);
      }

    return result;
    }
  //}}}
  }

// interface
//{{{
struct mfb_window* mfb_open_ex (const char* title, unsigned width, unsigned height, unsigned flags) {

  RECT rect = { 0 };
  int  x = 0, y = 0;

  load_functions();
  dpi_aware();
  init_keycodes();

  SWindowData* window_data = (SWindowData*)malloc(sizeof(SWindowData));
  if (window_data == 0x0)
    return 0x0;
  memset (window_data, 0, sizeof(SWindowData));

  SWindowData_Win* window_data_win = (SWindowData_Win*)malloc(sizeof(SWindowData_Win));
  if (window_data_win == 0x0) {
    //{{{  error, return
    free (window_data);
    return 0x0;
    }
    //}}}

  memset(window_data_win, 0, sizeof(SWindowData_Win));
  window_data->specific = window_data_win;
  window_data->buffer_width  = width;
  window_data->buffer_height = height;
  window_data->buffer_stride = width * 4;

  long s_window_style = WS_POPUP | WS_SYSMENU | WS_CAPTION;
  s_window_style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
  if (flags & WF_FULLSCREEN) {
    //{{{  fullscreen
    flags = WF_FULLSCREEN;  // Remove all other flags
    rect.right  = GetSystemMetrics (SM_CXSCREEN);
    rect.bottom = GetSystemMetrics (SM_CYSCREEN);
    s_window_style = WS_POPUP & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

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
    s_window_style = WS_POPUP;
  if (flags & WF_RESIZABLE)
    s_window_style |= WS_MAXIMIZEBOX | WS_SIZEBOX;

  if (flags & WF_FULLSCREEN_DESKTOP) {
    //{{{  desktop
    s_window_style = WS_OVERLAPPEDWINDOW;

    width  = GetSystemMetrics (SM_CXFULLSCREEN);
    height = GetSystemMetrics (SM_CYFULLSCREEN);

    rect.right  = width;
    rect.bottom = height;
    AdjustWindowRect (&rect, s_window_style, 0);
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
    get_monitor_scale(0, &scale_x, &scale_y);

    rect.right  = (LONG) (width  * scale_x);
    rect.bottom = (LONG) (height * scale_y);
    AdjustWindowRect(&rect, s_window_style, 0);

    rect.right  -= rect.left;
    rect.bottom -= rect.top;
    x = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom + rect.top) / 2;
    }
    //}}}

  window_data_win->wc.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  window_data_win->wc.lpfnWndProc   = WndProc;
  window_data_win->wc.hCursor       = LoadCursor(0, IDC_ARROW);
  window_data_win->wc.lpszClassName = title;
  RegisterClass (&window_data_win->wc);

  calc_dst_factor (window_data, width, height);

  window_data->window_width  = rect.right;
  window_data->window_height = rect.bottom;

  window_data_win->window = CreateWindowEx (0,
                                            title, title,
                                            s_window_style,
                                            x, y,
                                            window_data->window_width, window_data->window_height,
                                            0, 0, 0, 0);
  if (!window_data_win->window) {
    //{{{  error, return
    free (window_data);
    free (window_data_win);
    return 0x0;
    }
    //}}}

  SetWindowLongPtr (window_data_win->window, GWLP_USERDATA, (LONG_PTR) window_data);

  if (flags & WF_ALWAYS_ON_TOP)
    SetWindowPos (window_data_win->window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  ShowWindow (window_data_win->window, SW_NORMAL);

  window_data_win->hdc = GetDC (window_data_win->window);

  create_GL_context (window_data);
  window_data_win->timer = mfb_timer_create();
  mfb_set_keyboard_callback ((struct mfb_window*) window_data, keyboard_default);

  cLog::log (LOGINFO, "using windows OpenGL");

  // enable WM_POINTER wndProc messaging, to tell mouse from pen
  EnableMouseInPointer (true);

  // enable winTab, mainly for WT_PROXIMITY, get WT_PACKET
  if (!winTabLoad (window_data_win->window))
    cLog::log (LOGERROR, fmt::format ("winTab load failed"));

  window_data->is_initialized = true;
  return (struct mfb_window*)window_data;
  }
//}}}
//{{{
mfb_update_state mfb_update_ex (struct mfb_window* window, void* buffer, unsigned width, unsigned height) {

  if (!window)
    return STATE_INVALID_WINDOW;

  SWindowData* window_data = (SWindowData*)window;
  if (window_data->close) {
    destroy_window_data (window_data);
    return STATE_EXIT;
    }

  if (!buffer)
    return STATE_INVALID_BUFFER;
  window_data->draw_buffer = buffer;
  window_data->buffer_width = width;
  window_data->buffer_stride = width * 4;
  window_data->buffer_height = height;
  redraw_GL (window_data, buffer);

  return STATE_OK;
  }
//}}}
//{{{
mfb_update_state mfb_update_events (struct mfb_window* window) {

  if (!window)
    return STATE_INVALID_WINDOW;

  SWindowData* window_data = (SWindowData*)window;
  if (window_data->close) {
    destroy_window_data (window_data);
    return STATE_EXIT;
    }

  MSG msg;
  SWindowData_Win* window_data_win = (SWindowData_Win*)window_data->specific;
  while (!window_data->close && PeekMessage (&msg, window_data_win->window, 0, 0, PM_REMOVE)) {
    //if(msg.message == WM_PAINT)
    //    return STATE_OK;
    TranslateMessage (&msg);
    DispatchMessage (&msg);
    }

  return STATE_OK;
  }
//}}}

// viewport
//{{{
void mfb_get_monitor_scale (struct mfb_window* window, float* scale_x, float* scale_y) {

  HWND hWnd = 0x0;

  if (window) {
    SWindowData* window_data = (SWindowData*)window;
    SWindowData_Win* window_data_win = (SWindowData_Win*)window_data->specific;
    hWnd = window_data_win->window;
    }

  get_monitor_scale (hWnd, scale_x, scale_y);
  }
//}}}
//{{{
bool mfb_set_viewport (struct mfb_window* window, unsigned offset_x, unsigned offset_y, unsigned width, unsigned height) {

  SWindowData* window_data = (SWindowData*)window;
  SWindowData_Win* window_data_win = 0x0;

  if (!window_data)
    return false;

  if (offset_x + width > window_data->window_width)
    return false;

  if (offset_y + height > window_data->window_height)
    return false;

  window_data_win = (SWindowData_Win*)window_data->specific;

  float scale_x, scale_y;
  get_monitor_scale (window_data_win->window, &scale_x, &scale_y);
  window_data->dst_offset_x = (uint32_t)(offset_x * scale_x);
  window_data->dst_offset_y = (uint32_t)(offset_y * scale_y);

  window_data->dst_width = (uint32_t)(width  * scale_x);
  window_data->dst_height = (uint32_t)(height * scale_y);

  calc_dst_factor (window_data, window_data->window_width, window_data->window_height);

  return true;
  }
//}}}

// sync
//{{{
bool mfb_wait_sync (struct mfb_window* window) {

  if (!window)
    return false;

  SWindowData* window_data = (SWindowData*)window;
  if (window_data->close) {
    //{{{  return false
    destroy_window_data (window_data);
    return false;
    }
    //}}}

  if (g_use_hardware_sync)
    return true;

  SWindowData_Win* window_data_win = (SWindowData_Win*)window_data->specific;
  double current;

  while (true) {
    current = mfb_timer_now (window_data_win->timer);
    if (current >= g_time_for_frame) {
      mfb_timer_reset (window_data_win->timer);
      return true;
      }
    else if (g_time_for_frame - current > 2.0 / 1000.0) {
      timeBeginPeriod (1);
      Sleep (1);
      timeEndPeriod (1);

      MSG msg;
      if (PeekMessage (&msg, window_data_win->window, 0, 0, PM_REMOVE)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);

        if (window_data->close) {
          //{{{  return false
          destroy_window_data (window_data);
          return false;
          }
          //}}}
        }
      }
    }

  return true;
  }
//}}}

// timer
//{{{
void mfb_timer_init() {

  uint64_t frequency;
  QueryPerformanceFrequency ((LARGE_INTEGER*)&frequency);

  g_timer_frequency  = (double) ((int64_t) frequency);
  g_timer_resolution = 1.0 / g_timer_frequency;
  }
//}}}
//{{{
uint64_t mfb_timer_tick() {

  int64_t counter;
  QueryPerformanceCounter ((LARGE_INTEGER*) &counter);

  return counter;
  }
//}}}
