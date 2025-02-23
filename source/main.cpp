#include "audio.h"
#include "header.h"
#include "main.h"

#include <citro2d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SDカードからテクスチャを読み込む
const char* texturePath = "sdmc:/3ds/touch/image.t3x";
char buffer[BUFFER_SIZE];
char tja_notes[MEASURE_MAX][NOTES_MEASURE_MAX];	//ノーツ情報
int scene = 0,timecnt = 0,judgetmpcnt = 0,NotesSpeed = 200,touchid = -1,judgeid = -1,tkj_cnt = 0;
double BPM = 120,OffTime = 0,NowTime = 0;
bool isExit = false;

//static C2D_SpriteSheet spriteSheet;
C2D_TextBuf g_dynamicBuf;
C2D_Text dynText;
NOTES_T Notes[NOTES_MAX];

char *get_buffer();
void draw_text(float x, float y, const char *text, float r, float g, float b);

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

	//spriteSheet = C2D_SpriteSheetLoad(texturePath);
	load_sound();

	for (int i = 0; i < 8; ++i) {
		Notes[i].flag = true;
		Notes[i].num = i % 4;
		Notes[i].judge_time = 1 + (0.5 * i);
	}

	while (aptMainLoop()) {

		hidScanInput();
		hidTouchRead(&tp);
		unsigned int key = hidKeysDown();
		if (isExit == true) break;

		//描画開始
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		//上画面に描画
		C2D_TargetClear(top, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
		C3D_FrameDrawOn(top);
		C2D_SceneTarget(top);

		switch (scene) {

		case 0:	//テスト用画面

			//差を使って時間を測る
			if (timecnt == 0) OffTime = osGetTime() * 0.001;
			++timecnt;
			NowTime = osGetTime() * 0.001 - OffTime;

			//タッチ関係
			if (tp.px != 0 && tp.py != 0 && touchid == -1) {
				touchid = (int)tp.px / 80;
				play_sound(0);
			}
			else if (tp.px == 0 && tp.py == 0 && touchid != -1) touchid = -1;

			//レーン描画
			C2D_DrawRectSolid(39,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(119.75,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(199.5,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(279.25,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(359,0,0,1,TOP_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//ノーツ描画
			for (int i = 0; i < NOTES_MEASURE_MAX; ++i) {

				if (Notes[i].flag) {

					if (fabs(Notes[i].judge_time - NowTime) < DEFAULT_JUDGE_RANGE_PERFECT && touchid == Notes[i].num) {
						Notes[i].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 0;
					}
					else if (fabs(Notes[i].judge_time - NowTime) < DEFAULT_JUDGE_RANGE_NICE && touchid == Notes[i].num) {
						Notes[i].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 1;
					}
					else if (fabs(Notes[i].judge_time - NowTime) < DEFAULT_JUDGE_RANGE_BAD && touchid == Notes[i].num) {
						Notes[i].flag = false;
						judgetmpcnt = timecnt + 30;
						judgeid = 2;
					}
					Notes[i].y = JUDGE_Y - (Notes[i].judge_time - NowTime) * NotesSpeed;
					if (Notes[i].y < 5.0f) C2D_DrawRectSolid(40 + 79.75 * Notes[i].num,TOP_HEIGHT + Notes[i].y,0,80,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
				}
			}
			//下画面に描画（必要に応じて描画）
			C2D_TargetClear(bot, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
			C3D_FrameDrawOn(bot);
			C2D_SceneTarget(bot);
			if (key & KEY_START) isExit = true;

			//レーン描画
			C2D_DrawRectSolid(0,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(79.75,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(159.5,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(239.25,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			C2D_DrawRectSolid(319,0,0,1,BOTTOM_HEIGHT,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//ノーツ描画
			for (int i = 0; i < NOTES_MEASURE_MAX; ++i) {

				if (Notes[i].flag && Notes[i].y > -5.0f) {

					if (Notes[i].y > BOTTOM_HEIGHT) Notes[i].flag = false;
					C2D_DrawRectSolid(79.75 * Notes[i].num,Notes[i].y,0,80,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
				}
			}

			//判定線
			C2D_DrawRectSolid(0,JUDGE_Y,0,BOTTOM_WIDTH,1,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

			//判定文字
			if (timecnt < judgetmpcnt) {
				if (judgeid == 0) {
					snprintf(get_buffer(), BUFFER_SIZE, "PERFECT");
					draw_text(BOTTOM_WIDTH / 2, 170, get_buffer(), 1,1,0);
				}
				else if (judgeid == 1) {
					snprintf(get_buffer(), BUFFER_SIZE, "GOOD");
					draw_text(BOTTOM_WIDTH / 2, 170, get_buffer(), 1,0,0);
				}
				else if (judgeid == 2) {
					snprintf(get_buffer(), BUFFER_SIZE, "MISS");
					draw_text(BOTTOM_WIDTH / 2, 170, get_buffer(), 0,0,1);
				}
			}
			else judgeid = -1;
			break;
		}

		//描画終了
		C3D_FrameEnd(0);
	}

	//リソースの解放
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

/*void tkjload() {

	char* temp = NULL;
	bool isEnd = false;

	chdir("sdmc:/tkjfiles/");
	if ((fp = fopen(test.tkj, "r")) != NULL) {

		tkj_cnt = 0;
		while ((fgets(tkj_notes[tkj_cnt], NOTES_MEASURE_MAX, fp) != NULL || tja_cnt < MEASURE_MAX) && isEnd == false) {
			tkj_cnt++;
			temp = (char *)malloc((strlen() + 1));

			if (strstr(buf, "BPM:") == buf) {
				if (buf[4] != '\n' && buf[4] != '\r') {
					strlcpy(temp, buf + 4, strlen(buf) - 5);
					BPM = atof(temp);
				}
				continue;
			}
		}
		fclose(fp);
	}
}*/
