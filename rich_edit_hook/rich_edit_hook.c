
#include <Windows.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include "MinHook.h"

#pragma comment(lib, "libMinHook.lib")
#pragma comment(lib, "Shlwapi.lib")

typedef HMODULE(WINAPI *LOADLIBRARYEXW)(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);

int scroll_speed;

FARPROC aRichEditWndProc;
WNDPROC fpRichEditWndProc = NULL;
FARPROC aLoadLibraryExW;
LOADLIBRARYEXW fpLoadLibraryExW = NULL;

BOOL MatchFileNameWStr(const wchar_t* fileName, const wchar_t* match) {
	if (wcslen(fileName) > wcslen(match) && fileName[wcslen(fileName) - wcslen(match) - 1] != L'\\')
		return FALSE;
	return wcslen(fileName) >= wcslen(match) && _wcsicmp(fileName + wcslen(fileName) - wcslen(match), match) == 0;
}

LRESULT CALLBACK Scroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT res;
	short delta = GET_WHEEL_DELTA_WPARAM(wParam);
	for (int i = 0; i < scroll_speed; i++) {
		if (delta > 0) {
			res = fpRichEditWndProc(hWnd, WM_VSCROLL, SB_LINEUP, 0);
		} else {
			res = fpRichEditWndProc(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
		}
	}
	return res;
}

LRESULT CALLBACK RichEditWndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_MOUSEWHEEL:
			return Scroll(hWnd, message, wParam, lParam);
	}
	return fpRichEditWndProc(hWnd, message, wParam, lParam);
}

void HookRichEditWndProc(HINSTANCE hinstRichEdit){
	aRichEditWndProc = GetProcAddress(hinstRichEdit, "RichEditWndProc");
	if (MH_CreateHook(aRichEditWndProc, &RichEditWndProcHook, (LPVOID*)&fpRichEditWndProc) != MH_OK)
		return;
	if (MH_EnableHook(aRichEditWndProc) != MH_OK)
		return;
}

HMODULE LoadLibraryExWHook(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags) {
	HMODULE res = fpLoadLibraryExW(lpFileName, hFile, dwFlags);
	if (res == NULL)
		return res;
	if (MatchFileNameWStr(lpFileName, L"msftedit") == 0 || MatchFileNameWStr(lpFileName, L"msftedit.dll") == 0)
		HookRichEditWndProc(res);
	MH_DisableHook(&LoadLibraryExW);
	return res;
}

void HookLoadLibrary(){
	// all the LoadLibrary variants eventually call kernelbase!LoadLibraryExW
	HINSTANCE hinstKernelBase = GetModuleHandle("kernelbase.dll");
	aLoadLibraryExW = GetProcAddress(hinstKernelBase, "LoadLibraryExW");
	if (MH_CreateHook(aLoadLibraryExW, &LoadLibraryExWHook, (LPVOID*)&fpLoadLibraryExW) != MH_OK)
		return;
	if (MH_EnableHook(aLoadLibraryExW) != MH_OK)
		return;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	static BOOL matched = FALSE;
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH: {
			// ini
			char iniPath[MAX_PATH];
			SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, iniPath);
			PathAppend(iniPath, "rich_edit_scroll.ini");
			scroll_speed = GetPrivateProfileInt("default", "scroll_speed", 3, iniPath);
			// don't hook if process name is incorrect
			char filePath[MAX_PATH];
			GetModuleFileName(NULL, filePath, sizeof(filePath));
			char* fileName = PathFindFileName(filePath);
			char buffer[0x1000];
			GetPrivateProfileSectionNames(buffer, sizeof(buffer), iniPath);
			char* search = buffer;
			while (*search) {
				if (_stricmp(search, fileName) == 0) {
					matched = TRUE;
					break;
				}
				search = strchr(search, '\0') + 1;
			}
			if (!matched)
				break;
			scroll_speed = GetPrivateProfileInt(fileName, "scroll_speed", scroll_speed, iniPath);
			// hook RichEditWndProc
			if (MH_Initialize() != MH_OK)
				break;
			HINSTANCE hinstRichEdit = GetModuleHandle("msftedit.dll");
			if (hinstRichEdit == NULL)
				HookLoadLibrary();
			else
				HookRichEditWndProc(hinstRichEdit);
			break;
		}
		case DLL_PROCESS_DETACH:
			if (!matched)
				break;
			if (MH_Uninitialize() != MH_OK)
				break;
			break;
	}
	return TRUE;
}

LRESULT CALLBACK hook(int nCode, WPARAM wParam, LPARAM lParam) {
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
