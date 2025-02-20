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

#define VERSION "0.1.0"
#define DEFAULT_DIR "sdmc:/tjafiles/"

#define DEFAULT_JUDGE_RANGE_PERFECT	0.033
#define DEFAULT_JUDGE_RANGE_NICE	0.067
#define DEFAULT_JUDGE_RANGE_BAD		0.10

#define TOP_WIDTH  400
#define TOP_HEIGHT 240
#define BOTTOM_WIDTH  320
#define BOTTOM_HEIGHT 240
