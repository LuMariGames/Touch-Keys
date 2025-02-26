#pragma once
#include <climits>

typedef struct {
	int num = -1, knd, long_id;
	double judge_time = INT_MAX, bpm, scroll;
	bool flag = false;
	float y;

} NOTES_T;

typedef struct {

	int level[4],x,y,tmp;
	char title[256],path[256],tkj[256],wave[256];
	bool course[4],course_exist[4];
} LIST_T;

char *get_buffer();
void draw_text(float x, float y, const char *text, float r, float g, float b);
#define BUFFER_SIZE 144
