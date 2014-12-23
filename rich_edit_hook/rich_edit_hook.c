
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	// exit if process name is incorrect

	return TRUE;
}

__declspec(dllexport) LRESULT CALLBACK hook(int nCode, WPARAM wParam, LPARAM lParam) {

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
