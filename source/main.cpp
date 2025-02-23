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
int scene = 0,cnt = 0,NotesSpeed = 100,touchid = -1;
double BPM = 120,OffTime = 0,NowTime = 0;
bool isExit = false;

//static C2D_SpriteSheet spriteSheet;
C2D_TextBuf g_dynamicBuf;
C2D_Text dynText;

NOTES_T Notes[NOTES_MAX];

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

	for (int i = 0; i < 4; ++i) 
	Notes[i].flag = true;
	Notes[i].num = i % 4;
	Notes[i].judge_time = 1 + (0.3 * i);

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
			if (cnt == 0) OffTime = osGetTime() * 0.001;
			++cnt;
			NowTime = osGetTime() * 0.001 - OffTime;

			//下画面に描画（必要に応じて描画）
			C2D_TargetClear(bot, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
			C3D_FrameDrawOn(bot);
			C2D_SceneTarget(bot);
			if (key & KEY_START) isExit = true;

			for (int i = 0; i < NOTES_MEASURE_MAX; ++i) {

				if (Notes[i].flag) {

					if (fabs(Notes[i].judge_time - NowTime) < DEFAULT_JUDGE_RANGE_BAD && touchid == Notes[i].num) Notes[i].flag = false;
					if (Notes[i].flag) {
						Notes[i].y = 200 - (Notes[i].judge_time - NowTime) * NotesSpeed;
						C2D_DrawRectSolid(80 * Notes[i].num,Notes[i].y,0,80,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
					}
				}
			}

			if (tp.px != 0 && tp.py != 0 && touchid == -1) {
				touchid = (int)tp.px / 80;
				play_sound(0);
			}
			else if (tp.px == 0 && tp.py == 0 && touchid != -1) touchid = -1;
			C2D_DrawRectSolid(0,JUDGE_Y,0,BOTTOM_WIDTH,1,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
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
	C2D_DrawText(&dynText, C2D_WithColor, x, y, 0.5f, 0.5f, 0.5f, C2D_Color32f(r, g, b, 1.0f));
}
