
#include <Windows.h>
#include "MinHook.h"

#pragma comment(lib, "libMinHook.lib")

typedef HMODULE(WINAPI *LOADLIBRARYEXW)(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);

FARPROC aRichEditWndProc;
WNDPROC fpRichEditWndProc = NULL;
FARPROC aLoadLibraryExW;
LOADLIBRARYEXW fpLoadLibraryExW = NULL;

BOOL MatchFileNameStr(const char* fileName, const char* match) {
	if (strlen(fileName) > strlen(match) && fileName[strlen(fileName) - strlen(match) - 1] != '\\')
		return FALSE;
	return strlen(fileName) >= strlen(match) && _stricmp(fileName + strlen(fileName) - strlen(match), match) == 0;
}

BOOL MatchFileNameWStr(const wchar_t* fileName, const wchar_t* match) {
	if (wcslen(fileName) > wcslen(match) && fileName[wcslen(fileName) - wcslen(match) - 1] != L'\\')
		return FALSE;
	return wcslen(fileName) >= wcslen(match) && _wcsicmp(fileName + wcslen(fileName) - wcslen(match), match) == 0;
}

LRESULT CALLBACK Scroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	short delta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (delta > 0) {
		return fpRichEditWndProc(hWnd, WM_VSCROLL, SB_LINEUP, 0);
	} else {
		return fpRichEditWndProc(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
	}
}

LRESULT CALLBACK RichEditWndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_MOUSEWHEEL:
			return Scroll(hWnd, message, wParam, lParam);
		case WM_VSCROLL:
			switch (LOWORD(wParam)) {
				case SB_THUMBTRACK:
					return Scroll(hWnd, message, wParam, lParam);
			}
			break;
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
			// don't hook if process name is incorrect
			const char* matches[] = { "wordpad.exe" };
			char fileName[MAX_PATH];
			GetModuleFileName(NULL, fileName, sizeof(fileName));
			for (int i = 0; i < sizeof(matches)/sizeof(*matches); i++){
				const char* match = matches[i];
				matched |= MatchFileNameStr(fileName, match);
			}
			if (!matched)
				break;
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
