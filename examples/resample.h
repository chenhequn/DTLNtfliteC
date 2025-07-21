// 重采样函数声明
int resample_to_16k(const short *input_buf,    // 输入音频数据
                    short *output_buf,          // 输出音频数据
                    int frames,                 // 输入帧数
                    int in_channels,            // 通道数
                    int sample_rate,            // 输入采样率
                    int *output_frames);        // 输出帧数