
#include <Windows.h>
#include "MinHook.h"

#pragma comment(lib, "libMinHook.lib")

BOOL matched = FALSE;
WNDPROC fpRichEditWndProc = NULL;

LRESULT CALLBACK RichEditWndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// XXX
	return fpRichEditWndProc(hWnd, message, wParam, lParam);
}

void HookRichEditWndProc(HINSTANCE hinstRichEdit){
	FARPROC RichEditWndProc = GetProcAddress(hinstRichEdit, "RichEditWndProc");
	if (MH_CreateHook(&RichEditWndProc, &RichEditWndProcHook, (LPVOID*)&fpRichEditWndProc) != MH_OK)
		return;
	if (MH_EnableHook(&RichEditWndProc) != MH_OK)
		return;
}

void HookLoadLibrary(){
	// XXX
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH: {
			// don't hook if process name is incorrect
			const TCHAR* matches[] = { "rich_edit_scroll.exe", "sandbox.exe" };
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
