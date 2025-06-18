#include "dios_ssp_api.h"
#include "sndfile.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: wavio in.wav\n");
        return 0;
    }

    // Generate output filenames
    char out_filename[256];
    char vad_filename[256];
    char base_name[256];
    strncpy(base_name, argv[1], sizeof(base_name) - 1);
    base_name[sizeof(base_name) - 1] = '\0';
    
    char *dot = strrchr(base_name, '.');
    if (dot != NULL && strcmp(dot, ".wav") == 0) {
        *dot = '\0';  // Remove .wav extension
    }
    
    // Use length specifier to prevent truncation warning
    // Reserve space for suffix + null terminator
    snprintf(out_filename, sizeof(out_filename), "%.*s_dtln_ns_agc.wav", 
             (int)(sizeof(out_filename) - 18), base_name);
    snprintf(vad_filename, sizeof(vad_filename), "%.*s_vad.wav", 
             (int)(sizeof(vad_filename) - 9), base_name);

    SF_INFO info;
    SNDFILE *inwav = sf_open(argv[1], SFM_READ, &info);
    if (NULL == inwav) {
        fprintf(stderr, "open %s failed\n", argv[1]);
        return -1;
    }

    SF_INFO onfo;
    onfo.channels = 1;
    onfo.samplerate = info.samplerate;
    onfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    onfo.sections = 0;
    onfo.seekable = 1;
    SNDFILE *outwav = sf_open(out_filename, SFM_WRITE, &onfo);
    if (NULL == outwav) {
        fprintf(stderr, "open %s failed\n", out_filename);
        sf_close(inwav);
        return -2;
    }

    SF_INFO vnfo;
    vnfo.channels = 1;
    vnfo.samplerate = info.samplerate;
    vnfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    vnfo.sections = 0;
    vnfo.seekable = 1;
    SNDFILE *vadwav = sf_open(vad_filename, SFM_WRITE, &vnfo);
    if (NULL == vadwav) {
        fprintf(stderr, "open %s failed\n", vad_filename);
        sf_close(outwav);
        sf_close(inwav);
        return -3;
    }

    objSSP_Param param;
    param.AEC_KEY = 0;
    param.DTLN_KEY = 1;
    param.NS_KEY  = 1;
    param.AGC_KEY = 1;
    param.HPF_KEY = 0;
    param.BF_KEY  = 0;
    param.DOA_KEY = 0;
    param.mic_num = 1;
    param.ref_num = 0;
    param.loc_phi = 90.0f;
    param.modelpath[0] = "./model/model_1.tflite";
    param.modelpath[1] = "./model/model_2.tflite";
    memset(param.mic_coord, 0, sizeof(param.mic_coord));

    void *hssp = dios_ssp_init_api(&param);
    if (NULL == hssp) {
        fprintf(stderr, "dios_ssp_init_api failed\n");
        return -4;
    }

    int ret = dios_ssp_reset_api(hssp, &param);
    if (ret) {
        fprintf(stderr, "dios_ssp_reset_api failed, return %d\n", ret);
        return -5;
    }

    int framelen = 128;
    while (1) {
        short micbuf[framelen];
        short sspbuf[framelen];
        int vadrst = 0;

        int readsize = sf_readf_short(inwav, micbuf, framelen);
        if (readsize != framelen) {
            fprintf(stderr, "processing all data, exiting...\n");
            break;
        }

        ret = dios_ssp_process_api(hssp, micbuf, micbuf, sspbuf, &vadrst, &param);
	if (ret) {
            fprintf(stderr, "dios_ssp_process_api failed, return %d\n", ret);
            break;
        }

        sf_writef_short(outwav, sspbuf, framelen);

        if (vadrst) {
            memset(sspbuf, 0x66, sizeof(sspbuf));
        } else {
            memset(sspbuf, 0x0, sizeof(sspbuf));
        }
        sf_writef_short(vadwav, sspbuf, framelen);
    }

    ret = dios_ssp_uninit_api(hssp, &param);
    if (OK_AUDIO_PROCESS != ret) {
        fprintf(stderr, "dios_ssp_uninit_api failed\n");
        return -1;
    }

    sf_close(vadwav);
    sf_close(outwav);
    sf_close(inwav);

    return 0;
}
