#include "audio.h"
#include "header.h"
#include "main.h"
#include "playback.h"
#include "vorbis.h"

#include <citro2d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <time.h>

json_t *json;
json_error_t error_json;

struct timespec tv;
char buffer[BUFFER_SIZE];
char buf_select[256];
char tkj_notes[MEASURE_MAX][NOTES_MEASURE_MAX];	//ノーツ情報
int scene = 0,timecnt = 0,judgetmpcnt = 0,touchid = -1,judgeid = -1,tkj_cnt = 0,
SongNumber = 0,ratio = 0,NotesCount = 0,MaxNotesCnt = 0,Startcnt = 0,MeasureCount = 0,
course = COURSE_HARD,CurrentCourse = -1,keys = 4,
SongCount = 0,cursor = 0,course_cursor = 0,course_count = 0,SelectedId = 0,	//選曲画面用
touch_x,touch_y,PreTouch_x,PreTouch_y,PreTouchId,	//タッチ用
Combo = 0,Score = 0,NotesSpeed = 200;	//演奏用
double BPM = 120.0,SCROLL = 1.0,MEASURE = 1.0,OFFSET = 0,Note_x = 79.75,
tmpjudgetime = 0,OffTime = 0,SetTime = 0,NowTime = 0;
bool isExit = false,isPlayMain = false,isAuto = false,isCourseMatch = false,
isSelectCourse = false,isGameStart = false,isPause = false,Rubbing = false;

//static C2D_SpriteSheet spriteSheet;
C2D_TextBuf g_dynamicBuf;
C2D_Text dynText;
NOTES_T Notes[NOTES_MAX];
LIST_T List[LIST_MAX];
COMMAND_T Cmd[MEASURE_MAX];

int ctoi(char c);
char *get_buffer();
bool tkjload(),checknotes();
void draw_text(float x, float y, const char *text, float r, float g, float b),load_option();
void load_file_list(const char* path),Reset(),load_tkj_head_simple(LIST_T *List),exit_option();
void draw_select_text(float x, float y, const char *text),select_ini(),load_file_main(),disp_file_list();

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

	load_option();
	load_sound();

	while (aptMainLoop()) {

		hidScanInput();
		hidTouchRead(&tp);
		unsigned int key = hidKeysDown();
		int checknote = 0;
		if (isExit == true) break;

		//描画開始
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		//上画面に描画
		C2D_TargetClear(top, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
		C3D_FrameDrawOn(top);
		C2D_SceneTarget(top);

		switch (scene) {

		case 0:	//ロード画面

			snprintf(get_buffer(), BUFFER_SIZE, "Touch Keys v%s", VERSION);
			draw_select_text(120, 70, get_buffer());
			draw_select_text(120, 100, "Now Loading...");
			C3D_FrameEnd(0);
			load_file_main();
			scene = 1;	//選曲画面に移る
			select_ini();
			break;

		case 1:	//選曲画面

			//十字キーか左スティックで移動
			if (key & KEY_UP) {
				if (isSelectCourse == false) ++cursor;
				else if (course_cursor > 0) --course_cursor;
			}
			if (key & KEY_DOWN) {
				if (isSelectCourse == false) --cursor;
				else if (course_cursor < (course_count - 1)) ++course_cursor;
			}
			if (key & KEY_RIGHT) {
				if (isSelectCourse == false) cursor -= 5;
			}
			if (key & KEY_LEFT) {
				if (isSelectCourse == false) cursor += 5;
			}
			if (key & KEY_A && course_count != 0) {
				if (isSelectCourse == true) isGameStart = true;
				else isSelectCourse = true;
			}
			if (key & KEY_B) {
				isSelectCourse = false;
				course_cursor = 0;
			}
			disp_file_list();			//リスト表示

			//下画面に移動
			C2D_TargetClear(bot, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
			C3D_FrameDrawOn(bot);
			C2D_SceneTarget(bot);

			isPause = false;			//ポーズは不要なのでオフにする
			if (key & KEY_START) isExit = true;	//ソフトを閉じる
			if (isGameStart) {
				Reset();			//演奏画面に移ります
			}
			break;

		case 3:	//演奏画面

			//演奏オプション
			if (key & KEY_X) Reset();		//最初からやり直す
			if (key & KEY_A) isAuto = !isAuto;	//オート切り替え
			if (key & KEY_DUP && NotesSpeed < 600) NotesSpeed += 10;	//最大値(600)まで速度を上げれる
			if (key & KEY_DDOWN && NotesSpeed > 100) NotesSpeed -= 10;	//最低値(100)まで速度を下げれる
			//if (key & KEY_START) isPause = !isPause;

			//差を使って時間を測る
			clock_gettime(CLOCK_MONOTONIC, &tv);
			//if (timecnt == 0) OffTime = osGetTime() * 0.001;
			if (timecnt == 0) {
				OffTime = tv.tv_sec + tv.tv_nsec * 0.000000001;
				isPlayMain = false;
				play_main_music(&isPlayMain, List[SelectedId]);
			}
			++timecnt;
			//NowTime = osGetTime() * 0.001 - OffTime;
			NowTime = tv.tv_sec + tv.tv_nsec * 0.000000001 - OffTime;
			if (NowTime > Cmd[MeasureCount].time) ++MeasureCount;

			//曲再生
			if (NowTime >= 1.0 && !isPlayMain) {
				isPlayMain = true;
			}

			//レーン描画(上画面用)
			C2D_DrawRectSolid(40,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(359,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//ノーツ描画
			for (int i = 0; i < MaxNotesCnt; ++i) {

				if (Notes[i].flag) {

					//位置計算
					Note_x = 319.0 / Notes[i].keys;
					Notes[i].y = Cmd[MeasureCount].judge - (Notes[i].judge_time - NowTime) * NotesSpeed * Notes[i].scroll;
					if (Notes[i].y < TOP_HEIGHT * -1 && Notes[i].scroll < 0) {
						Notes[i].flag = false;
						Combo = 0;
					}
					if (Notes[i].y < 5.0f && Notes[i].y > -240.0f) C2D_DrawRectSolid(39 + Note_x * Notes[i].num,TOP_HEIGHT + Notes[i].y,0,Note_x,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
				}
			}

			//曲の進み具合を太線で表す
			ratio = vorbis_ratio();
			C2D_DrawRectSolid(0,0,0,ratio,20,C2D_Color32(0xF4, 0xB7, 0x00, 0xFF));
			C2D_DrawRectSolid(ratio,0,0,TOP_WIDTH - ratio,20,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			snprintf(get_buffer(), BUFFER_SIZE, "SCORE:%.7d COMBO:%.4d SPEED:%.1f", Score, Combo, NotesSpeed*0.01);
			draw_text(TOP_WIDTH / 2, 0, get_buffer(), 0,0,0);
			
			//下画面に移動
			C2D_TargetClear(bot, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
			C3D_FrameDrawOn(bot);
			C2D_SceneTarget(bot);

			for (int i = 0; i < MaxNotesCnt; ++i) {
				if (Notes[i].flag) {
					checknote = i;
					break;
				}
			}

			touchid = -1, Rubbing = false;
			PreTouch_x = touch_x, PreTouch_y = touch_y;
			touch_x = tp.px, touch_y = tp.py;
			if (touch_x != 0 && touch_y != 0 && !isAuto) touchid = (int)(tp.px / (319.0 / Notes[checknote].keys));
			if (PreTouchId != touchid && touchid != -1) {	//擦り判定
				PreTouchId = touchid;
				Rubbing = true;
			}
			if (PreTouch_x != 0 && PreTouch_y != 0 && !Rubbing) touchid = -1;
			if (touchid != -1) play_sound(0);

			//ノーツ判定
			int NotesJudge[8] = { -1,-1,-1,-1,-1,-1,-1,-1 };
			double NotesJudgeLag[8] = { 1,1,1,1,1,1,1,1 };
			for (int i = 0; i < MaxNotesCnt; ++i) {

				if (Notes[i].flag) {

					if (isAuto && Notes[i].judge_time < NowTime) {
						Notes[i].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 3;
						++Combo;
						play_sound(0);
						C2D_DrawRectangle(0 + (319.0 / Notes[i].keys * Notes[i].num),0,0,319.0 / Notes[i].keys,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0x00),C2D_Color32(0xFF, 0xFF, 0xFF, 0x00),C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF),C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
					}
					if (NotesJudgeLag[Notes[i].num] > fabs(Notes[i].judge_time - NowTime)) {
						NotesJudge[Notes[i].num] = i;
						NotesJudgeLag[Notes[i].num] = fabs(Notes[i].judge_time - NowTime);
					}
				}
			}
			if (!isAuto) {
				for (int i = 0; i < 8; ++i) {
					if (NotesJudgeLag[i] < DEFAULT_JUDGE_RANGE_PERFECT && touchid == Notes[NotesJudge[i]].num) {
						Notes[NotesJudge[i]].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 0;
						Score += 1000000 / MaxNotesCnt + 1;
						++Combo;
					}
					else if (NotesJudgeLag[i] < DEFAULT_JUDGE_RANGE_NICE && touchid == Notes[NotesJudge[i]].num) {
						Notes[NotesJudge[i]].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 1;
						Score += 500000 / MaxNotesCnt + 1;
						++Combo;
					}
					else if (NotesJudgeLag[i] < DEFAULT_JUDGE_RANGE_BAD && touchid == Notes[NotesJudge[i]].num) {
						Notes[NotesJudge[i]].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 2;
						Score += 250000 / MaxNotesCnt + 1;
						Combo = 0;
					}
				}
			}

			//タップエフェクト
			C2D_DrawRectangle(0 + (319.0 / Notes[checknote].keys * touchid),0,0,319.0 / Notes[checknote].keys,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0x00),C2D_Color32(0xFF, 0xFF, 0xFF, 0x00),C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF),C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//レーン描画(下画面用)
			C2D_DrawRectSolid(0,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(319,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//判定線
			C2D_DrawRectSolid(0,Cmd[MeasureCount].judge,0,BOTTOM_WIDTH,1,C2D_Color32(0xFF, 0xAA, 0x00, 0xFF));

			//ノーツ描画
			for (int i = 0; i < MaxNotesCnt; ++i) {

				if (Notes[i].flag) {
					if (Notes[i].y > BOTTOM_HEIGHT && Notes[i].scroll > 0) {
						Notes[i].flag = false;
						Combo = 0;
					}
					Note_x = 319.0 / Notes[i].keys;
					if (Notes[i].y > -5.0f && Notes[i].y < BOTTOM_HEIGHT) C2D_DrawRectSolid(Note_x * Notes[i].num,Notes[i].y,0,Note_x,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
				}
			}

			//判定文字
			if (timecnt < judgetmpcnt) {
				switch (judgeid) {
				case 0:
					snprintf(get_buffer(), BUFFER_SIZE, "PERFECT");
					draw_text(BOTTOM_WIDTH / 2, 160, get_buffer(), 1,1,0);
					break;
				case 1:
					snprintf(get_buffer(), BUFFER_SIZE, "GOOD");
					draw_text(BOTTOM_WIDTH / 2, 160, get_buffer(), 1,0,0);
					break;
				case 2:
					snprintf(get_buffer(), BUFFER_SIZE, "MISS");
					draw_text(BOTTOM_WIDTH / 2, 160, get_buffer(), 0,0,1);
					break;
				case 3:
					snprintf(get_buffer(), BUFFER_SIZE, "AUTO");
					draw_text(BOTTOM_WIDTH / 2, 160, get_buffer(), 1,1,0);
					break;
				}
			}
			else judgeid = -1;

			//デバッグ用テキスト
			/*snprintf(get_buffer(), BUFFER_SIZE, "%.4f", ovtime);
			draw_text(BOTTOM_WIDTH / 2, 0, get_buffer(), 0,1,0);*/

			if ((key & KEY_START || checknotes()) && ndspChnIsPlaying(CHANNEL) == false) {
				isPlayMain = true;
				scene = 1;		//選曲画面に戻る
				isGameStart = false;
				stopPlayback();
			}
			break;
		}

		//描画終了
		C3D_FrameEnd(0);
	}

	//リソースの解放
	stop_main_music();
	C2D_TextBufDelete(g_dynamicBuf);
	exit_option();

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

inline void draw_text(float x, float y, const char *text, float r, float g, float b) {

	//使用例
	//snprintf(get_buffer(), BUFFER_SIZE, "%d", 10);
	//draw_debug(300, 0, get_buffer());

	C2D_TextBufClear(g_dynamicBuf);
	C2D_TextParse(&dynText, g_dynamicBuf, text);
	C2D_TextOptimize(&dynText);
	C2D_DrawText(&dynText, C2D_WithColor | C2D_AlignCenter, x, y, 0.5f, 0.5f, 0.5f, C2D_Color32f(r, g, b, 1.0f));
}

inline bool tkjload() {

	FILE *fp;
	char* temp = NULL;
	isCourseMatch = false;

	chdir(List[SelectedId].path);
	if ((fp = fopen(List[SelectedId].tkj, "r")) != NULL) {

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
			if (isCourseMatch == true) {

				if (strstr(tkj_notes[tkj_cnt], "#END") == tkj_notes[tkj_cnt]) break;

				if (Startcnt != -1 && tkj_notes[tkj_cnt][0] != '#') {
					NotesCount = 0;
					while (tkj_notes[tkj_cnt][NotesCount] != ',' && tkj_notes[tkj_cnt][NotesCount] != '\n') ++NotesCount;
					for (int i = 0; i < NotesCount; ++i) {
						if (ctoi(tkj_notes[tkj_cnt][i]) != -1) {
							Notes[MaxNotesCnt].flag = true;
							Notes[MaxNotesCnt].num = ctoi(tkj_notes[tkj_cnt][i]);
							Notes[MaxNotesCnt].keys = keys;
							Notes[MaxNotesCnt].scroll = SCROLL;
							Notes[MaxNotesCnt].judge_time = OFFSET + tmpjudgetime + (240.0 / BPM * MEASURE * i / NotesCount);
							++MaxNotesCnt;
						}
					}
					++MeasureCount;
					tmpjudgetime += (240.0 / BPM) * MEASURE;
					Cmd[MeasureCount].judge = Cmd[MeasureCount - 1].judge;
				}

				if (strstr(tkj_notes[tkj_cnt], "#BPMCHANGE:") == tkj_notes[tkj_cnt]) {
					temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
					if (tkj_notes[tkj_cnt][11] != '\n' && tkj_notes[tkj_cnt][11] != '\r') {
						strlcpy(temp, tkj_notes[tkj_cnt] + 11, strlen(tkj_notes[tkj_cnt]) - 12);
						BPM = atof(temp);
					}
					free(temp);
				}

				if (strstr(tkj_notes[tkj_cnt], "#MEASURE:") == tkj_notes[tkj_cnt]) {
					temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
					if (tkj_notes[tkj_cnt][9] != '\n' && tkj_notes[tkj_cnt][9] != '\r') {
						strlcpy(temp, tkj_notes[tkj_cnt] + 9, strlen(tkj_notes[tkj_cnt]) - 10);
						MEASURE = atof(temp);
					}
					free(temp);
				}

				if (strstr(tkj_notes[tkj_cnt], "#SCROLL:") == tkj_notes[tkj_cnt]) {
					temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
					if (tkj_notes[tkj_cnt][8] != '\n' && tkj_notes[tkj_cnt][8] != '\r') {
						strlcpy(temp, tkj_notes[tkj_cnt] + 8, strlen(tkj_notes[tkj_cnt]) - 9);
						SCROLL = atof(temp);
					}
					free(temp);
				}

				if (strstr(tkj_notes[tkj_cnt], "#DELAY:") == tkj_notes[tkj_cnt]) {
					temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
					if (tkj_notes[tkj_cnt][7] != '\n' && tkj_notes[tkj_cnt][7] != '\r') {
						strlcpy(temp, tkj_notes[tkj_cnt] + 7, strlen(tkj_notes[tkj_cnt]) - 8);
						tmpjudgetime += atof(temp);
					}
					free(temp);
				}

				if (strstr(tkj_notes[tkj_cnt], "#KEYCHANGE:") == tkj_notes[tkj_cnt]) {
					temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
					if (tkj_notes[tkj_cnt][11] != '\n' && tkj_notes[tkj_cnt][11] != '\r') {
						strlcpy(temp, tkj_notes[tkj_cnt] + 11, strlen(tkj_notes[tkj_cnt]) - 12);
						keys = atoi(temp);
						if (keys > 8) keys = 8;
						else if (keys < 1) keys = 1;
					}
					free(temp);
				}

				if (strstr(tkj_notes[tkj_cnt], "#JUDGECHANGE:") == tkj_notes[tkj_cnt]) {
					temp = (char *)malloc((strlen(tkj_notes[tkj_cnt]) + 1));
					if (tkj_notes[tkj_cnt][13] != '\n' && tkj_notes[tkj_cnt][13] != '\r') {
						strlcpy(temp, tkj_notes[tkj_cnt] + 13, strlen(tkj_notes[tkj_cnt]) - 14);
						Cmd[MeasureCount].judge = atoi(temp);
						if (Cmd[MeasureCount].judge > 200) Cmd[MeasureCount].judge = 200;
						else if (Cmd[MeasureCount].judge < 0) Cmd[MeasureCount].judge = 0;
					}
					free(temp);
				}

				Cmd[MeasureCount].time = tmpjudgetime + OFFSET + (240.0 / BPM) * MEASURE;
				if (strstr(tkj_notes[tkj_cnt], "#START") == tkj_notes[tkj_cnt]) Startcnt = tkj_cnt + 1;
			}
			++tkj_cnt;
		}
		fclose(fp);
	}
	MeasureCount = 0;
	return isCourseMatch;
}

inline int ctoi(char c) {

	switch (c) {
	case '1': return 0;
	case '2': return 1;
	case '3': return 2;
	case '4': return 3;
	case '5': return 4;
	case '6': return 5;
	case '7': return 6;
	case '8': return 7;
	default: return -1;
	}
}

inline void Reset() {
	scene = 3,timecnt = 0,judgetmpcnt = 0,touchid = -1,judgeid = -1,tkj_cnt = 0,NotesCount = 0,CurrentCourse = -1;
	MaxNotesCnt = 0,Startcnt = -1,MeasureCount = 0,Score = 0,Combo = 0,keys = 4,Note_x = 79.75,Cmd[0].judge = 200;
	BPM = 120.0,SCROLL = 1.0,MEASURE = 1.0,OFFSET = 0.0,OffTime = 0,NowTime = 0,ratio = 0,tmpjudgetime = 1.0;
	isExit = false,isPlayMain = true;
	stopPlayback();
	tkjload();
}

inline void load_file_main() {

	chdir(DEFAULT_DIR);
	load_file_list(DEFAULT_DIR);
	SongNumber = SongCount;
}

inline void load_file_list(const char* path) {

	DIR* dir;
	struct dirent* dp;

	if ((dir = opendir(path)) != NULL) {

		DIR* db;
		char filename[512];
		while ((dp = readdir(dir)) != NULL) {

			chdir(path);

			strlcpy(filename, path, strlen(path));
			strcat(filename, "/");
			strcat(filename, dp->d_name);

			db = opendir(filename);

			struct stat st;
			stat(dp->d_name, &st);

			if ((st.st_mode & S_IFMT) != S_IFDIR) {

				if (db == NULL) {

					if (strstr(dp->d_name, ".tkj") != NULL) {

						strlcpy(List[SongCount].tkj, dp->d_name, strlen(dp->d_name) + 1);
						getcwd(List[SongCount].path, 256);
						load_tkj_head_simple(&List[SongCount]);
						++SongCount;
					}
				}
			}
			else {
				load_file_list(dp->d_name);
				chdir("../");
			}
			closedir(db);
		}
	}
	closedir(dir);
}

inline void load_tkj_head_simple(LIST_T *List) {		//選曲用のヘッダ取得

	snprintf(List->title, sizeof(List->title), "No Title");
	snprintf(List->wave, sizeof(List->wave), "audio.ogg");
	for (int i = 0; i < 4; ++i) {
		List->level[i] = 0;
		List->course[i] = false;
	}
	FILE *fp;
	char buf[128],*temp = NULL;
	int course = COURSE_CRAZY,cnt = 0;

	chdir(List->path);
	if ((fp = fopen(List->tkj, "r")) != NULL) {

		while (fgets(buf, 128, fp) != NULL) {

			temp = (char *)malloc((strlen(buf) + 1));

			if (strstr(buf, "TITLE:") == buf) {
				if (buf[6] != '\n' && buf[6] != '\r') {
					strlcpy(List->title, buf + 6, strlen(buf) - 7);
				}
				continue;
			}
			if (strstr(buf, "COURSE:") == buf) {
				if (buf[7] != '\n' && buf[7] != '\r') {
					strlcpy(temp, buf + 7, strlen(buf) - 8);
					if (strlen(temp) == 1) course = atoi(temp);			//数字表記
					else if (strcmp(temp, "Easy") == 0 || strcmp(temp, "easy") == 0)   course = COURSE_EASY;	//文字表記
					else if (strcmp(temp, "Normal") == 0 || strcmp(temp, "normal") == 0) course = COURSE_NORMAL;
					else if (strcmp(temp, "Hard") == 0 || strcmp(temp, "hard") == 0)   course = COURSE_HARD;
					else if (strcmp(temp, "Crazy") == 0 || strcmp(temp, "crazy") == 0)    course = COURSE_CRAZY;

					List->course[course] = true;
					List->course_exist[course] = true;
				}
				continue;
			}
			if (strstr(buf, "LEVEL:") == buf) {
				if (buf[6] != '\n' && buf[6] != '\r') {
					strlcpy(temp, buf + 6, strlen(buf) - 7);
					List->level[course] = atoi(temp);
					List->course[course] = true;
				}
				continue;
			}
			if (strstr(buf, "WAVE:") == buf) {
				if (buf[5] != '\n' && buf[5] != '\r') {
					strlcpy(List->wave, buf + 5, strlen(buf) - 6);
				}
				continue;
			}
			++cnt;
		}
	}
	free(temp);
	fclose(fp);
}

inline void disp_file_list() {

	int n = 0;
	course_count = 0;

	if (cursor > 0) cursor = -1*(SongNumber - 1);
	if (cursor < -1 * (SongNumber - 1)) cursor = 0;

	for (int i = 0; i < SongNumber; i++) {

		if ((n + cursor) * 20 + 60 >= 0 && (n + cursor) * 20 + 60 <= 220) {

			draw_select_text(30, (n + cursor) * 20 + 60, List[i].title);

			if (i != (cursor*-1)) {
				snprintf(buf_select, sizeof(buf_select), "★x%d", List[i].level[COURSE_CRAZY]);
				draw_select_text(360, (n + cursor) * 20 + 60, buf_select);
			}
		}

		n++;

		if (i == (cursor*-1)) {

			SelectedId = i;

			if (List[i].course[COURSE_CRAZY] == true) {

				if ((n + cursor - 1) == course_cursor) course = COURSE_CRAZY;
				draw_select_text(80, (n + cursor) * 20 + 60, "CRAZY");
				snprintf(buf_select, sizeof(buf_select), "★x%d", List[i].level[COURSE_CRAZY]);
				draw_select_text(160, (n + cursor) * 20 + 60, buf_select);
				n++;
				course_count++;
			}

			if (List[i].course[COURSE_HARD] == true) {

				if ((n + cursor - 1) == course_cursor) course = COURSE_HARD;
				draw_select_text(80, (n + cursor) * 20 + 60, "HARD");
				snprintf(buf_select, sizeof(buf_select), "★x%d", List[i].level[COURSE_HARD]);
				draw_select_text(160, (n + cursor) * 20 + 60, buf_select);
				n++;
				course_count++;
			}

			if (List[i].course[COURSE_NORMAL] == true) {

				if ((n + cursor - 1) == course_cursor) course = COURSE_NORMAL;
				draw_select_text(80, (n + cursor) * 20 + 60, "NORMAL");
				snprintf(buf_select, sizeof(buf_select), "★x%d", List[i].level[COURSE_NORMAL]);
				draw_select_text(160, (n + cursor) * 20 + 60, buf_select);
				n++;
				course_count++;
			}

			if (List[i].course[COURSE_EASY] == true) {

				if ((n + cursor - 1) == course_cursor) course = COURSE_EASY;
				draw_select_text(80, (n + cursor) * 20 + 60, "EASY");
				snprintf(buf_select, sizeof(buf_select), "★x%d", List[i].level[COURSE_EASY]);
				draw_select_text(160, (n + cursor) * 20 + 60, buf_select);
				n++;
				course_count++;
			}

			if (isSelectCourse == true) {

				draw_select_text(60, (course_cursor+1) * 20 + 60, ">>");
				//snprintf(buf_select, sizeof(buf_select), "%d",course_cursor);
				//draw_select_text(60, (course_cursor + 1) * 20 + 60, buf_select);
			}
		}
	}
	draw_select_text(10, 60, ">>");
	//snprintf(buf_select, sizeof(buf_select), "%d",course);
	//draw_select_text(0, 60, buf_select);
}

C2D_TextBuf g_SelectText = C2D_TextBufNew(4096);
C2D_Text SelectText;

void draw_select_text(float x, float y, const char *text) {

	C2D_TextBufClear(g_SelectText);
	C2D_TextParse(&SelectText, g_SelectText, text);
	C2D_TextOptimize(&SelectText);
	C2D_DrawText(&SelectText, C2D_WithColor, x, y, 1.0f, 0.5f, 0.5f, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
}

inline void select_ini() {
	//cursor = 0;
	course_cursor = 0;
	course_count = 0;
	SelectedId = 0;
	course = COURSE_CRAZY;
	isSelectCourse = false;
	isGameStart = false;
}

inline bool checknotes() {
	for (int i = 0; i < MaxNotesCnt; ++i) {
		if (Notes[i].flag) return false;
	}
	return true;
}

inline void save_option() {

	json_object_set(json, "NotesSpeed", json_integer(NotesSpeed));
	json_object_set(json, "isAuto", json_boolean(isAuto));
	
	json_dump_file(json, SETTING_FILE, 0);
}

inline void load_option() {
	
	json = json_load_file(SETTING_FILE, 0, &error_json);

	if (json != NULL){

		NotesSpeed = json_integer_value(json_object_get(json, "NotesSpeed"));
		isAuto = json_boolean_value(json_object_get(json, "isAuto"));
	}
	if (json == NULL) {			//開けなかった時
		json = json_pack("{}");	//ファイル空の時はこれしないとセットできなくなる
		save_option();			//書き込み
	}
}

inline void exit_option() {

	save_option();
	json_decref(json);
}
