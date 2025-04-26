#pragma once

#include <portaudio.h>
#include "opus.h"
#include <boost/signals2.hpp>

#include <vector>
#include <queue>

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QWidget>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QQueue>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <stdio.h>
}
#include <iostream>

#include <QObject>

//TODO: очень плохо реализован класс, необходимо сделать его более гибким, также решить проблему с деструктором и уделить отдельное внимание проблеме памяти

//TODO: придумать методы которые будут правильно настраивать кодеки и декодеры

class Video_camera : public QObject {
    Q_OBJECT

public:
    //Video_camera();
    explicit Video_camera( QObject *parent = nullptr);
    ~Video_camera();
    boost::signals2::signal<void(std::queue<std::shared_ptr<std::vector<uint8_t>>>&)> signalVideoCaptured;

    std::queue<std::shared_ptr<std::vector<uint8_t>>>in_data;
    std::queue<std::shared_ptr<std::vector<float>>>out_data;

    void init_device();
    void init_decoder();
    void decode_frame(AVPacket* packet);
    AVPacket* encode_frame(AVFrame* srcFrame);
    void capture_frame();
    void set_lables(QLabel *label, QLabel *label_2);
    //выключает трансляцию
    void stop();

    //выводит транслируется ли камера
    bool is_shared();

    QImage avFrameToQImage(AVFrame* frame, AVPixelFormat pix_fmt) {
        SwsContext* swsCtx = sws_getContext(frame->width, frame->height, pix_fmt,
                                            frame->width, frame->height, AV_PIX_FMT_RGB24,
                                            SWS_BILINEAR, nullptr, nullptr, nullptr);

        QImage img(frame->width, frame->height, QImage::Format_RGB888);
        uint8_t* dest[4] = { img.bits(), nullptr, nullptr, nullptr };
        int dest_linesize[4] = { static_cast<int>(img.bytesPerLine()), 0, 0, 0 };

        sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dest, dest_linesize);
        sws_freeContext(swsCtx);

        return img;
    }

    void handle_received_packet(const std::shared_ptr<std::vector<uint8_t>>& buffer) {
        AVPacket* packet = av_packet_alloc();
        av_new_packet(packet, buffer->size());
        memcpy(packet->data, buffer->data(), buffer->size());

        decode_frame(packet);

        //this->labelDecoded(packet);

        av_packet_free(&packet);
    }

    //включает трансляцию
public slots:
    void start();

signals:
    void startTimerSignal(); // Сигнал для запуска таймера в основном потоке

public:
    AVFormatContext* fmtCtx = nullptr;
    const AVInputFormat* input_format = nullptr;//av_find_input_format("dshow");
    AVDeviceInfoList* device_list = nullptr;
    int videoStreamIndex;
    const AVCodec* decoder = nullptr;
    AVCodecContext* decoderCtx = nullptr;//avcodec_alloc_context3(decoder);
    const AVCodec* encoderCodec = nullptr;//avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext* encoderCtx = nullptr;//avcodec_alloc_context3(encoderCodec);

    const AVCodec* h264Decoder = nullptr;//avcodec_find_decoder(AV_CODEC_ID_H264);
    AVCodecContext* h264DecoderCtx = nullptr;//avcodec_alloc_context3(h264Decoder);
    //avcodec_open2(h264DecoderCtx, h264Decoder, nullptr);

    AVPacket* packet = nullptr;//av_packet_alloc();
    AVFrame* frame = nullptr;//av_frame_alloc();
    AVFrame* yuvFrame = nullptr;//av_frame_alloc();
    AVFrame* decodedFrame = nullptr;//av_frame_alloc();
    struct SwsContext* swsCtx;

    QLabel* labelDecoded;
    QLabel *label_original;


private:
    // QLabel для отображения
    QLabel *label_;

    // Таймер для обновления
    QTimer *timer_;

    bool flag_is_Shared_{false};


};
