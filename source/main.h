#pragma once

typedef struct {
	int num, notes_max, knd, roll_id, text_id;
	double judge_time = INT_MAX, bpm, scroll, x_ini;
	bool flag, isThrough;
	float y;

} NOTES_T;

char *get_buffer();
void draw_text(float x, float y, const char *text, float r, float g, float b);
#define BUFFER_SIZE 144
