#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include <limits.h>
#include <math.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#define VERSION "2.1.1"
#define DEFAULT_DIR "sdmc:/tkjfiles/"
#define SETTING_FILE "sdmc:/TouchKeys_config.json"

#define DEFAULT_JUDGE_RANGE_PERFECT	0.050
#define DEFAULT_JUDGE_RANGE_NICE	0.075
#define DEFAULT_JUDGE_RANGE_BAD		0.100

#define TOP_WIDTH 400
#define TOP_HEIGHT 240
#define BOTTOM_WIDTH 320
#define BOTTOM_HEIGHT 240
#define NOTES_MEASURE_MAX 512
#define MEASURE_MAX 1024
#define NOTES_MAX 8192

#define LIST_MAX 1024
#define DEFAULT_BUFFER_SIZE 4096

enum COURSE_KND {

	COURSE_EASY = 0,
	COURSE_NORMAL,
	COURSE_HARD,
	COURSE_CRAZY,
};
