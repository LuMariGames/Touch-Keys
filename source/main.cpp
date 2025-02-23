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
int scene = 0,cnt = 0,NotesSpeed = 100;
float BPM = 120,OffTime = 0,NowTime = 0;

static C2D_SpriteSheet spriteSheet;
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

	while (aptMainLoop()) {

		hidScanInput();
		hidTouchRead(&tp);
		unsigned int key = hidKeysDown();
		if (isExit == true) break;

		int touchid = -1;

		// 描画開始
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		// 上画面に描画
		C2D_TargetClear(top, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
		C3D_FrameDrawOn(top);
		C2D_SceneTarget(top);

		switch (scene) {

		case 0:	//テスト用画面

			//差を使って時間を測る
			if (cnt == 0) OffTime = osGetTime();
			++cnt;
			NowTime = osGetTime() - OffTime;

			// 下画面に描画（必要に応じて描画）
			C2D_TargetClear(bot, C2D_Color32(0x42, 0x42, 0x42, 0xFF));
			C3D_FrameDrawOn(bot);
			C2D_SceneTarget(bot);

			for (int i = 0; i < NOTES_MAX; ++i)

				if (Notes[i].flag) {

					Notes[i].y = JUDGE_Y - (10 - NowTime) * NotesSpeed;
					C2D_DrawRectSolid(0,Notes[i].y,0,80,4,C2D_Color32(0x14, 0x91, 0xFF, 0xFF));
				}
			}

			C2D_DrawRectSolid(0,JUDGE_Y,0,BOTTOM_WIDTH,1,C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
			break;
		}

		// 描画終了
		C3D_FrameEnd(0);
	}

	// リソースの解放
	C2D_TextBufDelete(g_dynamicBuf);

	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}

char *get_buffer() {
	return buffer;
}

void notesdraw() {
	
}
