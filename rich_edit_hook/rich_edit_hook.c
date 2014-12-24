
#include <Windows.h>
#include "MinHook.h"

#pragma comment(lib, "libMinHook.lib")

typedef HMODULE(WINAPI *LOADLIBRARYEXW)(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);

BOOL matched = FALSE;
FARPROC aRichEditWndProc;
WNDPROC fpRichEditWndProc = NULL;
FARPROC aLoadLibraryExW;
LOADLIBRARYEXW fpLoadLibraryExW = NULL;

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
	if (MH_CreateHook(&aRichEditWndProc, &RichEditWndProcHook, (LPVOID*)&fpRichEditWndProc) != MH_OK)
		return;
	if (MH_EnableHook(&aRichEditWndProc) != MH_OK)
		return;
}

HMODULE LoadLibraryExWHook(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags) {
	HMODULE res = fpLoadLibraryExW(lpFileName, hFile, dwFlags);
	if (res == NULL)
		return res;
	if (wcscmp(lpFileName, L"msftedit") == 0 || wcscmp(lpFileName, L"msftedit.dll") == 0)
		HookRichEditWndProc(res);
	MH_DisableHook(&LoadLibraryExW);
	return res;
}

void HookLoadLibrary(){
	// all the LoadLibrary variants eventually call LoadLibraryExW
	HINSTANCE hinstKernelBase = GetModuleHandle("kernelbase.dll");
	aLoadLibraryExW = GetProcAddress(hinstKernelBase, "LoadLibraryExW");
	if (MH_CreateHook(&aLoadLibraryExW, &LoadLibraryExWHook, (LPVOID*)&fpLoadLibraryExW) != MH_OK)
		return;
	if (MH_EnableHook(&aLoadLibraryExW) != MH_OK)
		return;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH: {
			// don't hook if process name is incorrect
			const TCHAR* matches[] = { "sandbox.exe" };
			TCHAR fileName[MAX_PATH];
			GetModuleFileName(NULL, fileName, sizeof(fileName));
			for (int i = 0; i < sizeof(matches)/sizeof(*matches); i++){
				const TCHAR* match = matches[i];
				matched |= (
					strlen(fileName) > strlen(match) && fileName[strlen(fileName) - strlen(match) - 1] == '\\' &&
					strcmp(fileName + strlen(fileName) - strlen(match), match) == 0
				);
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

__declspec(dllexport) LRESULT CALLBACK hook(int nCode, WPARAM wParam, LPARAM lParam) {
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
