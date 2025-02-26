#include "audio.h"
#include "header.h"
#include "main.h"
#include "playback.h"

#include <citro2d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SDカードからテクスチャを読み込む
char buffer[BUFFER_SIZE];
char tkj_notes[MEASURE_MAX][NOTES_MEASURE_MAX];	//ノーツ情報
int scene = 0,timecnt = 0,judgetmpcnt = 0,NotesSpeed = 200,touchid = -1,judgeid = -1,tkj_cnt = 0,
NotesCount = 0,MaxNotesCnt = 0,Startcnt = 0,MeasureCount = 0,Score = 0,Combo = 0,course = COURSE_HARD,CurrentCourse = -1,
touch_x, touch_y, PreTouch_x, PreTouch_y;
double BPM = 120.0,OFFSET = 0,OffTime = 0,NowTime = 0;
bool isExit = false,isPlayMain = false,isAuto = false,isCourseMatch = false;

//static C2D_SpriteSheet spriteSheet;
C2D_TextBuf g_dynamicBuf;
C2D_Text dynText;
NOTES_T Notes[NOTES_MAX];

int ctoi(char c);
char *get_buffer();
bool tkjload();
void draw_text(float x, float y, const char *text, float r, float g, float b), Reset();

int main() {
	// 初期化
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	g_dynamicBuf = C2D_TextBufNew(4096);
	gfxSetWide(false);
	osSetSpeedupEnable(false);
	gfxSetDoubleBuffering(GFX_TOP, true);
	touchPosition tp;	//下画面タッチした座標

	// 描画バッファ
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	load_sound();
	Reset();

	while (aptMainLoop()) {

		hidScanInput();
		hidTouchRead(&tp);
		unsigned int key = hidKeysDown();
		if (isExit == true) break;

		//オプション関係
		if (key & KEY_START) isExit = true;	//ソフトを閉じる
		if (key & KEY_X) Reset();		//最初からやり直す
		if (key & KEY_A) isAuto = !isAuto;	//オート切り替え
		if (key & KEY_L) {	//難易度を下げる。その難易度が無かったら戻す
			--course;
			if (tkjload() == false) {
				++course;
				Reset();
			}
		}
		if (key & KEY_R) {	//難易度を上げる。その難易度が無かったら戻す
			++course;
			if (tkjload() == false) {
				--course;
				Reset();
			}
		}
		if (key & KEY_DUP && NotesSpeed < 400) NotesSpeed += 10;
		if (key & KEY_DDOWN && NotesSpeed > 100) NotesSpeed -= 10;

		//描画開始
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		//上画面に描画
		C2D_TargetClear(top, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
		C3D_FrameDrawOn(top);
		C2D_SceneTarget(top);

		switch (scene) {

			case 0:	//演奏画面

			//差を使って時間を測る
			if (timecnt == 0) OffTime = osGetTime() * 0.001;
			++timecnt;
			NowTime = osGetTime() * 0.001 - OffTime;

			touchid = -1;

			PreTouch_x = touch_x, PreTouch_y = touch_y;
			touch_x = tp.px, touch_y = tp.py;

			//タッチ関係
			if (touch_x != 0 && touch_y != 0) touchid = (int)tp.px / 80;
			if (PreTouch_x != 0 && PreTouch_y != 0) touchid = -1;
			if (touchid != -1) play_sound(0);

			//レーン描画
			C2D_DrawRectSolid(39,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(119.75,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(199.5,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(279.25,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(359,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//ノーツ判定
			int NotesJudge[4] = { -1,-1,-1,-1 };
			double NotesJudgeLag[4] = { 1,1,1,1 };
			for (int i = 0; i < MaxNotesCnt; ++i) {

				if (Notes[i].flag) {

					//オート時の判定
					if (isAuto && Notes[i].judge_time < NowTime) {
						Notes[i].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 0;
						Score += 100;
						play_sound(0);
					}
					if (NotesJudgeLag[Notes[i].num] > fabs(Notes[i].judge_time - NowTime)) {
						NotesJudge[Notes[i].num] = i;
						NotesJudgeLag[Notes[i].num] = fabs(Notes[i].judge_time - NowTime);
					}
				}
			}
			if (!isAuto) {
				for (int i = 0; i < 4; ++i) {
					if (NotesJudgeLag[i] < DEFAULT_JUDGE_RANGE_PERFECT && touchid == Notes[NotesJudge[i]].num) {
						Notes[NotesJudge[i]].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 0;
						Score += 1000;
						++Combo;
					}
					else if (NotesJudgeLag[i] < DEFAULT_JUDGE_RANGE_NICE && touchid == Notes[NotesJudge[i]].num) {
						Notes[NotesJudge[i]].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 1;
						Score += 400;
						++Combo;
					}
					else if (NotesJudgeLag[i] < DEFAULT_JUDGE_RANGE_BAD && touchid == Notes[NotesJudge[i]].num) {
						Notes[NotesJudge[i]].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 2;
						Score += 240;
						Combo = 0;
					}
				}
			}

			//ノーツ描画
			for (int i = 0; i < MaxNotesCnt; ++i) {

				if (Notes[i].flag) {

					//位置計算
					Notes[i].y = (JUDGE_Y - 2) - (Notes[i].judge_time - NowTime) * NotesSpeed;
					if (Notes[i].y < 5.0f && Notes[i].y > -240.0f) C2D_DrawRectSolid(39 + 79.75 * Notes[i].num,TOP_HEIGHT + Notes[i].y,0,80,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
				}
			}

			//スコア表示
			C2D_DrawRectSolid(0,0,0,TOP_WIDTH,16,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			snprintf(get_buffer(), BUFFER_SIZE, "SCORE:%.8d COMBO:%.6d", Score, Combo);
			draw_text(TOP_WIDTH / 2, 0, get_buffer(), 0,0,0);
			
			//下画面に移動
			C2D_TargetClear(bot, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
			C3D_FrameDrawOn(bot);
			C2D_SceneTarget(bot);

			//レーン描画
			C2D_DrawRectSolid(0,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(79.75,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(159.5,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(239.25,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(319,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//ノーツ描画
			for (int i = 0; i < MaxNotesCnt; ++i) {

				if (Notes[i].flag && Notes[i].y > -5.0f) {

					if (Notes[i].y > BOTTOM_HEIGHT) {
						Notes[i].flag = false;
						Combo = 0;
					}
					C2D_DrawRectSolid(79.75 * Notes[i].num,Notes[i].y,0,80,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
				}
			}

			//判定線
			C2D_DrawRectSolid(0,JUDGE_Y,0,BOTTOM_WIDTH,1,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//判定文字
			if (timecnt < judgetmpcnt) {
				if (judgeid == 0) {
					snprintf(get_buffer(), BUFFER_SIZE, "PERFECT");
					draw_text(BOTTOM_WIDTH / 2, 0, get_buffer(), 1,1,0);
				}
				else if (judgeid == 1) {
					snprintf(get_buffer(), BUFFER_SIZE, "GOOD");
					draw_text(BOTTOM_WIDTH / 2, 0, get_buffer(), 1,0,0);
				}
				else if (judgeid == 2) {
					snprintf(get_buffer(), BUFFER_SIZE, "MISS");
					draw_text(BOTTOM_WIDTH / 2, 0, get_buffer(), 0,0,1);
				}
			}
			else judgeid = -1;

			//デバッグ用テキスト
			/*snprintf(get_buffer(), BUFFER_SIZE, "%d", MaxNotesCnt);
			draw_text(BOTTOM_WIDTH / 2, 0, get_buffer(), 0,1,0);*/

			//曲再生
			if (timecnt == 60) {
				isPlayMain = true;
				play_main_music(&isPlayMain);
			}
			break;
		}

		//描画終了
		C3D_FrameEnd(0);
	}

	//リソースの解放
	stop_main_music();
	C2D_TextBufDelete(g_dynamicBuf);

	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	exit_music();
	return 0;
}

char *get_buffer() {
	return buffer;
}

void draw_text(float x, float y, const char *text, float r, float g, float b) {

	//使用例
	//snprintf(get_buffer(), BUFFER_SIZE, "%d", 10);
	// draw_debug(300, 0, get_buffer());

	C2D_TextBufClear(g_dynamicBuf);
	C2D_TextParse(&dynText, g_dynamicBuf, text);
	C2D_TextOptimize(&dynText);
	C2D_DrawText(&dynText, C2D_WithColor | C2D_AlignCenter, x, y, 0.5f, 0.5f, 0.5f, C2D_Color32f(r, g, b, 1.0f));
}

bool tkjload() {

	FILE *fp;
	char* temp = NULL;
	isCourseMatch = false;

	chdir("sdmc:/tkjfiles/");
	if ((fp = fopen("test.tkj", "r")) != NULL) {

		tkj_cnt = 0;
		while ((fgets(tkj_notes[tkj_cnt], NOTES_MEASURE_MAX, fp) != NULL || tkj_cnt < MEASURE_MAX)) {

			if (strstr(tkj_notes[tkj_cnt], "BPM:") == tkj_notes[tkj_cnt]) {
				temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
				if (tkj_notes[tkj_cnt][4] != '\n' && tkj_notes[tkj_cnt][4] != '\r') {
					strlcpy(temp, tkj_notes[tkj_cnt] + 4, strlen(tkj_notes[tkj_cnt]) - 5);
					BPM = atof(temp);
				}
				free(temp);
				++tkj_cnt;
				continue;
			}
			if (strstr(tkj_notes[tkj_cnt], "OFFSET:") == tkj_notes[tkj_cnt]) {
				temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
				if (tkj_notes[tkj_cnt][7] != '\n' && tkj_notes[tkj_cnt][7] != '\r') {
					strlcpy(temp, tkj_notes[tkj_cnt] + 7, strlen(tkj_notes[tkj_cnt]) - 8);
					OFFSET = atof(temp);
				}
				free(temp);
				++tkj_cnt;
				continue;
			}
			if (strstr(tkj_notes[tkj_cnt], "COURSE:") == tkj_notes[tkj_cnt]) {
				temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
				strlcpy(temp, tkj_notes[tkj_cnt] + 7, strlen(tkj_notes[tkj_cnt]) - 8);
				if (strlen(temp) == 1) CurrentCourse = atoi(temp);		//数字表記
				else if (strcmp(temp, "Easy") ==   0 || strcmp(temp, "easy") == 0)   CurrentCourse = COURSE_EASY;	//文字表記
				else if (strcmp(temp, "Normal") == 0 || strcmp(temp, "normal") == 0) CurrentCourse = COURSE_NORMAL;
				else if (strcmp(temp, "Hard") ==   0 || strcmp(temp, "hard") == 0)   CurrentCourse = COURSE_HARD;
				else if (strcmp(temp, "Crazy") ==  0 || strcmp(temp, "crazy") == 0)  CurrentCourse = COURSE_CRAZY;
				free(temp);
				if (course == CurrentCourse) isCourseMatch = true;
				++tkj_cnt;
				continue;
			}
			if (isCourseMatch == true && strstr(tkj_notes[tkj_cnt], "#START") == tkj_notes[tkj_cnt]) Startcnt = tkj_cnt + 1;
			if (isCourseMatch == true && strstr(tkj_notes[tkj_cnt], "#END") == tkj_notes[tkj_cnt]) break;
			++tkj_cnt;
		}
		fclose(fp);
	}
	return isCourseMatch;
}

int ctoi(char c) {

	switch (c) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	default: return -1;
	}
}

void Reset() {
	scene = 0,timecnt = 0,judgetmpcnt = 0,touchid = -1,judgeid = -1,tkj_cnt = 0,NotesCount = 0,CurrentCourse = -1;
	MaxNotesCnt = 0,Startcnt = 0,MeasureCount = 0,Score = 0,Combo = 0,BPM = 120.0,OFFSET = 0,OffTime = 0,NowTime = 0;
	isExit = false,isPlayMain = true;
	stop_main_music();
	tkjload();
	MeasureCount = Startcnt;
	while (MeasureCount < tkj_cnt) {
		NotesCount = 0;
		while (tkj_notes[MeasureCount][NotesCount] != ',' && tkj_notes[MeasureCount][NotesCount] != '\n') ++NotesCount;
		for (int i = 0; i < NotesCount; ++i) {
			if (ctoi(tkj_notes[MeasureCount][i]) != 0) {
				Notes[MaxNotesCnt].flag = true;
				Notes[MaxNotesCnt].num = ctoi(tkj_notes[MeasureCount][i]) - 1;
				Notes[MaxNotesCnt].judge_time = (1.222 + OFFSET) + (240.0 / BPM * (MeasureCount - Startcnt)) + (240.0 / BPM * i / NotesCount);
				++MaxNotesCnt;
			}
		}
		++MeasureCount;
	}
}
