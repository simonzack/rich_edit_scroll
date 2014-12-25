
#include <Windows.h>
#include <Tchar.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	HINSTANCE hMod = LoadLibrary(_T("rich_edit_hook.dll"));
	FARPROC proc = GetProcAddress(hMod, "hook");
	// WH_CBT is the least intrusive hook type
	HHOOK hHook = SetWindowsHookEx(WH_CBT, (HOOKPROC)proc, hMod, 0);
	FreeLibrary(hMod);
	// wait for signal to unhook
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	ReadFile(hStdIn, NULL, 0, NULL, NULL);
	UnhookWindowsHookEx(hHook);
}
