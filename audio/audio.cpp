#include <iostream>
#include "audio.h"
#include "Audio_parametrs.h"

std::mutex Audio::queue_mutex_;

Audio::Audio() {
    int error;

    decoder = opus_decoder_create(daupi::SAMPLE_RATE, daupi::CHANNELS, &error);

    if (error != OPUS_OK) {
        std::cerr << "failed open opus: " << opus_strerror(error) << std::endl;
    }

    encoder = opus_encoder_create(daupi::SAMPLE_RATE, daupi::CHANNELS, OPUS_APPLICATION_AUDIO, &error);

    if (error != OPUS_OK) {
        std::cerr << "failed open opus: " << opus_strerror(error) << std::endl;
    }

    initialization_device();
}

void Audio::check_err(PaError err)
{
    if(err != paNoError){
        exit(EXIT_FAILURE);
    }
}

int Audio::record_audio(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags, void *user_data)
{
    auto audio = (Audio*)user_data;
    const float* in = (const float*)input_buffer;
    auto in_vec = std::make_shared<std::vector<float>>(frames_per_buffer);
    for(int i = 0; i < frames_per_buffer; ++i) in_vec->at(i) = in[i];

    if (input_buffer != nullptr) {
        opus_int32 size_encoded_data = 0;
        auto encoded_data = encoded_voice(in,
                                          daupi::CHANNELS * daupi::FRAMES_PER_BUFFER,
                                          size_encoded_data,
                                          audio->encoder);

        encoded_data->resize(int(size_encoded_data));
            // Блокируем мьютекс перед добавлением в очередь
        std::lock_guard<std::mutex> lock(queue_mutex_);
        audio->in_data.push(encoded_data);
        std::cerr << "send signal auido captured " << std::endl;
        audio->signalAudioCaptured(audio->in_data);
    }
    return paContinue;
}

int Audio::play_callback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *user_data)
{
    auto audio = (Audio*)user_data;
    float *output = (float *)outputBuffer;
    size_t framesToCopy = framesPerBuffer;
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (audio->out_data.size() > 0) {

        std::vector<float> data_out = *audio->out_data.front();
        std::copy(data_out.data(), data_out.data()+daupi::FRAMES_PER_BUFFER, output);
        audio->out_data.pop();
        std::cout << data_out.size() << std::endl;
    } else {
        std::memset(output, 0, framesPerBuffer * sizeof(float));
    }

    return paContinue;
}

void Audio::initialization_device()
{
    err = Pa_Initialize();
    check_err(err);

    number_input_device = Pa_GetDefaultInputDevice();
    number_output_device = Pa_GetDefaultOutputDevice();

    input_param.device = number_input_device;
    input_param.channelCount = daupi::CHANNELS;
    input_param.sampleFormat = paFloat32;
    input_param.suggestedLatency = Pa_GetDeviceInfo(input_param.device)->defaultLowInputLatency;
    input_param.hostApiSpecificStreamInfo = NULL;

    output_param.device = number_output_device;
    output_param.channelCount = daupi::CHANNELS;
    output_param.sampleFormat = paFloat32;
    output_param.suggestedLatency = Pa_GetDeviceInfo(output_param.device)->defaultLowOutputLatency;
    output_param.hostApiSpecificStreamInfo = NULL;
}

void Audio::open_in_stream()
{
    err = Pa_OpenStream(
        &in_stream,
        &input_param,
        NULL,
        daupi::SAMPLE_RATE,
        daupi::FRAMES_PER_BUFFER,
        paFramesPerBufferUnspecified,
        record_audio,
        this
        //&in_data
        );
    check_err(err);

    err = Pa_StartStream(in_stream);
    check_err(err);
}

void Audio::open_out_stream()
{
    err = Pa_OpenStream(
        &out_stream,
        NULL,
        &output_param,
        daupi::SAMPLE_RATE,
        daupi::FRAMES_PER_BUFFER,
        paFramesPerBufferUnspecified,
        play_callback,
        this
        //&out_data
        );
    check_err(err);

    err = Pa_StartStream(out_stream);
    check_err(err);
}

std::shared_ptr<std::vector<unsigned char>>
Audio::encoded_voice(const float* in_data, size_t size_data, opus_int32& size_en_data, OpusEncoder* encoder)
{
    int error;

    if (error != OPUS_OK) {
        std::cerr << "failed open opus: " << opus_strerror(error) << std::endl;
        return nullptr;
    }

    int max_size_en = size_data*sizeof(float)/sizeof(char);
    auto encoded_data = std::make_shared<std::vector<unsigned char>>(max_size_en);

    size_en_data = opus_encode_float(encoder, in_data, size_data,
                                     encoded_data->data(), size_data);

    if (size_en_data < 0) {
        std::cerr << "Ошибка кодирования OPUS: " << opus_strerror(size_en_data) << std::endl;
        opus_encoder_destroy(encoder);
        return nullptr;
    }
    encoded_data->resize(size_en_data);
    return encoded_data;
}

void Audio::decoded_voice(std::shared_ptr<std::vector<unsigned char>> ar)
{
    int error = 0;

    if (error != OPUS_OK) {
        std::cerr << "Failed to create OPUS decoder: " << opus_strerror(error) << std::endl;
    }

    auto decoded_voice = std::make_shared<std::vector<float>>(daupi::FRAMES_PER_BUFFER * daupi::CHANNELS);

    int decoded_samples = opus_decode_float(this->decoder, ar->data(),
                                            ar->size(),
                                            decoded_voice->data(),
                                            daupi::FRAMES_PER_BUFFER * daupi::CHANNELS,
                                            0);

    if (decoded_samples < 0) {
        std::cerr << "error decoded OPUS: " << opus_strerror(decoded_samples) << std::endl;
    } else {

    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        this->out_data.push(decoded_voice);
    }

}
