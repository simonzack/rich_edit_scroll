
#include "resource.h"
#include <Windows.h>

typedef enum {
	ID_ICON,
	ID_MENU_EXIT
} ResourceID;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message){
		case WM_USER:
			switch (lParam) {
				case WM_RBUTTONDOWN: {
					HMENU popMenu = CreatePopupMenu();
					AppendMenu(popMenu, MF_STRING, ID_MENU_EXIT, "Exit");
					SetForegroundWindow(hWnd);
					POINT pCursor;
					GetCursorPos(&pCursor);
					TrackPopupMenu(popMenu, TPM_LEFTBUTTON, pCursor.x, pCursor.y, 0, hWnd, NULL);
					return 0;
				}
			}
			break;
		case WM_COMMAND: {
			WORD wmId = LOWORD(wParam);
			WORD wmEvent = HIWORD(wParam);
			switch (wmId) {
				case ID_MENU_EXIT:
					PostQuitMessage(0);
					return 0;
			}
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	// pipes for the inject process
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	HANDLE hStdInRead, hStdInWrite;
	CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0);
	SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);
	// create the inject process
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	STARTUPINFO si = {.cb = sizeof(STARTUPINFO), .hStdInput = hStdInRead, .dwFlags = STARTF_USESTDHANDLES};
	si.dwFlags = STARTF_USESTDHANDLES;
	CreateProcess(NULL, "rich_edit_inject.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	// create notify icon
	WNDCLASS wndCls = {
		.style = 0, .lpfnWndProc = WndProc, .cbClsExtra = 0, .cbWndExtra = 0, .hInstance = hInstance, .hIcon = NULL,
		.hCursor = NULL, .hbrBackground = (HBRUSH)COLOR_WINDOW, .lpszMenuName = NULL,
		.lpszClassName = "RichEditScrollCls"
	};
	RegisterClass(&wndCls);
	HWND hWnd = CreateWindow("RichEditScrollCls", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	NOTIFYICONDATA iconData = {
		.cbSize = sizeof(iconData), .hWnd = hWnd, .uID = ID_ICON, .uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE,
		.uCallbackMessage = WM_USER, .hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOOK))
	};
	strcpy_s(iconData.szTip, 64, "Rich Edit Smooth Scroll Disabler");
	Shell_NotifyIcon(NIM_ADD, &iconData);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// signal to unhook
	CloseHandle(hStdInWrite);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	Shell_NotifyIcon(NIM_DELETE, &iconData);
}
