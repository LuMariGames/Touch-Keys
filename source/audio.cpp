#include <tremor/ivorbisfile.h>
#include <ogg/ogg.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio.h"
#include "header.h"

#define AUDIO_BUFFER_SIZE 2048
#define STACKSIZE (2 * 1024)
#define SOUND_NUMBER 1

typedef struct {
	float rate;
	u32 channels;
	u32 encoding;
	u32 nsamples;
	u32 size;
	char* data;
	bool loop;
	int audiochannel;
	float mix[12];
	ndspInterpType interp;
	OggVorbis_File ovf;
} Sound;
Sound sound[SOUND_NUMBER];
ndspWaveBuf waveBuf[SOUND_NUMBER];

void load_sound() {

	ndspInit();
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspSetOutputCount(1);
	char sound_address[SOUND_NUMBER][32] = {
		"romfs:/sound/tap.ogg",
	};

	for (int i = 0; i < SOUND_NUMBER; ++i) {
		memset(&sound[i], 0, sizeof(sound[i]));
		sound[i].mix[0] = 1.0f;
		sound[i].mix[1] = 1.0f;
		FILE * file = fopen(sound_address[i], "rb");
		if (file == 0) {
			printf("no file\n");
			while (1);
		}
		if (ov_open(file, &sound[i].ovf, NULL, 0) < 0) {
			printf("ogg vorbis file error\n");
			while (1);
		}
		vorbis_info * vorbisInfo = ov_info(&sound[i].ovf, -1);
		if (vorbisInfo == NULL) {
			printf("could not retrieve ogg audio stream information\n");
			while (1);
		}
		sound[i].rate = (float)vorbisInfo->rate;
		sound[i].channels = (u32)vorbisInfo->channels;
		sound[i].encoding = NDSP_ENCODING_PCM16;
		sound[i].nsamples = (u32)ov_pcm_total(&sound[i].ovf, -1);
		sound[i].size = sound[i].nsamples * sound[i].channels * 2;
		sound[i].audiochannel = i;
		sound[i].interp = NDSP_INTERP_NONE;
		sound[i].loop = false;
		if (linearSpaceFree() < sound[i].size) {
			printf("not enough linear memory available %ld\n", sound[i].size);
		}
		sound[i].data = (char*)linearAlloc(sound[i].size);
		if (sound[i].data == 0) {
			printf("null\n");
			while (1);
		}
		int offset = 0;
		int eof = 0;
		int currentSection;
		while (!eof) {
			long ret = ov_read(&sound[i].ovf, &sound[i].data[offset], AUDIO_BUFFER_SIZE, &currentSection);
			if (ret == 0) {
				eof = 1;
			}
			else if (ret < 0) {
				ov_clear(&sound[i].ovf);
				linearFree(sound[i].data);
				printf("error in the ogg vorbis stream\n");
				while (1);
			}
			else {
				offset += ret;
			}
			//printf("%ld %d\n", ret, currentSection);
		}
		memset(&waveBuf[i], 0, sizeof(ndspWaveBuf));
		waveBuf[i].data_vaddr = sound[i].data;
		waveBuf[i].nsamples = sound[i].nsamples;
		waveBuf[i].looping = sound[i].loop;
		waveBuf[i].status = NDSP_WBUF_FREE;
		DSP_FlushDataCache(sound[i].data, sound[i].size);
		//linearFree(&sound[i].ovf);
		ov_clear(&sound[i].ovf);
		fclose(file);
		ndspChnReset(sound[i].audiochannel);
		ndspChnInitParams(sound[i].audiochannel);
		ndspChnSetMix(sound[i].audiochannel, sound[i].mix);
		ndspChnSetInterp(sound[i].audiochannel, sound[i].interp);
		ndspChnSetRate(sound[i].audiochannel, sound[i].rate);
		ndspChnSetFormat(sound[i].audiochannel, NDSP_CHANNELS(sound[i].channels) | NDSP_ENCODING(sound[i].encoding));
	}
}

int play_sound(int id) {

	if (sound[id].audiochannel == -1) {
		printf("No available audio channel\n");
		return -1;
	}
	ndspChnWaveBufClear(sound[id].audiochannel);
	ndspChnWaveBufAdd(sound[id].audiochannel, &waveBuf[id]);

	return 0;
}

void exit_music() {

	ndspChnWaveBufClear(sound[0].audiochannel);
	linearFree(sound[0].data);
	/*ndspChnWaveBufClear(sound[1].audiochannel);
	linearFree(sound[1].data);*/

	ndspExit();
}
