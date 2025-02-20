#include <citro2d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SDカードからテクスチャを読み込む
const char* texturePath = "sdmc:/3ds/touch/image.t3x";

C2D_SpriteSheet spriteSheet;

int main() {
    // 初期化
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // 描画バッファ
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    //spriteSheet = C2D_SpriteSheetLoad(texturePath);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;  // STARTボタンで終了

        // 描画開始
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // 上画面に描画
        C2D_TargetClear(top, C2D_Color32(0, 0, 0, 1));
        C2D_SceneBegin(top);

	

        // 下画面に描画（必要に応じて描画）
        C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 1));
        C2D_SceneBegin(bot);

        // 描画終了
        C3D_FrameEnd(0);
    }

    // リソースの解放
    C2D_Fini();
    C2D_SpriteSheetFree(spriteSheet);
    gfxExit();
    return 0;
}
