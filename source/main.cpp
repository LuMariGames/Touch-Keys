#include <citro2d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SD�J�[�h����e�N�X�`����ǂݍ���
const char* texturePath = "sdmc:/3ds/myapp/image.t3x";

C2D_SpriteSheet spriteSheet;

int main() {
    // ������
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // �`��o�b�t�@
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    spriteSheet = C2D_SpriteSheetLoad(texturePath);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;  // START�{�^���ŏI��

        // �`��J�n
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // ���ʂɕ`��
        C2D_TargetClear(top, C2D_Color32(0, 0, 0, 1));
        C2D_SceneBegin(top);
        // �X�e�[�W
        C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, 0),
            (float)(0),
            (float)(0),
            0.5f, NULL, 1.0f, 1.0f);

        // ����ʂɕ`��i�K�v�ɉ����ĕ`��j
        C2D_TargetClear(bot, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(bot);

        // �`��I��
        C3D_FrameEnd(0);
    }

    // ���\�[�X�̉��
    C2D_Fini();
    C2D_SpriteSheetFree(spriteSheet);
    gfxExit();
    return 0;
}
