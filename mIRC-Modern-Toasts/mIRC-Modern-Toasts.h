#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#define MIRCAPI EXTERN_C int STDAPICALLTYPE

#define MIRC_RETURN_HALT 0
#define MIRC_RETURN_CONTINUE 1
#define MIRC_RETURN_COMMAND 2
#define MIRC_RETURN_STRING 3

#define MIRC_KEEP_DLL 0
#define MIRC_UNLOAD_DLL 1

typedef struct {
   DWORD mVersion;
   HWND mHwnd;
   BOOL mKeep;
   BOOL mUnicode;
} LOADINFO;

/* The mIRC DLL API specifies that functions exported to mIRC are to have the
 * following interface:
 *
 * int __stdcall procname(
 *     HWND mWnd,
 *     HWND aWnd,
 *     char *data,
 *     char *params,
 *     BOOL show,
 *     BOOL nopause)
 *
 * mWnd is the handle to mIRC's main window
 * aWnd is the handle to the active window (eventually different to mWnd)
 * data is sent to the DLL, but the DLL can modify it to perform a command
 *     on return
 * params is filled by the DLL with parameters to use for the command sent
 *     in data
 * show specifies whether the command was called with . (FALSE) or not (TRUE)
 * nopause specifies whether the DLL must not pause mIRC--it is in a critical
 *     routine that cannot be interrupted
 * 
 * the DLL's return value is an integer:
 * . 0 - mIRC should /halt processing
 * . 1 - mIRC should continue processing
 * . 2 - the DLL filled data with a command mIRC should perform
 * . 3 - the DLL filled data with a result for $dll to return
 */
#pragma comment(linker, "/EXPORT:LoadDll=_LoadDll@4")
extern "C" void __stdcall LoadDll(LOADINFO *loadInfo);

#pragma comment(linker, "/EXPORT:UnloadDll=_UnloadDll@4")
extern "C" int __stdcall UnloadDll(int mTimeout);

#pragma comment(linker, "/EXPORT:ShowToast=_ShowToast@24")
MIRCAPI
ShowToast(HWND hMircWnd, HWND hActiveWnd, char *data, char *params, BOOL show,
    BOOL nopause);

#pragma comment(linker, "/EXPORT:SetLine1=_SetLine1@24")
MIRCAPI
SetLine1(HWND hMircWnd, HWND hActiveWnd, char *data, char *params, BOOL show,
    BOOL nopause);

#pragma comment(linker, "/EXPORT:SetLine2=_SetLine2@24")
MIRCAPI
SetLine2(HWND hMircWnd, HWND hActiveWnd, char *data, char *params, BOOL show,
    BOOL nopause);