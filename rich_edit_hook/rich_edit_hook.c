
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	return TRUE;
}

__declspec(dllexport) LRESULT CALLBACK hook(int nCode, WPARAM wParam, LPARAM lParam) {
	// exit if process name is incorrect

}
