#include <stdio.h>
#include <stdlib.h>
#include "resample.h"

int resample_to_16k(const short *input_buf, 
                    short *output_buf,
                    int frames,
                    int in_channels,
                    int sample_rate,
                    int *output_frames) 
{
    if (!input_buf || !output_buf || !output_frames) {
        return -1;
    }

    const int target_rate = 16000;
    if (sample_rate == target_rate) {
        // 采样率已是16k，直接拷贝
        for (int i = 0; i < frames * in_channels; i++) {
            output_buf[i] = input_buf[i];
        }
        *output_frames = frames;
        return 0;
    }

    *output_frames = frames * target_rate / sample_rate;

    // 对每个通道进行重采样
    for (int ch = 0; ch < in_channels; ch++) {
        // 提取单通道数据
        short *ch_in = (short *)malloc(frames * sizeof(short));
        if (!ch_in) {
            return -1;
        }

        for (int i = 0; i < frames; i++) {
            ch_in[i] = input_buf[i * in_channels + ch];
        }

        // 线性插值重采样
        short *ch_out = (short *)malloc(*output_frames * sizeof(short));
        if (!ch_out) {
            free(ch_in);
            return -1;
        }

        float scale = (float)frames / *output_frames;
        for (int i = 0; i < *output_frames; i++) {
            float pos = i * scale;
            int idx = (int)pos;
            float frac = pos - idx;
            if (idx >= frames - 1) {
                ch_out[i] = ch_in[frames - 1];
            } else {
                // 使用整数运算避免浮点数精度损失
                ch_out[i] = (short)((1.0f - frac) * ch_in[idx] + frac * ch_in[idx + 1]);
            }
        }

        // 写回重采样后的数据
        for (int i = 0; i < *output_frames; i++) {
            output_buf[i * in_channels + ch] = ch_out[i];
        }

        // 释放临时缓冲区
        free(ch_in);
        free(ch_out);
    }

    return 0;
} 