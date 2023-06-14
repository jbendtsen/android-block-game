#include "tetris.h"
#include "shaders.h"

#include <android/log.h>

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

int GL_Context::init(void *window, float *grid_colors)
{
	const EGLint attribs[] = {
	    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		//EGL_NATIVE_RENDERABLE, EGL_TRUE,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	this->display = display;
	auto res_init = eglInitialize(display, 0, 0);

    EGLint n_configs;
	EGLConfig cfg;
	eglChooseConfig(display, attribs, &cfg, 1, &n_configs);

    __android_log_print(ANDROID_LOG_INFO, "gl_ctx_init", "could not find egl config");

	EGLint format;
	eglGetConfigAttrib(display, cfg, EGL_NATIVE_VISUAL_ID, &format);
	set_window_egl_format(window, format);

	EGLSurface surface = eglCreateWindowSurface(display, cfg, (EGLNativeWindowType)window, NULL);
	this->surface = surface;

	const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3, // new requirement
		EGL_NONE
	};
	EGLContext context = eglCreateContext(display, cfg, EGL_NO_CONTEXT, context_attribs);
	this->context = context;
	if (!eglMakeCurrent(display, surface, surface, context)) {
	    __android_log_print(ANDROID_LOG_INFO, "gl_ctx_init", "eglMakeCurrent() failed");
		return -2;
	}

	EGLint w, h;
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	width = w;
	height = h;

	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	//glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	setup_objects(grid_colors);

	return 0;
}

void GL_Context::quit()
{
	delete_objects();

	if (display) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (context)
			eglDestroyContext(display, context);
		if (surface)
			eglDestroySurface(display, surface);

		eglTerminate(display);
	}

	display = NULL;
	context = NULL;
	surface = NULL;
}

static int compile_count = 0;

static void log_compile_message(u32 shader)
{
	compile_count++;
	GLint log_size = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
	if (log_size > 0) {
		char *compile_error = (char*)malloc(log_size + 1);
		glGetShaderInfoLog(shader, log_size, NULL, compile_error);
		__android_log_print(ANDROID_LOG_INFO, "glCompileShader", "%d", compile_count);
		__android_log_print(ANDROID_LOG_INFO, "glCompileShader", "%s", compile_error);
		free(compile_error);
	}
	else {
		GLint success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		__android_log_print(ANDROID_LOG_INFO, "glCompileShader", "result=%d", success);
	}
}

struct Board_Values {
	float gap;
	float cell;
	float w;
	float h;
	float off_x;
	float off_y;
};

static Board_Values make_board_values(float width, float height)
{
	float gap = 0.125;
	float cell_w = 1.0 / 14.0;
	float cell_h = cell_w * width / height;
	Board_Values bv = {
		.gap = gap,
		.cell = 1.0f - gap,
		.w = cell_w,
		.h = cell_h,
		.off_x = (10.0f*cell_w) + gap * cell_w,
		.off_y = (20.0f*cell_h)
	};
	return bv;
}

void GL_Context::setup_objects(float *grid_colors)
{
	Board_Values bv = make_board_values(width, height);
	float off_x = (1.0 - bv.off_x) / 2;
	float off_y = (1.0 - bv.off_y) / 2;

	const char *vertex_shader_text = make_board_vertex_shader(
		off_x, off_y, bv.cell * bv.w, bv.cell * bv.h, bv.gap * bv.w, bv.gap * bv.h, COLS
	);

	const char *frag_shader_text = make_board_fragment_shader();

	board.init(vertex_shader_text, frag_shader_text);

	glGenVertexArrays(1, &colors_array);
	glGenBuffers(1, &colors_buffer);
	glBindVertexArray(colors_array);
	glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
	glBufferData(GL_ARRAY_BUFFER, GRID_COLOR_SIZE, grid_colors, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(0, 1);

	const char *zero_line = "score_lum += highlight(make_0(pos));\n";
	const char *overlay_frag_text = make_pause_fragment_shader(&zero_line, 1);
	const char *overlay_vertex_text = make_pause_vertex_shader();

	overlay.init(overlay_vertex_text, overlay_frag_text);

	glGenVertexArrays(1, &overlay_vertices);
	glGenBuffers(1, &overlay_vertices_buffer);
}

void GL_Context::delete_objects()
{
    glDeleteVertexArrays(1, &colors_array);
    glDeleteBuffers(1, &colors_buffer);
	board.delete_objects();
	overlay.delete_objects();
}

static char digit_buffer[1000];
static char *digit_lines[10];

static void draw_overlay(GL_Context& ctx, int counter, int pause_position, int score)
{
	float inc_h = 1.0 / PAUSE_DELAY;

	float angle = M_PI * (float)pause_position / (2.0 * PAUSE_DELAY);
	float distance = sin(angle) * inc_h * PAUSE_DELAY;
	float ratio = (ctx.height / ctx.width);

	float y_offset;
	if (pause_position <= PAUSE_DELAY) {
		y_offset = distance - 1.0;
	}
	else {
		y_offset = 1.0 - distance;
	}

	if (score != ctx.prev_score) {
		ctx.overlay.delete_objects();

		int n = score;
		int n_digits = score == 0;
		while (n) {
			n /= 10;
			n_digits++;
		}

		n = score;
		float x_offset = (float)(n_digits-1) * -0.22;
		float x_inc = 0.44;
		char *str = digit_buffer;
		for (int i = 0; i < n_digits; i++) {
			int d = n % 10;
			n /= 10;
			float x = x_offset < 0.0 ? -x_offset : x_offset;
			char c = x_offset < 0.0 ? '-' : '+';
			int len = sprintf(str, "score_lum += highlight(make_%d(vec2(pos.x %c %f, pos.y)));\n", d, c, x);
			digit_lines[i] = str;
			str += len + 1;
			x_offset += x_inc;
		}

		const char *overlay_frag_text = make_pause_fragment_shader((const char **)digit_lines, n_digits);
		const char *overlay_vertex_text = make_pause_vertex_shader();
		ctx.overlay.init(overlay_vertex_text, overlay_frag_text);
	}

	ctx.prev_score = score;

	glUseProgram(ctx.overlay.program);
	glUniform1f(0, y_offset);
	glUniform1f(1, ratio);
	glUniform1i(2, counter);
	glUniform1f(3, distance);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void GL_Context::draw(float *grid_colors, int counter, int pause_position, int score)
{
	glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(board.program);
    glBindVertexArray(colors_array);
    glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, GRID_COLOR_SIZE, grid_colors);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GRID_COLOR_FLOATS / 4);
    glBindVertexArray(0);

	int pos = pause_position;
	if (pos > 0 && pos < 2*PAUSE_DELAY)
		draw_overlay(*this, counter, pause_position, score);

    eglSwapBuffers(display, surface);
}

void GL_Program::init(const char *vertex_text, const char *frag_text)
{
	int vertex_shader_len = strlen(vertex_text);
	int frag_shader_len = strlen(frag_text);

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_text, &vertex_shader_len);
	glCompileShader(vertex_shader);
	log_compile_message(vertex_shader);

	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &frag_text, &frag_shader_len);
	glCompileShader(frag_shader);
	log_compile_message(frag_shader);

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	glDetachShader(program, vertex_shader);
	glDetachShader(program, frag_shader);
}

void GL_Program::delete_objects()
{
	glDeleteProgram(program);
	glDeleteShader(vertex_shader);
	glDeleteShader(frag_shader);
}
