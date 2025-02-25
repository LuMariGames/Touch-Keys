#pragma once
#include <climits>

typedef struct {
	int num = -1, notes_max, knd, roll_id, text_id;
	double judge_time = INT_MAX, bpm, scroll, x_ini;
	bool flag = false, isThrough;
	float y;

} NOTES_T;

char *get_buffer();
void draw_text(float x, float y, const char *text, float r, float g, float b);
#define BUFFER_SIZE 144
