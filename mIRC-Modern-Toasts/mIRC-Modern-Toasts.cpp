#include "mIRC-Modern-Toasts.h"

#include <string>
#include <Psapi.h>
#include <objbase.h>
#include <ShlObj.h>
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
PWSTR pszFileMapView;

PWSTR pszDefaultAppID;
PWSTR pszAppID;

String^ line1;
String^ line2;

void InitAppUserModelID();
HRESULT TryFindAppUserModelID();
HRESULT CreateShortcut();
HRESULT _CreateShortcut(_In_ PCWSTR path);
HRESULT _SetAppIDOnShortcut(_In_ PCWSTR path);

void __stdcall LoadDll(_Inout_ LOADINFO *loadInfo)
{
    pszDefaultAppID = L"mIRC.mIRC";
    InitAppUserModelID();
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
    if (hFileMap) {
        pszFileMapView = static_cast<PWSTR>(MapViewOfFile(
            hFileMap,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            MIRC_FILE_MAP_SIZE
            ));
    }
    hWndmIRC = loadInfo->mHwnd;
    loadInfo->mKeep = true;
}

int __stdcall UnloadDll(_In_ int mTimeout)
{
    if (mTimeout == 1)
    {
        return MIRC_KEEP_DLL;
    }
    else
    {
        UnmapViewOfFile(pszFileMapView);
        CloseHandle(hFileMap);
        CoTaskMemFree(pszAppID);
        return MIRC_UNLOAD_DLL;
    }
}

void mIRCEvaluate(_In_ PCWSTR command)
{
    // This message processes the string from the file exactly as would be done
    // by the editbox. Use two slashes to evaluate the command and any
    // arguments, and use a dot to silence any errors (in case the alias does
    // not exist).
    wsprintf(pszFileMapView, L"//.%s", command);
    SendMessage(hWndmIRC, WM_MCOMMAND, MIRC_SM_UNICODE, 0);
}

void InitAppUserModelID()
{
    // If we don't succeed to find an autogenerated ID,
    // create the shortcut and assign it explicitly.
    if (FAILED(TryFindAppUserModelID())) {
        SetCurrentProcessExplicitAppUserModelID(pszDefaultAppID);
        SHStrDup(pszDefaultAppID, &pszAppID);
        CreateShortcut();
    }
}

HRESULT TryFindAppUserModelID()
{
    bool foundShortcutWithAppID = false;
    wchar_t pszExePath[MAX_PATH];
    DWORD written = GetModuleFileNameEx(GetCurrentProcess(), nullptr, pszExePath, ARRAYSIZE(pszExePath));
    HRESULT hr = written > 0 ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        ComPtr<IShellItem> spApps;
        hr = SHGetKnownFolderItem(FOLDERID_AppsFolder, static_cast<KNOWN_FOLDER_FLAG>(0), nullptr, IID_PPV_ARGS(&spApps));
        if (SUCCEEDED(hr))
        {
            ComPtr<IEnumShellItems> spEnum;
            hr = spApps->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&spEnum));
            if (SUCCEEDED(hr))
            {
                for (ComPtr<IShellItem> spAppItem;
                    !foundShortcutWithAppID && spEnum->Next(1, &spAppItem, nullptr) == S_OK;
                    spAppItem.Reset())
                {
                    ComPtr<IShellItem2> spAppItem2;
                    hr = spAppItem.As(&spAppItem2);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszTargetPath;
                        hr = spAppItem2->GetString(PKEY_Link_TargetParsingPath, &pszTargetPath);
                        // an app may not necessarily have a target, and this will fail.
                        // if that's the case, it's certainly not the one we're looking for
                        if (SUCCEEDED(hr)) {
                            if (CompareStringOrdinal(pszExePath, wcslen(pszExePath), pszTargetPath, wcslen(pszTargetPath), TRUE) == CSTR_EQUAL) {
                                foundShortcutWithAppID = true;
                                hr = spAppItem2->GetString(PKEY_AppUserModel_ID, &pszAppID);
                            }
                            CoTaskMemFree(pszTargetPath);
                        }
                    }
                }
            }
        }
    }
    if (!foundShortcutWithAppID) {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT CreateShortcut()
{
    HRESULT hr;
    wchar_t pszShortcutPath[MAX_PATH];
    DWORD written = GetEnvironmentVariable(L"APPDATA", pszShortcutPath, ARRAYSIZE(pszShortcutPath));
    hr = written > 0 ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(hr)) {
        errno_t concat = wcscat_s(pszShortcutPath, ARRAYSIZE(pszShortcutPath),
            L"\\Microsoft\\Windows\\Start Menu\\Programs\\mIRC with Modern Toasts.lnk");
        hr = concat == 0 ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr)) {
            hr = _CreateShortcut(pszShortcutPath);
            if (SUCCEEDED(hr)) {
                hr = _SetAppIDOnShortcut(pszShortcutPath);
            }
        }
    }
    return hr;
}

HRESULT _CreateShortcut(_In_ PCWSTR path)
{
    HRESULT hr;
    wchar_t pathToExe[MAX_PATH];
    DWORD written = GetModuleFileNameEx(GetCurrentProcess(), nullptr, pathToExe, ARRAYSIZE(pathToExe));
    hr = written > 0 ? S_OK : E_FAIL;
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
    return hr;
}

HRESULT _SetAppIDOnShortcut(_In_ PCWSTR path)
{
    ComPtr<IPropertyStore> store;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path, nullptr, GPS_READWRITE, IID_PPV_ARGS(&store));
    if (SUCCEEDED(hr))
    {
        PROPVARIANT pv;
        hr = InitPropVariantFromString(pszDefaultAppID, &pv);
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
SetLine1(
    _In_ HWND hMircWnd,
    _In_ HWND hActiveWnd,
    _Inout_ char *data,
    _Inout_ char *params,
    _In_ BOOL show,
    _In_ BOOL nopause)
{
    int num_wchar = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
    wchar_t *wstr = new wchar_t[num_wchar];
    MultiByteToWideChar(CP_UTF8, 0, data, -1, wstr, num_wchar);
    line1 = ref new String(wstr);
    delete[] wstr;
    return MIRC_RETURN_CONTINUE;
}

MIRCAPI
SetLine2(
    _In_ HWND hMircWnd,
    _In_ HWND hActiveWnd,
    _Inout_ char *data,
    _Inout_ char *params,
    _In_ BOOL show,
    _In_ BOOL nopause)
{
    // we're going to ignore the copy-paste here ok?
    int num_wchar = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
    wchar_t *wstr = new wchar_t[num_wchar];
    MultiByteToWideChar(CP_UTF8, 0, data, -1, wstr, num_wchar);
    line2 = ref new String(wstr);
    delete[] wstr;
    return MIRC_RETURN_CONTINUE;
}

MIRCAPI
ShowToast(
    _In_ HWND hMircWnd,
    _In_ HWND hActiveWnd,
    _Inout_ char *data,
    _Inout_ char *params,
    _In_ BOOL show,
    _In_ BOOL nopause)
{
    // called with $dllrun (ie in a new thread) so we must initialize COM ourselves
    RoInitialize(RO_INIT_MULTITHREADED);

    auto toastXml = ToastNotificationManager::GetTemplateContent(ToastTemplateType::ToastText02);
    auto xmlNodes = toastXml->GetElementsByTagName("text");
    xmlNodes->Item(0)->AppendChild(toastXml->CreateTextNode(line1));
    xmlNodes->Item(1)->AppendChild(toastXml->CreateTextNode(line2));
    auto toast = ref new ToastNotification(toastXml);

    bool isWaiting = true;
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

    bool trueValue = true;
    // wait so the thread stays alive so it can receive the toast events
    // wait indefinitely--if the user mouses over the notification it could
    // be there for a very very long time
    WaitOnAddress(&isWaiting, &trueValue, sizeof(isWaiting), INFINITE);

    RoUninitialize();
    return MIRC_RETURN_CONTINUE;
}

MIRCAPI
GetAppID(
    _In_ HWND hMircWnd,
    _In_ HWND hActiveWnd,
    _Inout_ char *data,
    _Inout_ char *params,
    _In_ BOOL show,
    _In_ BOOL nopause)
{
    wcstombs(data, pszAppID, 900);
    return MIRC_RETURN_STRING;
}