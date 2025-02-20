#include <citro2d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SDカードからテクスチャを読み込む
const char* texturePath = "sdmc:/3ds/touch/image.t3x";
int scene = 0;

C2D_SpriteSheet spriteSheet;

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

		case 0:	//ロード画面

			snprintf(get_buffer(), BUFFER_SIZE, "Touch Keys v%s", VERSION);
			draw_select_text(120, 70, get_buffer());
			draw_select_text(120, 100, "Now Loading...");
			C3D_FrameEnd(0);
			load_file_main();

		// 下画面に描画（必要に応じて描画）
		C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 1));
		C3D_FrameDrawOn(bot);
		C2D_SceneTarget(bot);

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
