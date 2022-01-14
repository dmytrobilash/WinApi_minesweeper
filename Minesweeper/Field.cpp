#include "Field.h"

void Field::setup(int w, int h, int mines_count) {
	Field::width = w;
	Field::height = h;
	Field::flag_count = mines_count;
	Field::hits = 0;
	Field::_setup_field(mines_count);
}

void Field::clear() {
	Field::_clear_field();
	Field::mines.clear();
}

static void _blit_image(HDC hdc, HBITMAP bitmap, RECT size, RECT pos);

void Field::draw(HDC hdc, RECT window_rect, short tex_w, short tex_h, HBITMAP bitmap) {
	static RECT empty_rt = { 0, 0, 16, 16 };
	static RECT opened_rt = { 16, 0, 16, 16 };
	static RECT flag_rt = { 32, 0, 16, 16 };
	static RECT mine_rt = { 96, 0,  16, 16 };
	static RECT mine_inac_rt = { 112, 0,  16, 16 };
	static RECT num_rt = { 0, 16, 16, 16 };

	for (short i = 0; i < height; i++) {
		for (short j = 0; j < width; j++) {
			if (field[i][j] & cell_state::CELL_OPENED) {
				if (Field::field[i][j] & cell_state::CELL_MINE) {
					if (Field::field[i][j] & CELL_FLAG) {
						_blit_image(hdc, bitmap, mine_inac_rt, { j * tex_w, i * tex_h, tex_w, tex_h });
					} else {
						_blit_image(hdc, bitmap, mine_rt, { j * tex_w, i * tex_h, tex_w, tex_h });
					}
				} else if (!(field[i][j] & cell_state::CELL_EMPTY)) {
					num_rt.left = ((field[i][j] & 0xF) - 1) * 16;
					_blit_image(hdc, bitmap, num_rt, { j * tex_w, i * tex_h, tex_w, tex_h });
				} else {
					_blit_image(hdc, bitmap, opened_rt, { j * tex_w, i * tex_h, tex_w, tex_h });
				}
			} else if (field[i][j] & cell_state::CELL_FLAG) {
				_blit_image(hdc, bitmap, flag_rt, { j * tex_w, i * tex_h, tex_w, tex_h });
			} else {
				_blit_image(hdc, bitmap, empty_rt, { j * tex_w, i * tex_h, tex_w, tex_h });
			}
		}
	}
}

cell_state Field::open(int x, int y) {
	Field::field[y][x] |= cell_state::CELL_OPENED;
	int i = (y > 0) ? -1 : 0;
	for (; i < 2; i++)
	{
		int j = (x > 0) ? -1 : 0;
		for (; j < 2; j++)
		{
			if (i == y && j == x)
				continue;

			if (x + j < Field::width && y + i < Field::height)
			{
				if (Field::field[y + i][x + j] & cell_state::CELL_EMPTY &&
				  !(Field::field[y + i][x + j] & cell_state::CELL_OPENED) &&
				  !(Field::field[y + i][x + j] & cell_state::CELL_FLAG))
				{
					Field::open(x + j, y + i);
				} else if (!(Field::field[y + i][x + j] & cell_state::CELL_MINE) &&
					       !(Field::field[y + i][x + j] & cell_state::CELL_FLAG)) {
					field[y + i][x + j] |= cell_state::CELL_OPENED;
				}
			}
		}
	}
	return cell_state::CELL_MINE;
}

cell_state Field::get_state(int x, int y) {
	return cell_state(Field::field[y][x]);
}

int Field::put_flag(int x, int y) {
	cell_state state = Field::get_state(x, y);
	auto check_mine = [x, y](int counter) -> void {
		for (const auto& m : mines) {
			const auto [m_x, m_y] = m;
			if (m_x == x && m_y == y) {
				Field::hits += counter;
				break;
			}
		}
	};
	if (!(state & cell_state::CELL_OPENED) && !(state & cell_state::CELL_FLAG) && Field::flag_count > 0) {
		Field::field[y][x] |= cell_state::CELL_FLAG;
		Field::flag_count--;
		check_mine(1);
		
	} else if (Field::field[y][x] & cell_state::CELL_FLAG) {
		Field::field[y][x] &= ~cell_state::CELL_FLAG;
		Field::flag_count++;
		check_mine(-1);
	}
	return Field::hits;
}

void Field::_setup_field(int mines_count) {
	Field::field = new int* [Field::height];

	for (int i = 0; i < Field::height; ++i) {
		Field::field[i] = new int[Field::width];
		for (int j = 0; j < Field::width; ++j) {
			Field::field[i][j] = cell_state::CELL_EMPTY;
		}
	}

	for (int i = 0; i < mines_count; ++i) {
		int x_mine;
		int y_mine;
		do {
			x_mine = rand() % Field::width;
			y_mine = rand() % Field::height;
		} while (Field::field[y_mine][x_mine] & cell_state::CELL_MINE);
		Field::field[y_mine][x_mine] &= ~cell_state::CELL_EMPTY;
		Field::field[y_mine][x_mine] |= cell_state::CELL_MINE;
		Field::mines.push_back({ x_mine, y_mine });
	}

	for (const auto& m : Field::mines) {
		const auto [x, y] = m;
		for (int n = -1; n <= 1; ++n) {
			if (y + n < 0 || y + n >= height)
				continue;
			for (int k = -1; k <= 1; ++k) {
				if (x + k < 0 || x + k >= width)
					continue;
				if (Field::field[y + n][x + k] & cell_state::CELL_MINE)
					continue;
				if (Field::field[y + n][x + k] & cell_state::CELL_EMPTY) {
					Field::field[y + n][x + k] &= ~cell_state::CELL_EMPTY;
				}
				Field::field[y + n][x + k]++;
			}
		}
	}
}

void Field::show_mines() {
	for (const auto& m : Field::mines) {
		const auto [x, y] = m;
		Field::field[y][x] |= cell_state::CELL_OPENED;
	}
}

int Field::get_flag_count() {
	return Field::flag_count;
}

void Field::_clear_field() {
	for (int i = 0; i < Field::height; ++i) {
		delete[] Field::field[i];
	}
	delete[] Field::field;
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

int** Field::field;
std::vector<std::pair<int, int>> Field::mines;
int Field::flag_count, Field::hits;
int Field::width, Field::height;