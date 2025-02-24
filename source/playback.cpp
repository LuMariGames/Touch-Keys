#include <cstdio>

#include "header.h"
#include "vorbis.h"
#include "main.h"

#define delete(ptr) \
	free((void*) ptr); ptr = NULL

static volatile bool stop = true;
extern float mix[12];

bool togglePlayback(void){

	bool paused = ndspChnIsPaused(CHANNEL);
	ndspChnSetPaused(CHANNEL, !paused);
	return !paused;
}

void stopPlayback(void){

	stop = true;
}

bool isPlaying(void){

	return !stop;
}

int testtest = 0;

void playFile(void* infoIn){

	struct decoder_fn decoder;
	struct playbackInfo_t* info = (playbackInfo_t*)infoIn;
	int16_t*	buffer1 = NULL;
	int16_t*	buffer2 = NULL;
	int16_t*	buffer3 = NULL;
	int16_t*	buffer4 = NULL;
	int16_t*	buffer5 = NULL;
	int16_t*	buffer6 = NULL;
	ndspWaveBuf	waveBuf[6];
	bool		lastbuf = false, isNdspInit = false;
	int		ret = -1;
	const char*	file = info->file;

	/* Reset previous stop command */
	stop = false;

	setVorbis(&decoder);

	if(ndspInit() < 0)
	{
		goto err;
	}

	isNdspInit = true;

	if((ret = (*decoder.init)(file)) != 0)
	{
		goto err;
	}

	if((*decoder.channels)() > 2 || (*decoder.channels)() < 1)
	{
		goto err;
	}
	testtest = 99;
	buffer1 = (int16_t*)linearAlloc(decoder.vorbis_buffer_size * sizeof(int16_t));
	buffer2 = (int16_t*)linearAlloc(decoder.vorbis_buffer_size * sizeof(int16_t));
	buffer3 = (int16_t*)linearAlloc(decoder.vorbis_buffer_size * sizeof(int16_t));
	buffer4 = (int16_t*)linearAlloc(decoder.vorbis_buffer_size * sizeof(int16_t));
	buffer5 = (int16_t*)linearAlloc(decoder.vorbis_buffer_size * sizeof(int16_t));
	buffer6 = (int16_t*)linearAlloc(decoder.vorbis_buffer_size * sizeof(int16_t));

	ndspChnReset(CHANNEL);
	ndspChnWaveBufClear(CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(CHANNEL, NDSP_INTERP_LINEAR);
	ndspChnSetRate(CHANNEL, (*decoder.rate)());
	ndspChnSetFormat(CHANNEL, (*decoder.channels)() == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
	ndspChnSetMix(CHANNEL, mix);

	memset(waveBuf, 0, sizeof(waveBuf));

	while (*info->isPlay == false) svcSleepThread(100000);

	waveBuf[0].nsamples = (*decoder.decode)(&buffer1[0]) / (*decoder.channels)();
	waveBuf[0].data_vaddr = &buffer1[0];
	ndspChnWaveBufAdd(CHANNEL, &waveBuf[0]);
	waveBuf[1].nsamples = (*decoder.decode)(&buffer2[0]) / (*decoder.channels)();
	waveBuf[1].data_vaddr = &buffer2[0];
	ndspChnWaveBufAdd(CHANNEL, &waveBuf[1]);	
	waveBuf[2].nsamples = (*decoder.decode)(&buffer3[0]) / (*decoder.channels)();
	waveBuf[2].data_vaddr = &buffer3[0];
	ndspChnWaveBufAdd(CHANNEL, &waveBuf[2]);
	waveBuf[3].nsamples = (*decoder.decode)(&buffer4[0]) / (*decoder.channels)();
	waveBuf[3].data_vaddr = &buffer4[0];
	ndspChnWaveBufAdd(CHANNEL, &waveBuf[3]);
	waveBuf[4].nsamples = (*decoder.decode)(&buffer5[0]) / (*decoder.channels)();
	waveBuf[4].data_vaddr = &buffer5[0];
	ndspChnWaveBufAdd(CHANNEL, &waveBuf[4]);
	waveBuf[5].nsamples = (*decoder.decode)(&buffer6[0]) / (*decoder.channels)();
	waveBuf[5].data_vaddr = &buffer6[0];
	ndspChnWaveBufAdd(CHANNEL, &waveBuf[5]);	
	
	while(ndspChnIsPlaying(CHANNEL) == false);

	while(stop == false){
		//音切れチェックの間隔(us, この場合100ms毎に確認する)
		svcSleepThread(100000);

		if(lastbuf == true && waveBuf[0].status == NDSP_WBUF_DONE &&
			waveBuf[1].status == NDSP_WBUF_DONE &&
			waveBuf[2].status == NDSP_WBUF_DONE &&
			waveBuf[3].status == NDSP_WBUF_DONE &&
			waveBuf[4].status == NDSP_WBUF_DONE &&
			waveBuf[5].status == NDSP_WBUF_DONE)
			break;

		if(ndspChnIsPaused(CHANNEL) == true || lastbuf == true)
			continue;

		//音声処理
		if(waveBuf[0].status == NDSP_WBUF_DONE) {
			size_t read = (*decoder.decode)(&buffer1[0]);
			if(read <= 0) {
				lastbuf = true;
				continue;
			}
			else if(read < decoder.vorbis_buffer_size) waveBuf[0].nsamples = read / (*decoder.channels)();
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[0]);
		}
		if(waveBuf[1].status == NDSP_WBUF_DONE) {
			size_t read = (*decoder.decode)(&buffer2[0]);
			if(read <= 0) {
				lastbuf = true;
				continue;
			}
			else if(read < decoder.vorbis_buffer_size) waveBuf[1].nsamples = read / (*decoder.channels)();
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[1]);
		}
		if(waveBuf[2].status == NDSP_WBUF_DONE) {
			size_t read = (*decoder.decode)(&buffer3[0]);
			if(read <= 0) {
				lastbuf = true;
				continue;
			}
			else if(read < decoder.vorbis_buffer_size) waveBuf[2].nsamples = read / (*decoder.channels)();
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[2]);
		}
		if(waveBuf[3].status == NDSP_WBUF_DONE) {
			size_t read = (*decoder.decode)(&buffer4[0]);
			if(read <= 0) {
				lastbuf = true;
				continue;
			}
			else if(read < decoder.vorbis_buffer_size) waveBuf[3].nsamples = read / (*decoder.channels)();
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[3]);
		}
		if(waveBuf[4].status == NDSP_WBUF_DONE) {
			size_t read = (*decoder.decode)(&buffer5[0]);
			if(read <= 0) {
				lastbuf = true;
				continue;
			}
			else if(read < decoder.vorbis_buffer_size) waveBuf[4].nsamples = read / (*decoder.channels)();
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[4]);
		}
		if(waveBuf[5].status == NDSP_WBUF_DONE) {
			size_t read = (*decoder.decode)(&buffer6[0]);
			if(read <= 0) {
				lastbuf = true;
				continue;
			}
			else if(read < decoder.vorbis_buffer_size) waveBuf[5].nsamples = read / (*decoder.channels)();
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[5]);
		}
	}

	(*decoder.exit)();
out:
	if(isNdspInit == true)
	{
		ndspChnWaveBufClear(CHANNEL);
		ndspExit();
	}

	delete(info->file);
	linearFree(buffer1);
	linearFree(buffer2);
	linearFree(buffer3);
	linearFree(buffer4);
	linearFree(buffer5);
	linearFree(buffer6);

	threadExit(0);
	return;

err:
	goto out;
}

struct playbackInfo_t playbackInfo;

inline int changeFile(const char* ep_file, struct playbackInfo_t* playbackInfo, bool *p_isPlayMain){

	s32 prio;
	static Thread thread = NULL;

	if (ep_file != NULL) return -1;
	
	if (thread != NULL) {
		stopPlayback();

		threadJoin(thread, U64_MAX);
		threadFree(thread);
		thread = NULL;
	}

	if (ep_file == NULL || playbackInfo == NULL) return 0;

	playbackInfo->file = strdup(ep_file);
	playbackInfo->isPlay = p_isPlayMain;

	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	thread = threadCreate(playFile, playbackInfo, 32000, prio - 1, -2, false);
	
	return 0;
}

void play_main_music(bool *p_isPlayMain) {

	chdir(DEFAULT_DIR);
	changeFile("audio.ogg", &playbackInfo, p_isPlayMain);
}

void pasue_main_music() {

	if (isPlaying() == true) {
		togglePlayback();
	}
}

void stop_main_music() {

	stopPlayback();
	changeFile(NULL, &playbackInfo ,NULL);
}

void init_main_music() {

	playbackInfo.file = NULL;
}
