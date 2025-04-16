#pragma once

// #include <iostream>
#include <portaudio.h>
#include "opus.h"
#include <boost/signals2.hpp>

#include <vector>
#include <queue>

class Audio
{
public:
    Audio();
    boost::signals2::signal<void(std::queue<std::shared_ptr<std::vector<unsigned char>>>&)> signalAudioCaptured;

    void initialization_device();

    bool is_shared();
    bool is_open_in();

    void open_in_stream();
    void open_out_stream();

    void stop_in_stream();
    void stop_out_stream();

    void start_in_stream();
    void start_out_stream();

    void close_in_stream();
    void close_out_stream();

    void close();

    std::queue<std::shared_ptr<std::vector<unsigned char>>>in_data;
    std::queue<std::shared_ptr<std::vector<float>>>out_data;
    void decoded_voice(std::shared_ptr<std::vector<unsigned char>> ar);


private:
    static void check_err(PaError err);

    static int record_audio(
        const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
        const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
        void* user_data);

    static int play_callback(const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags, void *userData);




    static std::shared_ptr<std::vector<unsigned char>> encoded_voice(const float* data,
                                                                     size_t size_data,
                                                                     opus_int32& size_en_data,
                                                                     OpusEncoder* enc);


private:

    static std::mutex queue_mutex_;
    PaError err;

    bool flag_is_shared_{false};
    bool flag_is_open_in_{false};

    int number_input_device = 0;
    int number_output_device = 0;

    PaStream* in_stream;
    PaStream* out_stream;

    OpusEncoder *encoder;
    OpusDecoder *decoder;

    PaStreamParameters input_param;
    PaStreamParameters output_param;
};


