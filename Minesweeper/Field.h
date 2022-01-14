#pragma once

#include <vector>
#include <utility>
#include <windows.h>
#include <tchar.h>

enum cell_state {
	CELL_EMPTY = 16,
	CELL_MINE = 32,
	CELL_OPENED = 64,
	CELL_FLAG = 128,
};

class Field {
private:
	static int** field;
	static std::vector<std::pair<int, int>> mines;
	static int flag_count, hits;
	static int width, height;

	static void _setup_field(int mines_count);
	static void _clear_field();
public:
	Field() = delete;
	static void setup(int w, int h, int mines_count);
	static void clear();
	static void draw(HDC hdc, RECT window_rect, short tex_w, short tex_h, HBITMAP bitmap);
	static cell_state open(int x, int y);
	static cell_state get_state(int x, int y);
	static int put_flag(int x, int y);
	static void show_mines();
	static int get_flag_count();
};