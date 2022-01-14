#include "framework.h"
#include <ctime>
#include <chrono>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define TILE_COUNT 20
#define TILE_W 32
#define TILE_H 32
#define WINDOW_WIDTH TILE_COUNT * TILE_W
#define WINDOW_HEIGHT TILE_COUNT * TILE_H

#include "Field.h"

// Глобальні змінні:
HINSTANCE hInst; 	//Дескриптор програми	
HWND mainWindow;
HBITMAP bitmap;
LPCTSTR szWindowClass = L"QWERTY";
LPCTSTR szTitle = L"Bilash";
LPCTSTR box_msg;
static unsigned int ms_timer;
static unsigned int mine_count = TILE_COUNT;

bool game_over;
VOID GameOver(BOOL);

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void _blit_image(HDC hdc, HBITMAP bitmap, RECT size, RECT pos);
static void get_time(int& m, int& s);
BOOL WINAPI AboutDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	MyRegisterClass(hInstance);
	INITCOMMONCONTROLSEX init_ctrl;
	init_ctrl.dwSize = sizeof(init_ctrl);
	init_ctrl.dwICC = ICC_LINK_CLASS | ICC_INTERNET_CLASSES;
	BOOL b = InitCommonControlsEx(&init_ctrl);
	int i = GetLastError();

	ms_timer = 0;
	Field::setup(TILE_COUNT, TILE_COUNT, mine_count);
	game_over = false;
	// Створення вікна програми
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}
	bitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, 0);
	InvalidateRect(mainWindow, NULL, FALSE);

	bool exit = false;
	unsigned int stamp = 0;
	while (!exit)
	{
		unsigned int start = GetTickCount64();
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			WndProc(mainWindow, msg.message, msg.wParam, msg.lParam);
			if (msg.message == WM_QUIT) {
				exit = true;
			}
		}

		if (game_over) {
			MessageBox(mainWindow, box_msg, L"Игра завершена", MB_OK);
			game_over = false;
			ms_timer = 0;
			Field::clear();
			Field::setup(TILE_COUNT, TILE_COUNT, mine_count);
			InvalidateRect(mainWindow, NULL, FALSE);
			continue;
		}

		unsigned int end = GetTickCount64();
		if (!game_over) {
			stamp += end - start;
			if (stamp > 200) {
				ms_timer += stamp;
				stamp = 0;
				InvalidateRect(mainWindow, NULL, FALSE);
			}
		}
	}
	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW; 		//стиль вікна
	wcex.lpfnWndProc = (WNDPROC)WndProc; 		//віконна процедура
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance; 			//дескриптор програми
	wcex.hIcon = LoadIcon(NULL, IDI_QUESTION); 		//визначення іконки
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW); 	//визначення курсору
	wcex.hbrBackground = GetSysColorBrush(COLOR_WINDOW + 1); //установка фону
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_MINESWEEPER); 				//визначення меню
	wcex.lpszClassName = szWindowClass; 		//ім’я класу
	wcex.hIconSm = NULL;

	return RegisterClassEx(&wcex); 			//реєстрація класу вікна
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance; //зберігає дескриптор додатка в змінній hInst
	hWnd = CreateWindow(szWindowClass, 	// ім’я класу вікна
		szTitle, 				// назва програми
		WS_OVERLAPPEDWINDOW,			// стиль вікна
		450, 			// положення по Х	
		0,			// положення по Y	
		WINDOW_WIDTH + 16,            // розмір x
		WINDOW_HEIGHT + 45 + TILE_H * 3, 			// розмір по Y
		NULL, 					// дескриптор батьківського вікна	
		NULL, 					// дескриптор меню вікна
		hInstance, 				// дескриптор програми
		NULL); 				// параметри створення.

	if (!hWnd) 	//Якщо вікно не cтворилось, функція повертає FALSE
	{
		return FALSE;
	}
	ShowWindow(hWnd, nCmdShow); 		//Показати вікно
	UpdateWindow(hWnd); 				//Оновити вікно
	mainWindow = hWnd;
	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	static HDC hMemDC = 0;
	static HBITMAP hMemBitmap = 0;
	static HBRUSH fieldbrush = CreateSolidBrush(RGB(192, 192, 192));
	static RECT flag_rt = { 32, 0, 16, 16 };

	switch (message)
	{
	case WM_CREATE:
		break;
	case WM_RBUTTONDOWN: {
		int x = LOWORD(lParam) / TILE_W;
		int y = HIWORD(lParam) / TILE_H;
		if (x >= 0 && x < TILE_COUNT && y >= 0 && y < TILE_COUNT) {
			int hits = Field::put_flag(x, y);
			if (hits == mine_count) {
				GameOver(FALSE);
				Field::show_mines();
			}
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	}
	case WM_LBUTTONDOWN: {
		int x = LOWORD(lParam) / TILE_W;
		int y = HIWORD(lParam) / TILE_H;
		if (x >= 0 && x < TILE_COUNT && y >= 0 && y < TILE_COUNT) {
			cell_state state = Field::get_state(x, y);

			if (state & cell_state::CELL_MINE) {
				GameOver(TRUE);
				Field::show_mines();
			} else if (!(state & cell_state::CELL_FLAG)) {
				Field::open(x, y);
			}
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlg);
				break;
			case IDM_MIDDLE: {
				mine_count = TILE_COUNT + 10;
				Field::clear();
				Field::setup(TILE_COUNT, TILE_COUNT, mine_count);
				break;
			}
			case IDM_EASY: {
				mine_count = TILE_COUNT;
				Field::clear();
				Field::setup(TILE_COUNT, TILE_COUNT, mine_count);
				break;
			}
			case IDM_HARD: {
				mine_count = TILE_COUNT * 2;
				Field::clear();
				Field::setup(TILE_COUNT, TILE_COUNT, mine_count);
				break;
			}
		}
		break;
	case WM_PAINT: {
		HDC hdc = BeginPaint(hWnd, &ps); 	//Почати графічний вивід	

		if (hMemDC == 0) {
			hMemDC = CreateCompatibleDC(hdc);
			hMemBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH + 16, WINDOW_HEIGHT + 25 + TILE_H * 3);
			SelectObject(hMemDC, hMemBitmap);
			SetBkMode(hMemDC, TRANSPARENT);

			HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			LOGFONT logfont;
			GetObject(hFont, sizeof(LOGFONT), &logfont);
			logfont.lfHeight = -MulDiv(TILE_W / 1.2, GetDeviceCaps(hMemDC, LOGPIXELSY), 72);
			SelectObject(hMemDC, CreateFontIndirect(&logfont));
		}

		RECT rt;
		char buf[10];
		GetClientRect(hWnd, &rt); 		//Область вікна для малювання
		FillRect(hMemDC, &rt, fieldbrush);
		Field::draw(hMemDC, rt, TILE_W, TILE_H, bitmap);
		_blit_image(hMemDC, bitmap, flag_rt, {TILE_W * 2, WINDOW_HEIGHT + TILE_H, TILE_W, TILE_H});
		sprintf_s(buf, "%d", Field::get_flag_count());
		TextOutA(hMemDC, TILE_W * 3 + 2, WINDOW_HEIGHT + TILE_H, buf, strlen(buf));
		int m = 0, s = 0;
		get_time(m, s);
		sprintf_s(buf, "%02d:%02d", m, s);
		TextOutA(hMemDC, WINDOW_WIDTH - TILE_W * 4, WINDOW_HEIGHT + TILE_H, buf, strlen(buf));
		BitBlt(hdc, 0, 0, rt.right - rt.left, rt.bottom - rt.top, hMemDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps); 		//Закінчити графічний вивід	
		break;
	}
	case WM_DESTROY: 				//Завершення роботи
		Field::clear();
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		return 1;
	default:
		//Обробка повідомлень, які не оброблені користувачем
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

VOID GameOver(BOOL fail) {
	game_over = true;
	if (fail) {
		box_msg = L"Вы проиграли!";
	} else {
		box_msg = L"Вы выиграли!";
	}
}

BOOL WINAPI AboutDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HBITMAP bmp;
	switch (uMsg)
	{
	case WM_CLOSE:
		EndDialog(hwnd, 0);
		DestroyWindow(hwnd);
		return TRUE;

	case WM_INITDIALOG: {
		bmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		SendDlgItemMessage(hwnd, IDC_STATICBMP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp);
		int i = GetLastError();
		return TRUE;
	}

	case WM_NOTIFY: {
		HWND g_hLink = GetDlgItem(hwnd, IDC_SYSLINK2);
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_CLICK:
		case NM_RETURN:
		{
			PNMLINK pNMLink = (PNMLINK)lParam;
			LITEM   item = pNMLink->item;

			if ((((LPNMHDR)lParam)->hwndFrom == g_hLink) && (item.iLink == 0))
			{
				ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
			}
			break;
		}
		}
		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: EndDialog(hwnd, 0); break;
		}
		return TRUE;
	}
	return FALSE;
}

static void _blit_image(HDC hdc, HBITMAP bitmap, RECT size, RECT pos) {
	HDC image_dc = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap = SelectObject(image_dc, bitmap);
	StretchBlt(hdc,
		pos.left, pos.top,
		pos.right, pos.bottom,
		image_dc,
		size.left, size.top,
		size.right, size.bottom, SRCCOPY);
	SelectObject(image_dc, oldBitmap);
	DeleteDC(image_dc);
}

static void get_time(int& m, int& s) {
	auto d = std::chrono::milliseconds(ms_timer);
	auto mc = std::chrono::duration_cast<std::chrono::minutes>(d);
	d -= mc;
	auto sc = std::chrono::duration_cast<std::chrono::seconds>(d);
	m = mc.count();
	s = sc.count();
}