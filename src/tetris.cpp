#include "tetris.h"

#define CAN_MOVE_LATERALLY(t) (t == 1 || (t >= 15 && t % 5 == 0))

static const u8 shapes[][2] = {
    {0x41, 0xf},  // I piece
    {0x22, 0xf},  // O piece
    {0x32, 0x27}, // J piece
    {0x32, 0xf},  // L piece
    {0x32, 0x1e}, // S piece
    {0x32, 0x33}, // Z piece
    {0x32, 0x17}  // T piece
};

static const int rotation_offsets[][4] = {
    {0, 0, 0, 0}, // I piece
    {0, 0, 0, 0}, // O piece
    {0, 1, 0, 0}, // J piece
    {0, 0, 2, 0}, // L piece
    {1, 0, 1, 0}, // S piece
    {0, 1, 0, 1}, // Z piece
    {0, 1, 1, 0}, // T piece
};

static const int rotation_moves[][4][8] = {
    { // I piece
        {2,-2, 1,-1, 0,0, -1,1},
        {1,2, 0,1, -1,0, -2,-1},
        {-1,1, 0,0, 1,-1, 2,-2},
        {-2,-1, -1,0, 0,1, 1,2}
    },
    { // O piece
        {0,0, 0,0, 0,0, 0,0},
        {0,0, 0,0, 0,0, 0,0},
        {0,0, 0,0, 0,0, 0,0},
        {0,0, 0,0, 0,0, 0,0},
    },
    { // J piece
        {1,-1, 0,0, -1,1, -2,0},
        {1,1, 0,0, -1,-1, 0,-2},
        {-1,1, 0,0, 1,-1, 2,0},
        {-1,-1, 0,0, 1,1, 0,2}
    },
    { // L piece
        {1,-1, 0,0, -1,1, 0,-2},
        {1,1, 0,0, -1,-1, 2,0},
        {-1,1, 0,0, 1,-1, 0,2},
        {-1,-1, 0,0, 1,1, -2,0}
    },
    { // S piece
        {1,0, 0,1, 1,-2, 0,-1},
        {-1,1, -2,0, 1,1, 0,0},
        {0,-1, 1,-2, 0,1, 1,0},
        {0,0, 1,1, -2,0, -1,1}
    },
    { // Z piece
        {2,-1, 1,0, 0,-1, -1,0},
        {0,2, -1,1, 0,0, -1,-1},
        {-1,0, 0,-1, 1,0, 2,-1},
        {-1,-1, 0,0, -1,1, 0,2},
    },
    { // T piece
        {1,-1, 0,0, -1,1, -1,-1},
        {1,1, 0,0, -1,-1, 1,-1},
        {-1,1, 0,0, 1,-1, 1,1},
        {-1,-1, 0,0, 1,1, -1,1}
    }
};

static int get_gravity(int level)
{
    int gravity = 1;

    if (level <= 10) {
        float x = (float)level / 2.1 - 6.9; // very scientific
        gravity = (int)(x*x + 0.5);
    }
    else if (level <= 12)
        gravity = 5;
    else if (level <= 15)
        gravity = 4;
    else if (level <= 18)
        gravity = 3;
    else if (level <= 28)
        gravity = 2;

    return gravity;
}

static bool do_rotate(int *cells, int *pos_xy, int *move_xy)
{
    if (!pos_xy || !move_xy)
        return false;

    bool can_move = true;
    for (int i = 0; i < 8; i += 2) {
        int x = pos_xy[i] + move_xy[i];
        int y = pos_xy[i+1] + move_xy[i+1];

        if (x < 0 || x >= COLS ||
            y < 0 || y >= TOTAL_ROWS ||
            cells[y*COLS+x] > 0
        ) {
            can_move = false;
            break;
        }
    }

    if (!can_move)
        return false;

    int c = cells[pos_xy[1] * COLS + pos_xy[0]];

    for (int i = 0; i < 8; i += 2)
        cells[pos_xy[i+1] * COLS + pos_xy[i]] = 0;

    for (int i = 0; i < 8; i += 2) {
        int idx = (pos_xy[i+1] + move_xy[i+1]) * COLS;
        idx += pos_xy[i] + move_xy[i];
        cells[idx] = c;
    }

    return true;
}

void Tetris::clear_shadows()
{
    for (int i = 0; i < COLS; i++)
        shadows[i] = 0;
}

int Tetris::decide_next_piece(int seed)
{
    u64 random = generate_random_64(seed);
    int random_piece = random % N_PIECES;
    random /= N_PIECES;

    if (bag_idx == 0) {
        for (int i = 0; i < BAG_SIZE; i++)
            bag[i] = 0;

        for (int i = 0; i < BAG_SIZE; i++) {
            int d = BAG_SIZE - i;
            int p = random % d;
            random /= d;

            for (int j = 0; j < BAG_SIZE; j++) {
                if (bag[j] > 0)
                    continue;
                if (p == 0) {
                    bag[j] = i + 1;
                    break;
                }
                p--;
            }
        }
    }
    
    int piece = bag[bag_idx] - 1;
    if (piece >= N_PIECES)
        piece = random_piece;

    bag_idx = (bag_idx + 1) % BAG_SIZE;
    return piece + 1;
}

void Tetris::drop_shape()
{
    cur_shape = next_shape;
    if (!next_shape)
        cur_shape = decide_next_piece(0);

    int s_idx = cur_shape - 1;
    next_shape = decide_next_piece(1);

    int w = shapes[s_idx][0] >> 4;
    int h = shapes[s_idx][0] & 0xf;
    u8 blocks = shapes[s_idx][1];

    int x_off = (COLS - w) / 2;
    int y_off = TOTAL_ROWS - VIS_ROWS;
    int x = 0;
    int y = 0;

    while (blocks) {
        if (blocks & 1) {
            int c_idx = (y_off+y)*COLS + x_off+x;
            if (board->cells[c_idx] > 0) {
                game_over = true;
                clear_shadows();
            }
            board->cells[c_idx] = -s_idx - 1;
        }

        x = (x + 1) % w;
        if (x == 0)
            y = (y + 1) % h;
        
        blocks >>= 1;
    }
}

void Tetris::try_move(int move_dir)
{
    if (!move_dir)
        return;

    int i = TOTAL_ROWS * COLS;
    while (i > 0) {
        i--;
        if (board->cells[i] < 0) {
            int next = i+move_dir;
            if (next / COLS != i / COLS || board->cells[next] > 0)
                return;
        }
    }

    int start = TOTAL_ROWS * COLS;
    int end = 0;
    if (move_dir < 0) {
        int temp = start;
        start = end;
        end = temp;
    }

    for (int i = start; i != end; i -= move_dir) {
        if (board->cells[i] < 0) {
            board->cells[i+move_dir] = board->cells[i];
            board->cells[i] = 0;
        }
    }
}

void Tetris::try_flip(int flip_dir)
{
    int s = cur_shape - 1;
    if (flip_dir == 0 || s == 1)
        return;

    int idx;
    for (idx = 0; idx < TOTAL_ROWS * COLS && board->cells[idx] >= 0; idx++);

    int r = cur_rotation;
    int r2 = (r + (4 + flip_dir) % 4) % 4;

    int ox = idx % COLS;
    ox -= rotation_offsets[s][r];
    int oy = idx / COLS;

    int w = shapes[s][0] >> 4;
    int h = shapes[s][0] & 0xf;
    u8 blocks = shapes[s][1];
    
    int pos_xy[8];
    int p = 0;
    for (int i = 0; i < w*h && p < 8; i++) {
        if ((blocks & (1 << i)) == 0)
            continue;

        int x = i % w;
        int y = i / w;
        x = (r == 2 || r == 3) ? w-x-1 : x;
        y = (r == 1 || r == 2) ? h-y-1 : y;
        if (r == 1 || r == 3) {
            int temp = x;
            x = y;
            y = temp;
        }

        pos_xy[p++] = ox + x;
        pos_xy[p++] = oy + y;
    }

    if (flip_dir < 0)
        r = r2;

    int move_xy[8];
    for (int i = 0; i < 8; i++)
        move_xy[i] = rotation_moves[s][r][i] * flip_dir;

    if (do_rotate(board->cells, pos_xy, move_xy))
        cur_rotation = r2;
}

void Tetris::clear_rows(int full_rows_bitarray)
{
    int fields = full_rows_bitarray;
    int cleared = 0;
    bool repeat_row = false;

    int i = TOTAL_ROWS * COLS;
    while (i > 0) {
        i--;

        if (i % COLS == COLS-1) {
            if (repeat_row) {
                i += COLS;
                repeat_row = false;
            }

            int row = (i / COLS) - cleared;
            if (fields & (1 << row)) {
                fields &= ~(1 << row);
                cleared++;
                repeat_row = true;
            }
        }

        int gap = cleared * COLS;
        next_board->cells[i] = i < gap ? 0 : board->cells[i-gap];
    }
}

void Tetris::update()
{
    if (!board) {
        board = &board_1;
        board->generate_colors();
    }
    if (!next_board) {
        next_board = &board_2;
        next_board->copy_colors_from(*board);
    }
    
    int flip_dir = 0;
    int move_dir = 0;
    int down_timer = 0;
    bool pause_triggered = false;

    for (int i = 0; i < MAX_INPUTS; i++) {
        int s = input_sections[i];
        if (s > 0) input_timers[i]++;
        int t = input_timers[i];

        if (s == 6 && t == 1)
            pause_triggered = true;
        else if (s == 5)
            down_timer = t;
        else if (s == 1 && t == 1)
            flip_dir -= 1;
        else if (s == 2 && t == 1)
            flip_dir += 1;
        else if (s == 3 && CAN_MOVE_LATERALLY(t))
            move_dir -= 1;
        else if (s == 4 && CAN_MOVE_LATERALLY(t))
            move_dir += 1;
    }

    if (down_timer == 1 && (
        (pause_position > 0 && pause_position <= PAUSE_DELAY && pause_direction >= 0) ||
        (pause_position == 0 && pause_triggered))
    ) {
        int pos = pause_position;
        int s = score;
        //soft_drop = false; // not necessary
        *this = {0};
        pause_position = pos;
        score = s;

        board = &board_1;
        next_board = &board_2;
        board->generate_colors();
        next_board->copy_colors_from(*board);

        resetting = true;
    }

    if (resetting) {
        if (pause_position >= 2*PAUSE_DELAY) {
            pause_position = 0;
            pause_direction = 0;
            score = 0;
            resetting = false;
        }
        else {
            pause_position++;
        }
    }
    else {
        if (pause_triggered && !pause_direction)
            pause_direction = pause_position <= 0 ? 1 : -1;

        pause_position += pause_direction;

        if (pause_position <= 0 || pause_position >= PAUSE_DELAY)
            pause_direction = 0;

        if (pause_position > 0)
            return;
    }

    if (game_over) {
        game_over_timer++;
        return;
    }

    if (clear_rows_timer > 0) {
        clear_rows_timer++;
        if (clear_rows_timer > CLEAR_ROWS_DELAY) {
            Tetris_Board *temp = board;
            board = next_board;
            next_board = temp;
            next_board->copy_colors_from(*board);
            clear_rows_timer = 0;
        }

        return;
    }

    if (should_drop || !ever_updated) {
        drop_shape();
        cur_rotation = 0;
        should_drop = false;
        if (game_over)
            return;
    }

    if (down_timer == 0) {
        soft_drop = false;
        rows_soft_dropped = 0;
    }
    else if (down_timer == 1) {
        soft_drop = true;
    }
    
    try_move(move_dir);
    try_flip(flip_dir);

    clear_shadows();
    for (int i = 0; i < TOTAL_ROWS * COLS; i++) {
        int& s = shadows[i % COLS];
        if (s == 0 and board->cells[i] < 0)
            s = -1;
        else if (s < 0 && (i >= (TOTAL_ROWS-1)*COLS || board->cells[i+COLS] > 0))
            s = i / COLS;
    }

    int ticks = soft_drop ? SOFT_DROP_TICKS : get_gravity(rows_cleared / ROWS_PER_LEVEL);    
    bool should_tick = !ever_updated;

    counter++;
    while (counter >= ticks) {
        counter -= ticks;
        should_tick = true;
    }
    if (!should_tick)
        return;

    ever_updated = true;

    if (soft_drop)
        rows_soft_dropped += 1;

    bool stuck = false;
    for (int i = 0; i < TOTAL_ROWS * COLS; i++) {
        if (board->cells[i] < 0 && (i >= (TOTAL_ROWS-1) * COLS || board->cells[i+COLS] > 0)) {
            stuck = true;
            break;
        }
    }

    should_drop = stuck;
    if (should_drop) {
        score += rows_soft_dropped;
        rows_soft_dropped = 0;
        soft_drop = false;
    }

    for (int i = TOTAL_ROWS * COLS - 1; i >= 0; i--) {
        if (board->cells[i] >= 0)
            continue;

        if (stuck) {
            board->cells[i] = -board->cells[i];
        }
        else if (i < (TOTAL_ROWS-1) * COLS) {
            board->cells[i+COLS] = board->cells[i];
            board->cells[i] = 0;
        }
    }

    if (!stuck)
        return;

    int old_rows_cleared = rows_cleared;
    int full_rows_bitarray = 0;
    int occupied = 0;

    for (int i = 0; i < TOTAL_ROWS * COLS; i++) {
        if (board->cells[i] > 0)
            occupied++;

        if (i % COLS == COLS-1) {
            if (occupied == COLS) {
                full_rows_bitarray |= 1 << (i / COLS);
                rows_cleared++;
            }
            occupied = 0;
        }
    }

    if (full_rows_bitarray) {
        clear_rows(full_rows_bitarray);
        clear_shadows();

        clear_rows_timer = 1;
        int cur_level = old_rows_cleared / ROWS_PER_LEVEL;
        int next_level = rows_cleared / ROWS_PER_LEVEL;

        if (next_level > cur_level)
            next_board->generate_colors();

        const int bonus[] = {0, 40, 100, 300, 1200};
        int n = rows_cleared - old_rows_cleared;
        score += bonus[n] * (next_level+1);
    }
}

void Tetris::refresh_colors(float *grid_colors)
{
    int skip = (TOTAL_ROWS - VIS_ROWS) * COLS;
    float x = (float)clear_rows_timer / (float)CLEAR_ROWS_DELAY;
    float y = (float)game_over_timer / (float)GAME_OVER_DELAY;
    if (y >= 1.0) y = 1.0;

    const float blank_lum = 0.125;
    const float shadow_lum = 0.1875;

    for (int i = 0; i < VIS_ROWS * COLS; i++) {
        int c1 = board->cells[skip+i] * 3;
        int c2 = next_board->cells[skip+i] * 3;
        c1 = c1 < 0 ? -c1 : c1;
        c2 = c2 < 0 ? -c2 : c2;

        float r = (shadows[i % COLS] == (skip+i) / COLS) ? shadow_lum : blank_lum;
        float g = r;
        float b = r;

        float r2 = blank_lum;
        float g2 = r2;
        float b2 = r2;

        if (c1 > 0) {
            c1 -= 3;
            r = board->colors[c1];
            g = board->colors[c1 + 1];
            b = board->colors[c1 + 2];
        }
        if (c2 > 0) {
            c2 -= 3;
            r2 = next_board->colors[c2];
            g2 = next_board->colors[c2 + 1];
            b2 = next_board->colors[c2 + 2];
        }

        r = (1-x) * r + x * r2;
        g = (1-x) * g + x * g2;
        b = (1-x) * b + x * b2;

        grid_colors[i*4]   = (1-y) * r + y * blank_lum;
        grid_colors[i*4+1] = (1-y) * g + y * blank_lum;
        grid_colors[i*4+2] = (1-y) * b + y * blank_lum;

        int col = COLS-1 - (i % COLS);
        int row = (i / COLS) + 3;
        grid_colors[i*4+3] = (row << 11) | (col << 6);
    }

    int next_positions[4];
    int n = 0;
    for (int i = 0; i < 8; i++) {
        if ((shapes[next_shape-1][1] >> i) & 1)
            next_positions[n++] = i;
    }

    float *next_piece_colors = &grid_colors[VIS_ROWS * COLS * 4];

    for (int i = 0; i < 4; i++) {
        float *cur  = &board->colors[(next_shape-1)*3];
        float *next = &next_board->colors[(next_shape-1)*3];
        
        float r = (1-x) * cur[0] + x * next[0];
        float g = (1-x) * cur[1] + x * next[1];
        float b = (1-x) * cur[2] + x * next[2];

        int w = shapes[next_shape-1][0] >> 4;
        int row = next_positions[i] / w;
        int col = w-1 - (next_positions[i] % w);

        next_piece_colors[i*4]   = (1-y) * r + y * blank_lum;
        next_piece_colors[i*4+1] = (1-y) * g + y * blank_lum;
        next_piece_colors[i*4+2] = (1-y) * b + y * blank_lum;
        next_piece_colors[i*4+3] = (row << 11) | (col << 6) | 0x10;
    }
}

static void rgb_from_hue(float *rgb, int h)
{
    h %= 0x600;
    int r = 0;
    int g = 0;
    int b = 0;
    if (h <= 0xff) {
        r = 0xff;
        g = h;
    }
    else if (h <= 0x1ff) {
        r = 0xff - (h & 0xff);
        g = 0xff;
    }
    else if (h <= 0x2ff) {
        g = 0xff;
        b = h & 0xff;
    }
    else if (h <= 0x3ff) {
        g = 0xff - (h & 0xff);
        b = 0xff;
    }
    else if (h <= 0x4ff) {
        b = 0xff;
        r = h & 0xff;
    }
    else {
        b = 0xff - (h & 0xff);
        r = 0xff;
    }
    rgb[0] = (float)r / 255.0;
    rgb[1] = (float)g / 255.0;
    rgb[2] = (float)b / 255.0;
}

static void apply_luma(float *rgb, float y)
{
    y = 0.3 + (1-y) * 0.7;
    rgb[0] = 1.0 - ((1.0 - rgb[0]) * y);
    rgb[1] = 1.0 - ((1.0 - rgb[1]) * y);
    rgb[2] = 1.0 - ((1.0 - rgb[2]) * y);
}

static void apply_desaturation(float *rgb, float weight)
{
    float lum = (rgb[0] + rgb[1] + rgb[2]) / 3.0;
    float a = weight;
    float b = 1-a;
    rgb[0] = lum * a + rgb[0] * b;
    rgb[1] = lum * a + rgb[1] * b;
    rgb[2] = lum * a + rgb[2] * b;
}

void Tetris_Board::generate_colors()
{
    u64 random = generate_random_64(0);
    int color_offset = random % 0x600;
    random /= 0x600;
    int spread = 112 + (random % 64);
    random /= 64;

    float luma_limit = (float)(112*2 + 64 - spread) / 176.0;

    float weights[N_PIECES*2];
    generate_random_floats(weights, N_PIECES*2);

    for (int i = 0; i < N_PIECES; i++) {
        float *rgb = &colors[i*3];
        rgb_from_hue(rgb, i * spread + color_offset);
        apply_luma(rgb, weights[i] * luma_limit);
        apply_desaturation(rgb, weights[i+N_PIECES] * 0.8);
    }
}

void Tetris_Board::copy_colors_from(Tetris_Board& other)
{
    for (int i = 0; i < N_PIECES*3; i++)
        colors[i] = other.colors[i];
}