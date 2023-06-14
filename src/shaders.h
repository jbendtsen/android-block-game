#pragma once

#include <stdio.h>

static int _copy_string(char *dst, const char *src) {
	int len = 0;
	while ((*dst++ = *src++)) len++;
	return len;
}

static char _board_vertex_text[940];

static const char *make_board_vertex_shader(float arg0, float arg1, float arg2, float arg3, float arg4, float arg5, int arg6) {
	int offset = 0;

	offset += _copy_string(_board_vertex_text + offset,
		"#version 300 es\n"
		"precision mediump float;\n"
		"layout (location = 0) in vec4 color_and_pos;\n"
		"out vec3 rgb;\n"
		"\n"
		"const vec2 OFFSET = vec2("
	);
	offset += sprintf(_board_vertex_text + offset, "%f", arg0);

	offset += _copy_string(_board_vertex_text + offset,
		", "
	);
	offset += sprintf(_board_vertex_text + offset, "%f", arg1);

	offset += _copy_string(_board_vertex_text + offset,
		");\n"
		"const vec2 CELL = vec2("
	);
	offset += sprintf(_board_vertex_text + offset, "%f", arg2);

	offset += _copy_string(_board_vertex_text + offset,
		", "
	);
	offset += sprintf(_board_vertex_text + offset, "%f", arg3);

	offset += _copy_string(_board_vertex_text + offset,
		");\n"
		"const vec2 GAP = vec2("
	);
	offset += sprintf(_board_vertex_text + offset, "%f", arg4);

	offset += _copy_string(_board_vertex_text + offset,
		", "
	);
	offset += sprintf(_board_vertex_text + offset, "%f", arg5);

	offset += _copy_string(_board_vertex_text + offset,
		");\n"
		"const int COLS = "
	);
	offset += sprintf(_board_vertex_text + offset, "%i", arg6);

	offset += _copy_string(_board_vertex_text + offset,
		";\n"
		"\n"
		"void main() {\n"
		"    int p = gl_VertexID % 3;\n"
		"    int v = ~((gl_VertexID - 3) >> 2) & 3;\n"
		"    int data = int(color_and_pos.a);\n"
		"    float edge_x = float((p & 1) ^ (v & 1)) * CELL.x;\n"
		"    float edge_y = float(((p & 2) ^ (v & 2)) >> 1) * CELL.y;\n"
		"    float col = float((data >> 6) & 0x1f) * (CELL.x + GAP.x);\n"
		"    float row = float(((data >> 11) & 0x1f) - 3) * (CELL.y + GAP.y);\n"
		"    float cell_len = float(0x40 - (data & 0x3f)) / 64.0;\n"
		"    vec2 pos = vec2(OFFSET.x + (col + edge_x) * cell_len, OFFSET.y + (row + edge_y) * cell_len);\n"
		"    pos = 1.0 - 2.0*pos;\n"
		"    rgb = color_and_pos.rgb;\n"
		"    gl_Position = vec4(pos, 0.0, 1.0);\n"
		"}\n"
		"\n"

	);

	return &_board_vertex_text[0];
}

static char _board_fragment_text[124];

static const char *make_board_fragment_shader() {
	int offset = 0;

	offset += _copy_string(_board_fragment_text + offset,
		"#version 300 es\n"
		"precision mediump float;\n"
		"in vec3 rgb;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"    fragColor = vec4(rgb, 1.0);\n"
		"}\n"
		"\n"

	);

	return &_board_fragment_text[0];
}

static char _pause_vertex_text[371];

static const char *make_pause_vertex_shader() {
	int offset = 0;

	offset += _copy_string(_pause_vertex_text + offset,
		"#version 300 es\n"
		"precision highp float;\n"
		"layout (location = 0) uniform float y_offset;\n"
		"out vec2 coords;\n"
		"void main() {\n"
		"    int p = gl_VertexID % 3;\n"
		"    int v = ~((gl_VertexID - 3) >> 2) & 3;\n"
		"    coords = vec2(float((p & 1) ^ (v & 1)), float(((p & 2) ^ (v & 2)) >> 1));\n"
		"    vec2 pos = 1.0 - 2.0*vec2(coords.x, coords.y + y_offset);\n"
		"    gl_Position = vec4(pos, 0.0, 1.0);\n"
		"}\n"
		"\n"

	);

	return &_pause_vertex_text[0];
}

static char _pause_fragment_text[9213];

static const char *make_pause_fragment_shader(const char **arg0, int arg0_len) {
	int offset = 0;

	offset += _copy_string(_pause_fragment_text + offset,
		"#version 300 es\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"\n"
		"layout (location = 1) uniform float ratio;\n"
		"layout (location = 2) uniform int frames;\n"
		"layout (location = 3) uniform float fade_in_out;\n"
		"in vec2 coords;\n"
		"out vec4 fragColor;\n"
		"\n"
		"const float EDGE_FADE = 0.1;\n"
		"const float DIAMOND_EDGE = 1.0 / 3.0;\n"
		"const float PI = 3.14159265;\n"
		"\n"
		"float make_offset(float period) {\n"
		"    int p = int(60.0 / period);\n"
		"    return float(frames % p) / float(p);\n"
		"}\n"
		"\n"
		"float make_shape(vec2 point) {\n"
		"    point -= floor(point);\n"
		"    vec2 warped = vec2(point.x, (point.y - 0.5) * 2.0 / 3.5);\n"
		"    warped += vec2(-warped.y, warped.x);\n"
		"    float lum = float(warped.x >= DIAMOND_EDGE && warped.x < 1.0-DIAMOND_EDGE && warped.y >= DIAMOND_EDGE && warped.y < 1.0-DIAMOND_EDGE);\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_move(float period, float magnitude, float disp_ratio) {\n"
		"    float pos = sin(2.0 * PI * make_offset(period)) * magnitude;\n"
		"    return pos + make_offset(disp_ratio);\n"
		"}\n"
		"\n"
		"vec3 rgb_from_hue(int h) {\n"
		"    int s = h >> 8;\n"
		"    int r = int(s == 4) * (h & 0xff) + int(s == 5 || s == 0) * 0xff + int(s == 1) * (0xff - (h & 0xff));\n"
		"    int g = int(s == 0) * (h & 0xff) + int(s == 1 || s == 2) * 0xff + int(s == 3) * (0xff - (h & 0xff));\n"
		"    int b = int(s == 2) * (h & 0xff) + int(s == 3 || s == 4) * 0xff + int(s == 5) * (0xff - (h & 0xff));\n"
		"    return vec3(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0);\n"
		"}\n"
		"\n"
		"float make_background(vec2 screen) {\n"
		"    vec2 p1 = screen * 4.5;\n"
		"    p1.x *= 1.25;\n"
		"    p1.x *= mix(0.9, 1.1, 0.5 + 0.5 * sin(2.0 * PI * make_offset(0.066)));\n"
		"    p1.y *= mix(0.9, 1.1, 0.5 + 0.5 * cos(2.0 * PI * make_offset(0.064)));\n"
		"    p1.x += make_move(0.035, 0.61, 0.13);\n"
		"    p1.y += make_move(0.021, 0.63, 0.28);\n"
		"    vec2 p2 = (screen + 0.125) * 4.5;\n"
		"    p2.x *= 1.25;\n"
		"    p2.x *= mix(0.9, 1.1, 0.5 + 0.5 * sin(2.0 * PI * make_offset(0.060)));\n"
		"    p2.y *= mix(0.9, 1.1, 0.5 + 0.5 * cos(2.0 * PI * make_offset(0.062)));\n"
		"    p2.x += make_move(0.045, 0.59, -0.2);\n"
		"    p2.y += make_move(0.031, 0.65, 0.07);\n"
		"    float lum = make_shape(p1) + make_shape(p2);\n"
		"    float fade = 1.0;\n"
		"    fade -= (clamp(EDGE_FADE - coords.x, 0.0, EDGE_FADE) / EDGE_FADE);\n"
		"    fade -= (clamp(coords.x - (1.0-EDGE_FADE), 0.0, EDGE_FADE) / EDGE_FADE);\n"
		"    fade -= (clamp(EDGE_FADE - coords.y, 0.0, EDGE_FADE) / EDGE_FADE);\n"
		"    fade -= (clamp(coords.y - (1.0-EDGE_FADE), 0.0, EDGE_FADE) / EDGE_FADE);\n"
		"    lum *= clamp(0.0, 1.0, fade);\n"
		"    return fade_in_out * lum;\n"
		"    //vec3 color = rgb_from_hue(frames % 0x600);\n"
		"    //return fade_in_out * lum * (color * 0.25 + 0.75);\n"
		"}\n"
		"\n"
		"float make_0(vec2 pos) {\n"
		"    pos -= 0.5;\n"
		"    float dist1 = sqrt(pos.x*pos.x + 0.375*pos.y*pos.y);\n"
		"    float dist2 = sqrt(pos.x*pos.x + 0.3*pos.y*pos.y);\n"
		"    float lum = clamp(0.2 - dist1, 0.0, 1.0) * 20.0;\n"
		"    lum *= clamp(dist2 - 0.115, 0.0, 1.0) * 20.0;\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_1(vec2 pos) {\n"
		"    pos.x -= 0.55;\n"
		"    pos.y -= 0.5;\n"
		"    float lum = clamp((1.0 - abs(pos.y) * 3.0) * 4.0, 0.0, 1.0);\n"
		"    lum *= 1.0 - clamp(abs(pos.x) * 18.0, 0.0, 1.0);\n"
		"\n"
		"    pos.x += 0.35;\n"
		"    pos.y -= 0.65;\n"
		"    float dist1 = sqrt(pos.x*pos.x + pos.y*pos.y) - 0.5;\n"
		"    float a = 1.0 - clamp(abs(dist1) * 20.0, 0.0, 1.0);\n"
		"    \n"
		"    pos.x -= 0.29;\n"
		"    pos.y += 0.47;\n"
		"    float dist2 = sqrt(pos.x*pos.x + 0.8*pos.y*pos.y);\n"
		"    float b = 1.0 - clamp(abs(dist2) / 0.14, 0.0, 1.0);\n"
		"    b = clamp(b * 4.0, 0.0, 1.0);\n"
		"\n"
		"    lum += a * b;\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_2(vec2 pos) {\n"
		"    pos.x -= 0.35;\n"
		"    pos.y -= 0.49;\n"
		"    vec2 r1 = vec2(pos.x - pos.y, pos.y + pos.x);\n"
		"    vec2 r2 = vec2(pos.x - pos.y, pos.y + pos.x);\n"
		"    float dist1 = sqrt(1.1*r1.x*r1.x + 0.3*r1.y*r1.y);\n"
		"    float dist2 = sqrt(r2.x*r2.x + 0.33*r2.y*r2.y);\n"
		"    float b = clamp(dist1 - 0.23, 0.0, 1.0) / 0.23;\n"
		"    float c = clamp(0.34 - dist2, 0.0, 1.0) / 0.34;\n"
		"    float lum = b*c * 16.0;\n"
		"    \n"
		"    lum *= clamp((pos.x+0.05) * 20.0, 0.0, 1.0);\n"
		"\n"
		"    float a = 1.0 - clamp(abs(pos.y + 0.26) * 20.0, 0.0, 1.0);\n"
		"    a *= clamp((pos.x+0.03) * 20.0, 0.0, 1.0);\n"
		"    lum += a * clamp((0.36 - pos.x) * 16.0, 0.0, 1.0);\n"
		"\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_3(vec2 pos) {\n"
		"    pos.x -= 0.49;\n"
		"    pos.y -= 0.65;\n"
		"    float dist1 = sqrt(pos.x*pos.x + 1.3*pos.y*pos.y);\n"
		"    pos.y += 0.28;\n"
		"    float dist2 = sqrt(pos.x*pos.x + 1.2*pos.y*pos.y);\n"
		"    pos.y -= 0.14;\n"
		"    pos.x += 0.21;\n"
		"    float dist3 = sqrt(pos.x*pos.x + 1.0*pos.y*pos.y);\n"
		"\n"
		"    float a = clamp(0.055 - abs(0.155 - dist1), 0.0, 0.05) * 20.0;\n"
		"    float b = clamp(0.055 - abs(0.165 - dist2), 0.0, 0.05) * 20.0;\n"
		"\n"
		"    float within = a + b;\n"
		"    within *= clamp((dist3 - 0.18) * 20.0, 0.0, 1.0);\n"
		"    within *= clamp((dist1 - 0.08) * 20.0, 0.0, 1.0);\n"
		"    within *= clamp((dist2 - 0.08) * 20.0, 0.0, 1.0);\n"
		"    \n"
		"    return within;\n"
		"}\n"
		"\n"
		"float make_4(vec2 pos) {\n"
		"    pos.x -= 0.51;\n"
		"    pos.y -= 0.5;\n"
		"    float a = clamp((1.0 - abs(pos.y) * 3.05) * 4.0, 0.0, 1.0);\n"
		"    a *= 1.0 - clamp(abs(pos.x-0.06) * 18.0, 0.0, 1.0);\n"
		"\n"
		"    float b = clamp((1.0 - abs(pos.x) * 4.5) * 4.0, 0.0, 1.0);\n"
		"    b *= 1.0 - clamp(abs(pos.y+0.08) * 18.0, 0.0, 1.0);\n"
		"\n"
		"    vec2 r = vec2(pos.x - 0.67*pos.y, pos.y + 0.67*pos.x);\n"
		"    float c = clamp((1.0 - abs(r.y-0.08) * 3.4) * 4.0, 0.0, 1.0);\n"
		"    c *= 1.0 - clamp(abs(r.x+0.135) * 18.0, 0.0, 1.0);\n"
		"\n"
		"    float lum = a + b + c;\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_5(vec2 pos) {\n"
		"    pos -= 0.5;\n"
		"    float a = clamp((1.0 - abs(pos.x) * 6.0) * 4.0, 0.0, 1.0);\n"
		"    a *= 1.0 - clamp(abs(pos.y-0.27) * 18.0, 0.0, 1.0);\n"
		"    vec2 r = vec2(pos.x - 0.1*pos.y, pos.y + 0.1*pos.x);\n"
		"    float b = clamp((1.0 - abs(r.y-0.14) * 7.0) * 3.0, 0.0, 1.0);\n"
		"    b *= 1.0 - clamp(abs(r.x+0.14) * 18.0, 0.0, 1.0);\n"
		"\n"
		"    pos.x += 0.05;\n"
		"    pos.y += 0.1;\n"
		"    float dist1 = sqrt(pos.x*pos.x + 1.4*pos.y*pos.y);\n"
		"    float c = 1.0 - clamp(abs(dist1 - 0.2) * 18.0, 0.0, 1.0);\n"
		"\n"
		"    pos.x += 0.13;\n"
		"    c *= clamp(pos.x * 12.0, 0.0, 1.0);\n"
		"\n"
		"    float lum = a + b + c;\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_6(vec2 pos) {\n"
		"    pos.x -= 0.5;\n"
		"    pos.y -= 0.37;\n"
		"    pos.y *= 0.96;\n"
		"    float dist = sqrt(pos.x*pos.x + 1.1*pos.y*pos.y);\n"
		"    float a = 1.0 - clamp(abs(dist - 0.15) * 20.0, 0.0, 1.0);\n"
		"    \n"
		"    vec2 r = vec2(pos.x - 0.6*pos.y, pos.y + 0.6*pos.x);\n"
		"    float b = clamp((1.0 - abs(r.y-0.22) * 4.0) * 4.0, 0.0, 1.0);\n"
		"    b *= 1.0 - clamp(abs(r.x+0.18) * 16.0, 0.0, 1.0);\n"
		"    \n"
		"    float lum = a + b;\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_7(vec2 pos) {\n"
		"    pos.x -= 0.5;\n"
		"    pos.y -= 0.37;\n"
		"\n"
		"    float a = clamp((1.0 - abs(pos.x) * 5.0) * 4.0, 0.0, 1.0);\n"
		"    a *= 1.0 - clamp(abs(pos.y-0.39) * 18.0, 0.0, 1.0);\n"
		"\n"
		"    vec2 r = vec2(pos.x - 0.5*pos.y, pos.y + 0.5*pos.x);\n"
		"    float b = clamp((1.0 - abs(r.y-0.14) * 2.7) * 5.0, 0.0, 1.0);\n"
		"    b *= 1.0 - clamp(abs(r.x+0.04) * 16.0, 0.0, 1.0);\n"
		"    \n"
		"    float lum = a + b;\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float make_8(vec2 pos) {\n"
		"    pos -= 0.5;\n"
		"    pos.y -= 0.15;\n"
		"    float dist1 = sqrt(1.2*pos.x*pos.x + 1.4*pos.y*pos.y);\n"
		"    pos.y += 0.28;\n"
		"    float dist2 = sqrt(1.2*pos.x*pos.x + 1.3*pos.y*pos.y);\n"
		"\n"
		"    float a = clamp(0.055 - abs(0.155 - dist1), 0.0, 0.05) * 20.0;\n"
		"    float b = clamp(0.055 - abs(0.165 - dist2), 0.0, 0.05) * 20.0;\n"
		"\n"
		"    float within = a + b - a*b;\n"
		"    within *= clamp((dist1 - 0.08) * 20.0, 0.0, 1.0);\n"
		"    within *= clamp((dist2 - 0.08) * 20.0, 0.0, 1.0);\n"
		"    \n"
		"    return within;\n"
		"}\n"
		"\n"
		"float make_9(vec2 pos) {\n"
		"    pos.x -= 0.5;\n"
		"    pos.y -= 0.64;\n"
		"    pos.y *= 0.96;\n"
		"    float dist = sqrt(pos.x*pos.x + 1.1*pos.y*pos.y);\n"
		"    float a = 1.0 - clamp(abs(dist - 0.15) * 20.0, 0.0, 1.0);\n"
		"    \n"
		"    vec2 r = vec2(pos.x - 0.6*pos.y, pos.y + 0.6*pos.x);\n"
		"    float b = clamp((1.0 - abs(r.y+0.22) * 3.9) * 4.0, 0.0, 1.0);\n"
		"    b *= 1.0 - clamp(abs(r.x-0.18) * 16.0, 0.0, 1.0);\n"
		"    \n"
		"    float lum = a + b;\n"
		"    return lum;\n"
		"}\n"
		"\n"
		"float highlight(float a) {\n"
		"    return float(a > 0.1 && a < 0.3) + 0.5 * float(a >= 0.3);\n"
		"}\n"
		"\n"
		"float make_score() {\n"
		"    vec2 pos = coords - 0.5;\n"
		"    pos *= vec2(-4.5, -9.0);\n"
		"    pos += 0.5;\n"
		"    float score_lum = 0.0;\n"
		"    "
	);

	for (int i = 0; i < arg0_len; i++)
		offset += _copy_string(_pause_fragment_text + offset, arg0[i]);

	offset += _copy_string(_pause_fragment_text + offset,
		"\n"
		"    //vec3 score_rgb = pick faster colour;\n"
		"    return score_lum;\n"
		"}\n"
		"\n"
		"void main() {\n"
		"    vec2 screen = vec2(coords.x, coords.y * ratio);\n"
		"    float back_lum = make_background(screen);\n"
		"    float score_lum = make_score();\n"
		"    back_lum *= float(score_lum <= 0.0) + 0.5 * score_lum;\n"
		"    vec3 rgb = rgb_from_hue(frames % 0x600);\n"
		"    vec3 back_rgb = back_lum * (rgb * 0.25 + 0.75);\n"
		"    vec3 score_rgb = score_lum * (rgb * 0.5 + 0.5);\n"
		"    fragColor = vec4(back_rgb + score_rgb, 1.0);\n"
		"}\n"

	);

	return &_pause_fragment_text[0];
}

