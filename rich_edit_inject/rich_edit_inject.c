
#include <Windows.h>
#include <Tchar.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	PTSTR name = sizeof(void*) == 8 ? _T("rich_edit_hook_64.dll") : _T("rich_edit_hook_32.dll");
	HINSTANCE hMod = LoadLibrary(name);
	if (hMod == NULL)
		return 1;
	FARPROC proc = GetProcAddress(hMod, "hook");
	// WH_CBT is the least intrusive hook type
	HHOOK hHook = SetWindowsHookEx(WH_CBT, (HOOKPROC)proc, hMod, 0);
	FreeLibrary(hMod);
	// wait for signal to unhook
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	ReadFile(hStdIn, NULL, 0, NULL, NULL);
	UnhookWindowsHookEx(hHook);
	return 0;
}
