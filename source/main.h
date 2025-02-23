#pragma once

typedef struct {
	int num, notes_max, knd, roll_id, text_id;
	double judge_time, bpm, scroll, x_ini;
	bool flag, isThrough;
	float y;

} NOTES_T;

#define BUFFER_SIZE 144
