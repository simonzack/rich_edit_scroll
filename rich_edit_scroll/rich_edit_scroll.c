
#include <Windows.h>
#include <Tchar.h>
#include "resource.h"

typedef enum {
	ID_ICON,
	ID_MENU_EXIT
} ResourceID;

BOOL Is64BitWindows() {
#if defined(_WIN64)
	return TRUE;
#elif defined(_WIN32)
	BOOL f64 = FALSE;
	return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
	return FALSE;
#endif
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message){
		case WM_USER:
			switch (lParam) {
				case WM_RBUTTONDOWN: {
					HMENU popMenu = CreatePopupMenu();
					AppendMenu(popMenu, MF_STRING, ID_MENU_EXIT, _T("Exit"));
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

BOOL CreateInjectProcess(PHANDLE phStdInWrite, PPROCESS_INFORMATION ppi, PTSTR name) {
	// pipes for the inject process
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	HANDLE hStdInRead;
	CreatePipe(&hStdInRead, phStdInWrite, &saAttr, 0);
	SetHandleInformation(*phStdInWrite, HANDLE_FLAG_INHERIT, 0);
	// create the inject process
	ZeroMemory(ppi, sizeof(PROCESS_INFORMATION));
	STARTUPINFO si = {.cb = sizeof(STARTUPINFO), .hStdInput = hStdInRead, .dwFlags = STARTF_USESTDHANDLES};
	si.dwFlags = STARTF_USESTDHANDLES;
	return CreateProcess(NULL, name, NULL, NULL, TRUE, 0, NULL, NULL, &si, ppi);
}

void CloseInjectProcess(HANDLE hStdInWrite, PPROCESS_INFORMATION ppi) {
	CloseHandle(hStdInWrite);
	WaitForSingleObject(ppi->hProcess, INFINITE);
	CloseHandle(ppi->hProcess);
	CloseHandle(ppi->hThread);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	BOOL created32 = FALSE, created64 = FALSE;
	HANDLE hStdInWrite32 = NULL, hStdInWrite64 = NULL;
	PROCESS_INFORMATION pi32, pi64;
	if (Is64BitWindows())
		created64 = CreateInjectProcess(&hStdInWrite64, &pi64, _tcsdup(_T("rich_edit_inject_64.exe")));
	created32 = CreateInjectProcess(&hStdInWrite32, &pi32, _tcsdup(_T("rich_edit_inject_32.exe")));
	// create notify icon
	WNDCLASS wndCls = {
		.style = 0, .lpfnWndProc = WndProc, .cbClsExtra = 0, .cbWndExtra = 0, .hInstance = hInstance, .hIcon = NULL,
		.hCursor = NULL, .hbrBackground = (HBRUSH)COLOR_WINDOW, .lpszMenuName = NULL,
		.lpszClassName = _T("RichEditScrollCls")
	};
	RegisterClass(&wndCls);
	HWND hWnd = CreateWindow(_T("RichEditScrollCls"), NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	NOTIFYICONDATA iconData = {
		.cbSize = sizeof(iconData), .hWnd = hWnd, .uID = ID_ICON, .uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE,
		.uCallbackMessage = WM_USER, .hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOOK))
	};
	_tcscpy_s(iconData.szTip, sizeof(iconData.szTip) / sizeof(*iconData.szTip), _T("Rich Edit Smooth Scroll Disabler"));
	Shell_NotifyIcon(NIM_ADD, &iconData);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// signal to unhook
	if (created32)
		CloseInjectProcess(hStdInWrite32, &pi32);
	if (created64)
		CloseInjectProcess(hStdInWrite64, &pi64);
	Shell_NotifyIcon(NIM_DELETE, &iconData);
}
