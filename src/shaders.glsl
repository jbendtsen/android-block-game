$board_vertex 100

#version 300 es
precision mediump float;
layout (location = 0) in vec4 color_and_pos;
out vec3 rgb;

const vec2 OFFSET = vec2(@f, @f);
const vec2 CELL = vec2(@f, @f);
const vec2 GAP = vec2(@f, @f);
const int COLS = @i;

void main() {
    int p = gl_VertexID % 3;
    int v = ~((gl_VertexID - 3) >> 2) & 3;
    int data = int(color_and_pos.a);
    float edge_x = float((p & 1) ^ (v & 1)) * CELL.x;
    float edge_y = float(((p & 2) ^ (v & 2)) >> 1) * CELL.y;
    float col = float((data >> 6) & 0x1f) * (CELL.x + GAP.x);
    float row = float(((data >> 11) & 0x1f) - 3) * (CELL.y + GAP.y);
    float cell_len = float(0x40 - (data & 0x3f)) / 64.0;
    vec2 pos = vec2(OFFSET.x + (col + edge_x) * cell_len, OFFSET.y + (row + edge_y) * cell_len);
    pos = 1.0 - 2.0*pos;
    rgb = color_and_pos.rgb;
    gl_Position = vec4(pos, 0.0, 1.0);
}

$board_fragment

#version 300 es
precision mediump float;
in vec3 rgb;
out vec4 fragColor;
void main() {
    fragColor = vec4(rgb, 1.0);
}

$pause_vertex

#version 300 es
precision highp float;
layout (location = 0) uniform float y_offset;
out vec2 coords;
void main() {
    int p = gl_VertexID % 3;
    int v = ~((gl_VertexID - 3) >> 2) & 3;
    coords = vec2(float((p & 1) ^ (v & 1)), float(((p & 2) ^ (v & 2)) >> 1));
    vec2 pos = 1.0 - 2.0*vec2(coords.x, coords.y + y_offset);
    gl_Position = vec4(pos, 0.0, 1.0);
}

$pause_fragment 1000

#version 300 es
precision highp float;
precision highp int;

layout (location = 1) uniform float ratio;
layout (location = 2) uniform int frames;
layout (location = 3) uniform float fade_in_out;
in vec2 coords;
out vec4 fragColor;

const float EDGE_FADE = 0.1;
const float DIAMOND_EDGE = 1.0 / 3.0;
const float PI = 3.14159265;

float make_offset(float period) {
    int p = int(60.0 / period);
    return float(frames % p) / float(p);
}

float make_shape(vec2 point) {
    point -= floor(point);
    vec2 warped = vec2(point.x, (point.y - 0.5) * 2.0 / 3.5);
    warped += vec2(-warped.y, warped.x);
    float lum = float(warped.x >= DIAMOND_EDGE && warped.x < 1.0-DIAMOND_EDGE && warped.y >= DIAMOND_EDGE && warped.y < 1.0-DIAMOND_EDGE);
    return lum;
}

float make_move(float period, float magnitude, float disp_ratio) {
    float pos = sin(2.0 * PI * make_offset(period)) * magnitude;
    return pos + make_offset(disp_ratio);
}

vec3 rgb_from_hue(int h) {
    int s = h >> 8;
    int r = int(s == 4) * (h & 0xff) + int(s == 5 || s == 0) * 0xff + int(s == 1) * (0xff - (h & 0xff));
    int g = int(s == 0) * (h & 0xff) + int(s == 1 || s == 2) * 0xff + int(s == 3) * (0xff - (h & 0xff));
    int b = int(s == 2) * (h & 0xff) + int(s == 3 || s == 4) * 0xff + int(s == 5) * (0xff - (h & 0xff));
    return vec3(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0);
}

float make_background(vec2 screen) {
    vec2 p1 = screen * 4.5;
    p1.x *= 1.25;
    p1.x *= mix(0.9, 1.1, 0.5 + 0.5 * sin(2.0 * PI * make_offset(0.066)));
    p1.y *= mix(0.9, 1.1, 0.5 + 0.5 * cos(2.0 * PI * make_offset(0.064)));
    p1.x += make_move(0.035, 0.61, 0.13);
    p1.y += make_move(0.021, 0.63, 0.28);
    vec2 p2 = (screen + 0.125) * 4.5;
    p2.x *= 1.25;
    p2.x *= mix(0.9, 1.1, 0.5 + 0.5 * sin(2.0 * PI * make_offset(0.060)));
    p2.y *= mix(0.9, 1.1, 0.5 + 0.5 * cos(2.0 * PI * make_offset(0.062)));
    p2.x += make_move(0.045, 0.59, -0.2);
    p2.y += make_move(0.031, 0.65, 0.07);
    float lum = make_shape(p1) + make_shape(p2);
    float fade = 1.0;
    fade -= (clamp(EDGE_FADE - coords.x, 0.0, EDGE_FADE) / EDGE_FADE);
    fade -= (clamp(coords.x - (1.0-EDGE_FADE), 0.0, EDGE_FADE) / EDGE_FADE);
    fade -= (clamp(EDGE_FADE - coords.y, 0.0, EDGE_FADE) / EDGE_FADE);
    fade -= (clamp(coords.y - (1.0-EDGE_FADE), 0.0, EDGE_FADE) / EDGE_FADE);
    lum *= clamp(0.0, 1.0, fade);
    return fade_in_out * lum;
    //vec3 color = rgb_from_hue(frames % 0x600);
    //return fade_in_out * lum * (color * 0.25 + 0.75);
}

float make_0(vec2 pos) {
    pos -= 0.5;
    float dist1 = sqrt(pos.x*pos.x + 0.375*pos.y*pos.y);
    float dist2 = sqrt(pos.x*pos.x + 0.3*pos.y*pos.y);
    float lum = clamp(0.2 - dist1, 0.0, 1.0) * 20.0;
    lum *= clamp(dist2 - 0.115, 0.0, 1.0) * 20.0;
    return lum;
}

float make_1(vec2 pos) {
    pos.x -= 0.55;
    pos.y -= 0.5;
    float lum = clamp((1.0 - abs(pos.y) * 3.0) * 4.0, 0.0, 1.0);
    lum *= 1.0 - clamp(abs(pos.x) * 18.0, 0.0, 1.0);

    pos.x += 0.35;
    pos.y -= 0.65;
    float dist1 = sqrt(pos.x*pos.x + pos.y*pos.y) - 0.5;
    float a = 1.0 - clamp(abs(dist1) * 20.0, 0.0, 1.0);
    
    pos.x -= 0.29;
    pos.y += 0.47;
    float dist2 = sqrt(pos.x*pos.x + 0.8*pos.y*pos.y);
    float b = 1.0 - clamp(abs(dist2) / 0.14, 0.0, 1.0);
    b = clamp(b * 4.0, 0.0, 1.0);

    lum += a * b;
    return lum;
}

float make_2(vec2 pos) {
    pos.x -= 0.35;
    pos.y -= 0.49;
    vec2 r1 = vec2(pos.x - pos.y, pos.y + pos.x);
    vec2 r2 = vec2(pos.x - pos.y, pos.y + pos.x);
    float dist1 = sqrt(1.1*r1.x*r1.x + 0.3*r1.y*r1.y);
    float dist2 = sqrt(r2.x*r2.x + 0.33*r2.y*r2.y);
    float b = clamp(dist1 - 0.23, 0.0, 1.0) / 0.23;
    float c = clamp(0.34 - dist2, 0.0, 1.0) / 0.34;
    float lum = b*c * 16.0;
    
    lum *= clamp((pos.x+0.05) * 20.0, 0.0, 1.0);

    float a = 1.0 - clamp(abs(pos.y + 0.26) * 20.0, 0.0, 1.0);
    a *= clamp((pos.x+0.03) * 20.0, 0.0, 1.0);
    lum += a * clamp((0.36 - pos.x) * 16.0, 0.0, 1.0);

    return lum;
}

float make_3(vec2 pos) {
    pos.x -= 0.49;
    pos.y -= 0.65;
    float dist1 = sqrt(pos.x*pos.x + 1.3*pos.y*pos.y);
    pos.y += 0.28;
    float dist2 = sqrt(pos.x*pos.x + 1.2*pos.y*pos.y);
    pos.y -= 0.14;
    pos.x += 0.21;
    float dist3 = sqrt(pos.x*pos.x + 1.0*pos.y*pos.y);

    float a = clamp(0.055 - abs(0.155 - dist1), 0.0, 0.05) * 20.0;
    float b = clamp(0.055 - abs(0.165 - dist2), 0.0, 0.05) * 20.0;

    float within = a + b;
    within *= clamp((dist3 - 0.18) * 20.0, 0.0, 1.0);
    within *= clamp((dist1 - 0.08) * 20.0, 0.0, 1.0);
    within *= clamp((dist2 - 0.08) * 20.0, 0.0, 1.0);
    
    return within;
}

float make_4(vec2 pos) {
    pos.x -= 0.51;
    pos.y -= 0.5;
    float a = clamp((1.0 - abs(pos.y) * 3.05) * 4.0, 0.0, 1.0);
    a *= 1.0 - clamp(abs(pos.x-0.06) * 18.0, 0.0, 1.0);

    float b = clamp((1.0 - abs(pos.x) * 4.5) * 4.0, 0.0, 1.0);
    b *= 1.0 - clamp(abs(pos.y+0.08) * 18.0, 0.0, 1.0);

    vec2 r = vec2(pos.x - 0.67*pos.y, pos.y + 0.67*pos.x);
    float c = clamp((1.0 - abs(r.y-0.08) * 3.4) * 4.0, 0.0, 1.0);
    c *= 1.0 - clamp(abs(r.x+0.135) * 18.0, 0.0, 1.0);

    float lum = a + b + c;
    return lum;
}

float make_5(vec2 pos) {
    pos -= 0.5;
    float a = clamp((1.0 - abs(pos.x) * 6.0) * 4.0, 0.0, 1.0);
    a *= 1.0 - clamp(abs(pos.y-0.27) * 18.0, 0.0, 1.0);
    vec2 r = vec2(pos.x - 0.1*pos.y, pos.y + 0.1*pos.x);
    float b = clamp((1.0 - abs(r.y-0.14) * 7.0) * 3.0, 0.0, 1.0);
    b *= 1.0 - clamp(abs(r.x+0.14) * 18.0, 0.0, 1.0);

    pos.x += 0.05;
    pos.y += 0.1;
    float dist1 = sqrt(pos.x*pos.x + 1.4*pos.y*pos.y);
    float c = 1.0 - clamp(abs(dist1 - 0.2) * 18.0, 0.0, 1.0);

    pos.x += 0.13;
    c *= clamp(pos.x * 12.0, 0.0, 1.0);

    float lum = a + b + c;
    return lum;
}

float make_6(vec2 pos) {
    pos.x -= 0.5;
    pos.y -= 0.37;
    pos.y *= 0.96;
    float dist = sqrt(pos.x*pos.x + 1.1*pos.y*pos.y);
    float a = 1.0 - clamp(abs(dist - 0.15) * 20.0, 0.0, 1.0);
    
    vec2 r = vec2(pos.x - 0.6*pos.y, pos.y + 0.6*pos.x);
    float b = clamp((1.0 - abs(r.y-0.22) * 4.0) * 4.0, 0.0, 1.0);
    b *= 1.0 - clamp(abs(r.x+0.18) * 16.0, 0.0, 1.0);
    
    float lum = a + b;
    return lum;
}

float make_7(vec2 pos) {
    pos.x -= 0.5;
    pos.y -= 0.37;

    float a = clamp((1.0 - abs(pos.x) * 5.0) * 4.0, 0.0, 1.0);
    a *= 1.0 - clamp(abs(pos.y-0.39) * 18.0, 0.0, 1.0);

    vec2 r = vec2(pos.x - 0.5*pos.y, pos.y + 0.5*pos.x);
    float b = clamp((1.0 - abs(r.y-0.14) * 2.7) * 5.0, 0.0, 1.0);
    b *= 1.0 - clamp(abs(r.x+0.04) * 16.0, 0.0, 1.0);
    
    float lum = a + b;
    return lum;
}

float make_8(vec2 pos) {
    pos -= 0.5;
    pos.y -= 0.15;
    float dist1 = sqrt(1.2*pos.x*pos.x + 1.4*pos.y*pos.y);
    pos.y += 0.28;
    float dist2 = sqrt(1.2*pos.x*pos.x + 1.3*pos.y*pos.y);

    float a = clamp(0.055 - abs(0.155 - dist1), 0.0, 0.05) * 20.0;
    float b = clamp(0.055 - abs(0.165 - dist2), 0.0, 0.05) * 20.0;

    float within = a + b - a*b;
    within *= clamp((dist1 - 0.08) * 20.0, 0.0, 1.0);
    within *= clamp((dist2 - 0.08) * 20.0, 0.0, 1.0);
    
    return within;
}

float make_9(vec2 pos) {
    pos.x -= 0.5;
    pos.y -= 0.64;
    pos.y *= 0.96;
    float dist = sqrt(pos.x*pos.x + 1.1*pos.y*pos.y);
    float a = 1.0 - clamp(abs(dist - 0.15) * 20.0, 0.0, 1.0);
    
    vec2 r = vec2(pos.x - 0.6*pos.y, pos.y + 0.6*pos.x);
    float b = clamp((1.0 - abs(r.y+0.22) * 3.9) * 4.0, 0.0, 1.0);
    b *= 1.0 - clamp(abs(r.x-0.18) * 16.0, 0.0, 1.0);
    
    float lum = a + b;
    return lum;
}

float highlight(float a) {
    return float(a > 0.1 && a < 0.3) + 0.5 * float(a >= 0.3);
}

float make_score() {
    vec2 pos = coords - 0.5;
    pos *= vec2(-4.5, -9.0);
    pos += 0.5;
    float score_lum = 0.0;
    @m
    //vec3 score_rgb = pick faster colour;
    return score_lum;
}

void main() {
    vec2 screen = vec2(coords.x, coords.y * ratio);
    float back_lum = make_background(screen);
    float score_lum = make_score();
    back_lum *= float(score_lum <= 0.0) + 0.5 * score_lum;
    vec3 rgb = rgb_from_hue(frames % 0x600);
    vec3 back_rgb = back_lum * (rgb * 0.25 + 0.75);
    vec3 score_rgb = score_lum * (rgb * 0.5 + 0.5);
    fragColor = vec4(back_rgb + score_rgb, 1.0);
}