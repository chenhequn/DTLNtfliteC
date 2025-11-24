#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include "resample.h"
#include "dios_ssp_api.h"

#define MAX_CHANNELS 8
#define FRAMES_SIZE 128

// 全局变量用于信号处理
static volatile int keep_running = 1;

// 信号处理函数
void handle_signal(int sig) {
    keep_running = 0;
}

// 示例前处理函数
void preprocess(const short* input, short* output, int num_channels, int num_frames) {
    for (int i = 0; i < num_frames * num_channels; ++i) {
        output[i] = input[i];  // 简单拷贝
    }
}

// 打开 PCM 设备
snd_pcm_t* open_pcm(const char* device_name, snd_pcm_stream_t stream, int channels, int rate) {
    snd_pcm_t* pcm_handle = NULL;
    snd_pcm_hw_params_t* hw_params = NULL;
    int err;

    // 打开PCM设备
    err = snd_pcm_open(&pcm_handle, device_name, stream, 0);
    if (err < 0) {
        fprintf(stderr, "Cannot open audio device %s: %s\n", device_name, snd_strerror(err));
        return NULL;
    }

    // 分配硬件参数结构
    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate(pcm_handle, hw_params, rate, 0);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
    snd_pcm_hw_params(pcm_handle, hw_params);
    snd_pcm_hw_params_free(hw_params);
    snd_pcm_prepare(pcm_handle);

    return pcm_handle;

error:
    fprintf(stderr, "Cannot set hardware parameters: %s\n", snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    snd_pcm_close(pcm_handle);
    return NULL;
}

objSSP_Param* init_signal_config(void) {
    objSSP_Param* SSP_PARAM = (objSSP_Param*)malloc(sizeof(objSSP_Param));
    if (!SSP_PARAM) {
        return NULL;
    }

    // 设置基本参数
    SSP_PARAM->AEC_KEY  = 0;      // 关闭AEC
    SSP_PARAM->DTLN_KEY = 1;      // 打开DTLN
    SSP_PARAM->NS_KEY   = 1;      // 开启降噪
    SSP_PARAM->AGC_KEY  = 1;      // 开启AGC
    SSP_PARAM->HPF_KEY  = 0;      // 开启高通滤波
    SSP_PARAM->BF_KEY   = 0;      // 关闭波束形成
    SSP_PARAM->DOA_KEY  = 0;      // 关闭DOA
    SSP_PARAM->mic_num  = 1;      // 通道数
    SSP_PARAM->ref_num  = 0;      // 无参考通道
    SSP_PARAM->loc_phi  = 90.0f;  // 默认方位角
    SSP_PARAM->modelpath[0] = "./model/model_1.tflite";
    SSP_PARAM->modelpath[1] = "./model/model_2.tflite";
    memset(SSP_PARAM->mic_coord, 0, sizeof(SSP_PARAM->mic_coord));

    return SSP_PARAM;
}

int main(void) {
    const char* input_dev = "default";
    //const char* input_dev = "hw:5,0";
    const int in_channels = 8;
    const int out_channels = 1;
    const int in_sample_rate = 16000;
    const int target_sample_rate = 16000;
    const int array_frm_len = 128; //前处理模块每次处理的音频长度
    const int capture_frames = (in_sample_rate / target_sample_rate) * array_frm_len;

    // 注册信号处理函数
    signal(SIGINT, handle_signal);

    // 打开PCM设备
    snd_pcm_t* input_pcm = open_pcm(input_dev, SND_PCM_STREAM_CAPTURE, in_channels, in_sample_rate);
    if (!input_pcm) {
        return -1;
    }

    // 分配输入缓冲区
    short* capture_buf = (short*)malloc(capture_frames * in_channels * sizeof(short));
    if (!capture_buf) {
        fprintf(stderr, "Failed to allocate capture buffer\n");
        snd_pcm_close(input_pcm);
        return -1;
    }

    // 生成文件名
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char captured_filename[64];
    strftime(captured_filename, sizeof(captured_filename), "captured_%Y%m%d_%H%M%S.pcm", t);

    // 打开文件
    char out_path[128];
    snprintf(out_path, sizeof(out_path), "out/%s", captured_filename);
    FILE *fp_captured = fopen(out_path, "wb");
    if (!fp_captured) {
        fprintf(stderr, "Failed to open output file: %s\n", captured_filename);
        free(capture_buf);
        snd_pcm_close(input_pcm);
        return -1;
    }

    // 根据filename，生成处理后的文件名，命名为*_processed
    char processed_filename[64];
    char *dot = strrchr(captured_filename, '.');
    if (dot) {
        size_t prefix_len = dot - captured_filename;
        strncpy(processed_filename, captured_filename, prefix_len);
        processed_filename[prefix_len] = '\0';
        strcat(processed_filename, "_processed");
        strcat(processed_filename, dot);
    } else {
        strcpy(processed_filename, captured_filename);
        strcat(processed_filename, "_processed");
    }
    
    // 打开处理后的输出文件
    snprintf(out_path, sizeof(out_path), "out/%s", processed_filename);
    FILE *fp_processed = fopen(out_path, "wb");
    if (!fp_processed) {
        fprintf(stderr, "Failed to open processed output file: %s\n", processed_filename);
        free(capture_buf);
        fclose(fp_processed);
        snd_pcm_close(input_pcm);
        return -1;
    }
    
    // 初始化前处理模块
    objSSP_Param* SSP_PARAM = init_signal_config();
    void* st;
    st = dios_ssp_init_api(SSP_PARAM);
    if (NULL == st) {
        fprintf(stderr, "dios_ssp_init_api failed\n");
        return -4;
    }

    int ret = dios_ssp_reset_api(st, SSP_PARAM);
    if (ret) {
        fprintf(stderr, "dios_ssp_reset_api failed, return %d\n", ret);
        return -5;
    }

    printf("Start processing... (Press Ctrl+C to stop)\n");

    while (keep_running) {
        // 采集音频
        int err = snd_pcm_readi(input_pcm, capture_buf, capture_frames);
        if (err < 0) {
            fprintf(stderr, "Read error: %s\n", snd_strerror(err));
            snd_pcm_prepare(input_pcm);
            continue;
        }

        // 提取第一个声道的数据
        short* first_channel_buf = (short*)malloc(capture_frames * sizeof(short));
        if (!first_channel_buf) {
            fprintf(stderr, "Failed to allocate first channel buffer\n");
            break;
        }
        for (int i = 0; i < capture_frames; ++i) {
            first_channel_buf[i] = capture_buf[i * in_channels];
        }
        size_t written = fwrite(first_channel_buf, sizeof(short), capture_frames, fp_captured);
        if (written != capture_frames) {
            fprintf(stderr, "Failed to write all samples for first channel\n");
        }
        fflush(fp_captured);

        // 分配处理后的缓冲区processed_buf
        short* processed_buf = (short*)malloc(capture_frames * out_channels * sizeof(short));
        if (!processed_buf) {
            fprintf(stderr, "Failed to allocate process buffer\n");
            free(first_channel_buf);
            free(processed_buf);
            continue;
        }

        int vadrst = 0;
        // 信号处理
        ret = dios_ssp_process_api(st, first_channel_buf, first_channel_buf, processed_buf, &vadrst, SSP_PARAM);
        if (ret != 0) {
            fprintf(stderr, "dios_ssp_process_api processing failed\n");
            free(first_channel_buf);
            free(processed_buf);
            continue;
        }

        // 写入处理后的音频数据
        written = fwrite(processed_buf, sizeof(short), 
                              capture_frames * out_channels, fp_processed);
        if (written != capture_frames * out_channels) {
            fprintf(stderr, "Failed to write all samples\n");
        }

        fflush(fp_processed);

        // 释放缓冲区
        free(first_channel_buf);
        free(processed_buf);
    }

    // 清理资源
    dios_ssp_uninit_api(st, SSP_PARAM);
    free(SSP_PARAM);
    free(capture_buf);
    fclose(fp_captured);
    fclose(fp_processed);

    snd_pcm_close(input_pcm);

    printf("\nProcessing completed.\n");
    return 0;
}
