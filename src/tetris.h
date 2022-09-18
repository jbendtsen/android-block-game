#pragma once

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

#define MAX_INPUTS 8

#define COLS 10
#define VIS_ROWS 20
#define TOTAL_ROWS 24

#define N_PIECES           7
#define BAG_SIZE          10
#define SOFT_DROP_TICKS    3
#define CLEAR_ROWS_DELAY  30
#define GAME_OVER_DELAY   60
#define PAUSE_DELAY       VIS_ROWS
#define ROWS_PER_LEVEL    10

#define GRID_COLOR_FLOATS ((VIS_ROWS * COLS + 4) * 4)
#define GRID_COLOR_SIZE (GRID_COLOR_FLOATS * sizeof(float))

struct Tetris_Board {
    int cells[TOTAL_ROWS * COLS];
    float colors[N_PIECES * 3];

    void generate_colors();
    void copy_colors_from(Tetris_Board& other);
};

struct Tetris {
    int input_sections[MAX_INPUTS];
    int input_timers[MAX_INPUTS];

    Tetris_Board board_1;
    Tetris_Board board_2;

    Tetris_Board *board;
    Tetris_Board *next_board;

    int bag[BAG_SIZE];
    int shadows[COLS];

    int score;
    int bag_idx;
    int counter;
    int rows_soft_dropped;
    int clear_rows_timer;
    int game_over_timer;

    int pause_position;
    int pause_direction;

    int rows_cleared;
    int cur_shape;
    int next_shape;
    int cur_rotation;
    bool ever_updated;
    bool should_drop;
    bool game_over;
    bool soft_drop;
    bool resetting;

    void clear_shadows();
    int decide_next_piece(int seed);
    void drop_shape();
    void try_move(int move_dir);
    void try_flip(int flip_dir);
    void clear_rows(int full_rows_bitarray);
    void update();
    void refresh_colors(float *grid_colors);
};

struct GL_Program {
    u32 vertex_shader;
    u32 frag_shader;
    u32 program;

    void init(const char *vertex_shader_text, const char *frag_shader_text);
    void delete_objects();
};

struct GL_Context {
	void *display; // EGLDisplay
	void *context; // EGLContext
	void *surface; // EGLSurface

	int width;
	int height;

    GL_Program board;
    GL_Program overlay;
	u32 colors_array;
	u32 colors_buffer;
    u32 overlay_vertices;
    u32 overlay_vertices_buffer;

    int prev_score;

	int init(void *window, float *grid_colors);
	void quit();

	void setup_objects(float *grid_colors);
	void delete_objects();

    void draw(float *grid_colors, int counter, int pause_position, int score);
};

void set_window_egl_format(void *window, int format);
int locate_input(float x, float y);
u64 generate_random_64(int seed);
void generate_random_floats(float *out, int n);