#include "mIRC-Modern-Toasts.h"

#include <Psapi.h>
#include <objbase.h>
#include <ShObjIdl.h>
#include <propvarutil.h>
#include <propkey.h>

#include <roapi.h>
#include <wrl\client.h>

using namespace Windows::Foundation;
using namespace Windows::UI::Notifications;
using namespace Windows::Data::Xml::Dom;
using namespace Microsoft::WRL;
using namespace Platform;

#define MIRC_FILE_MAP_SIZE 4096

HWND hWndmIRC;
HANDLE hFileMap;
LPWSTR pFileMapView;

LPWSTR defaultAppID;
LPWSTR pszAppID;

String^ line1;
String^ line2;

HRESULT CreateShortcut();
HRESULT _CreateShortcutIfNotExists(LPWSTR path, rsize_t size);
HRESULT _SetAppIDOnShortcut(LPWSTR path);

void __stdcall LoadDll(LOADINFO *loadInfo)
{
    defaultAppID = L"mIRC.mIRC";
    HRESULT hr = GetCurrentProcessExplicitAppUserModelID(&pszAppID);
    if (FAILED(hr))
    {
        hr = CreateShortcut();
        if (SUCCEEDED(hr)) {
            pszAppID = defaultAppID;
        }
    }
    // set up the memory file mapping so we can call our
    // callback alias with SendMessage
    hFileMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        MIRC_FILE_MAP_SIZE,
        L"mIRC"
    );
    pFileMapView = (LPWSTR)MapViewOfFile(
        hFileMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        MIRC_FILE_MAP_SIZE
    );
    hWndmIRC = loadInfo->mHwnd;
    loadInfo->mKeep = true;
}

int __stdcall UnloadDll(int mTimeout)
{
    if (mTimeout == 1)
    {
        return MIRC_KEEP_DLL;
    }
    else
    {
        UnmapViewOfFile(pFileMapView);
        CloseHandle(hFileMap);
        return MIRC_UNLOAD_DLL;
    }
}

void mIRCEvaluate(LPWSTR command)
{
    // This message processes the string from the file exactly as would be done
    // by the editbox. Use two slashes to evaluate the command and any
    // arguments, and use a dot to silence any errors (in case the alias does
    // not exist).
    wsprintf(pFileMapView, L"//.%s", command);
    SendMessage(hWndmIRC, WM_MCOMMAND, MIRC_SM_UNICODE, 0);
}

HRESULT CreateShortcut()
{
    wchar_t path[MAX_PATH];
    DWORD written = GetEnvironmentVariable(L"APPDATA", path, MAX_PATH);
    HRESULT hr = written > 0 ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(hr)) {
        errno_t concat = wcscat_s(path, ARRAYSIZE(path), L"\\Microsoft\\Windows\\Start Menu\\Programs\\mIRC.lnk");
        hr = concat == 0 ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr)) {
            hr = _CreateShortcutIfNotExists(path, ARRAYSIZE(path));
            if (SUCCEEDED(hr)) {
                hr = _SetAppIDOnShortcut(path);
            }
        }
    }
    return hr;
}

HRESULT _CreateShortcutIfNotExists(LPWSTR path, rsize_t size)
{
    DWORD attributes = GetFileAttributes(path);
    bool exists = attributes < 0xFFFFFFF;
    HRESULT hr = S_OK;
    if (!exists)
    {
        wchar_t pathToExe[MAX_PATH];
        DWORD written = GetModuleFileNameEx(GetCurrentProcess(), nullptr, pathToExe, ARRAYSIZE(pathToExe));
        HRESULT hr = written > 0 ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            ComPtr<IShellLink> shellLink;
            hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
            if (SUCCEEDED(hr))
            {
                hr = shellLink->SetPath(pathToExe);
                if (SUCCEEDED(hr))
                {
                    hr = shellLink->SetArguments(L"");
                    if (SUCCEEDED(hr))
                    {
                        ComPtr<IPersistFile> persistFile;
                        hr = shellLink.As(&persistFile);
                        if (SUCCEEDED(hr))
                        {
                            hr = persistFile->Save(path, TRUE);
                        }
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT _SetAppIDOnShortcut(LPWSTR path)
{
    ComPtr<IPropertyStore> store;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path, nullptr, GPS_READWRITE, IID_PPV_ARGS(&store));
    if (SUCCEEDED(hr))
    {
        PROPVARIANT pv;
        hr = InitPropVariantFromString(defaultAppID, &pv);
        if (SUCCEEDED(hr))
        {
            hr = store->SetValue(PKEY_AppUserModel_ID, pv);
            if (SUCCEEDED(hr))
            {
                hr = store->Commit();
            }
            PropVariantClear(&pv);
        }
    }
    return hr;
}

MIRCAPI
SetLine1(HWND hMircWnd, HWND hActiveWnd, char *data, char *params, BOOL show,
    BOOL nopause)
{
    int num_wchar = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
    wchar_t *wstr = new wchar_t[num_wchar];
    MultiByteToWideChar(CP_UTF8, 0, data, -1, wstr, num_wchar);
    line1 = ref new String(wstr);

    return MIRC_RETURN_CONTINUE;
}

MIRCAPI
SetLine2(HWND hMircWnd, HWND hActiveWnd, char *data, char *params, BOOL show,
    BOOL nopause)
{
    // we're going to ignore the copy-paste here ok?
    int num_wchar = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
    wchar_t *wstr = new wchar_t[num_wchar];
    MultiByteToWideChar(CP_UTF8, 0, data, -1, wstr, num_wchar);
    line2 = ref new String(wstr);

    return MIRC_RETURN_CONTINUE;
}

MIRCAPI
ShowToast(HWND hMircWnd, HWND hActiveWnd, char *data, char *params, BOOL show,
    BOOL nopause)
{
    // called with $dllrun (ie in a new thread) so we must initialize COM ourselves
    RoInitialize(RO_INIT_MULTITHREADED);

    auto toastXml = ToastNotificationManager::GetTemplateContent(ToastTemplateType::ToastText02);
    auto xmlNodes = toastXml->GetElementsByTagName("text");
    xmlNodes->Item(0)->AppendChild(toastXml->CreateTextNode(line1));
    xmlNodes->Item(1)->AppendChild(toastXml->CreateTextNode(line2));
    auto toast = ref new ToastNotification(toastXml);

    bool isWaiting = true;
    bool trueValue = true;
    toast->Activated += ref new TypedEventHandler<ToastNotification^, Object^>(
        [&isWaiting](ToastNotification^ toast, Object^ result) {
            mIRCEvaluate(L"mmt-toast-activate-callback");
            isWaiting = false;
            WakeByAddressAll(&isWaiting);
        }
    );
    toast->Dismissed += ref new TypedEventHandler<ToastNotification^, ToastDismissedEventArgs^>(
        [&isWaiting](ToastNotification^ toast, ToastDismissedEventArgs^ e) {
            mIRCEvaluate(L"mmt-toast-dismiss-callback");
            isWaiting = false;
            WakeByAddressAll(&isWaiting);
        }
    );
    toast->Failed += ref new TypedEventHandler<ToastNotification^, ToastFailedEventArgs^>(
        [&isWaiting](ToastNotification^ toast, ToastFailedEventArgs^ e) {
            mIRCEvaluate(L"mmt-toast-fail-callback");
            isWaiting = false;
            WakeByAddressAll(&isWaiting);
        }
    );
    ToastNotificationManager::CreateToastNotifier(ref new String(pszAppID))->Show(toast);

    // wait so the thread stays alive so it can receive the toast events
    while (isWaiting)
    {
        // wait indefinitely--if the user mouses over the notification it could
        // be there for a very very long time
        if (!WaitOnAddress(&isWaiting, &trueValue, sizeof(bool), -1)) {
            break;
        }
    }

    RoUninitialize();
    return MIRC_RETURN_CONTINUE;
}