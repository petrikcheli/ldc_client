#include "video_camera.h"
#include <thread>

Video_camera::Video_camera(QObject *parent) : timer_(new QTimer(this))
{


    // this->timer = new QTimer(this);
    // connect(timer, &QTimer::timeout, this, &Video::share_start);
    init_device();
}



Video_camera::~Video_camera()
{
    // Clean up
    sws_freeContext(swsCtx);
    av_frame_free(&frame);
    av_frame_free(&yuvFrame);
    av_frame_free(&decodedFrame);
    av_packet_free(&packet);
    avcodec_free_context(&decoderCtx);
    avcodec_free_context(&encoderCtx);
    avcodec_free_context(&h264DecoderCtx);
    avformat_close_input(&fmtCtx);

    // Освобождаем память, выделенную для списка устройств
    avdevice_free_list_devices(&device_list);
}

void Video_camera::init_device()
{
    avdevice_register_all();

    // === Инициализация захвата с камеры ===
    fmtCtx = nullptr;

    input_format = av_find_input_format("dshow");
    if (!input_format) {
        std::cerr << "Failed found input format" << std::endl;
        return;
    }

    device_list = nullptr;
    int device_count = avdevice_list_input_sources(input_format, nullptr, nullptr, &device_list);
    if (device_count < 0) {
        std::cerr << "Failed get list device" << std::endl;
        return;
    }

    std::cout << "Found " << device_count << "device" << std::endl;
    for (int i = 0; i < device_count; ++i) {
        std::cout << "Device " << i + 1 << " " << device_list->devices[i]->device_name << std::endl;
        std::cout << "Description " << device_list->devices[i]->device_description << std::endl;
    }

    // std::cout << "\nChoice Device :";
    // int i_device = 0;
    // std::cin >> i_device;
    // char *str_device = device_list->devices[i_device - 1]->device_description;
    // char buffer[256]; // выделяем буфер подходящего размера

    // snprintf(buffer, sizeof(buffer), "%s%s", "video=", str_device);

    // std::cout << std::endl <<  buffer << std::endl;


}

void Video_camera::init_decoder()
{
    // === Декодер для проверки после кодирования ===
    h264Decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    h264DecoderCtx = avcodec_alloc_context3(h264Decoder);
    avcodec_open2(h264DecoderCtx, h264Decoder, nullptr);
}

void Video_camera::decode_frame(AVPacket* packet)
{
    if (avcodec_send_packet(h264DecoderCtx, packet) >= 0) {
        AVFrame* decodedFrame = av_frame_alloc();
        while (avcodec_receive_frame(h264DecoderCtx, decodedFrame) >= 0) {
            QImage imgDecoded = avFrameToQImage(decodedFrame, h264DecoderCtx->pix_fmt);
            labelDecoded->setPixmap(QPixmap::fromImage(imgDecoded).scaled(labelDecoded->size(), Qt::KeepAspectRatio));
        }
        av_frame_free(&decodedFrame);
    }
}

AVPacket* Video_camera::encode_frame(AVFrame* srcFrame)
{
    static int64_t pts = 0;

    // Конвертируем в формат для кодировщика
    sws_scale(swsCtx, srcFrame->data, srcFrame->linesize, 0, srcFrame->height, yuvFrame->data, yuvFrame->linesize);
    yuvFrame->pts = pts++;

    // Посылаем кадр в кодировщик
    if (avcodec_send_frame(encoderCtx, yuvFrame) >= 0) {
        AVPacket* encPkt = av_packet_alloc();
        if (avcodec_receive_packet(encoderCtx, encPkt) >= 0) {
            return encPkt;
        }
        av_packet_free(&encPkt);
    }

    return nullptr;
}

void Video_camera::capture_frame()
{
    std::thread captured_frame([this]{
        while(flag_is_Shared_){
            if (av_read_frame(fmtCtx, packet) >= 0) {
                if (packet->stream_index == videoStreamIndex) {
                    if (avcodec_send_packet(decoderCtx, packet) >= 0) {

                        while (avcodec_receive_frame(decoderCtx, frame) >= 0) {

                            // Кодируем кадр
                            AVPacket* encPkt = encode_frame(frame);
                            if (!encPkt || encPkt->size <= 0) continue;

                            // Для отладки можешь посмотреть размер пакета:
                            qDebug() << "Encoded packet size:" << encPkt->size;

                            // Отрисовываем оригинальный кадр
                            QImage imgOriginal = avFrameToQImage(frame, decoderCtx->pix_fmt);
                            label_original->setPixmap(QPixmap::fromImage(imgOriginal).scaled(label_original->size(), Qt::KeepAspectRatio));

                            // Упаковываем пакет в shared_ptr
                            auto array_encPkt = std::make_shared<std::vector<uint8_t>>(encPkt->data, encPkt->data + encPkt->size);

                            // Пушим в очередь
                            in_data.push(array_encPkt);

                            // Сигналим, что новый кадр готов
                            signalVideoCaptured(in_data);
                            // Чистим пакет
                            av_packet_unref(encPkt);
                            av_packet_free(&encPkt);
                        }
                    }
                }
                //av_packet_unref(yuvFrame);

                av_packet_unref(packet);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    captured_frame.detach();
}

void Video_camera::set_lables(QLabel *label, QLabel *label_2)
{
    this->labelDecoded = label;
    this->label_original = label_2;
}

void Video_camera::start()
{
    if (avformat_open_input(&fmtCtx, "video=HD Camera", input_format, nullptr) != 0) {
        qDebug() << "Cannot open camera";
        return;
    }
    //avformat_close_input()
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        qDebug() << "Cannot find stream info";
        return;
    }

    decoder = nullptr;
    videoStreamIndex = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);

    decoderCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decoderCtx, fmtCtx->streams[videoStreamIndex]->codecpar);
    avcodec_open2(decoderCtx, decoder, nullptr);

    // === Инициализация кодека H.264 ===
    encoderCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    encoderCtx = avcodec_alloc_context3(encoderCodec);
    encoderCtx->bit_rate = 2000000;
    encoderCtx->width = decoderCtx->width;
    encoderCtx->height = decoderCtx->height;
    encoderCtx->time_base = {1, 20};
    encoderCtx->framerate = {20, 1};
    encoderCtx->gop_size = 1;
    encoderCtx->max_b_frames = 0;
    encoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    //av_opt_set(encoderCtx->priv_data, "preset", "ultrafast", 0);
    //av_opt_set(encoderCtx->priv_data, "tune", "zerolatency", 0);
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "preset", "ultrafast", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);  // для реального времени
    av_dict_set(&opts, "annexb", "1", 0);           // вот это критично!
    //av_opt_set(encoderCtx->priv_data, "preset", "slow", 0);

    if (avcodec_open2(encoderCtx, encoderCodec, &opts) < 0) {
        qDebug() << "Failed to open H.264 encoder";
        return;
    }
    av_dict_free(&opts);
    // === Декодер для проверки после кодирования ===
    h264Decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    h264DecoderCtx = avcodec_alloc_context3(h264Decoder);
    avcodec_open2(h264DecoderCtx, h264Decoder, nullptr);

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    yuvFrame = av_frame_alloc();
    decodedFrame = av_frame_alloc();

    // Настраиваем кадр для кодирования
    yuvFrame->format = encoderCtx->pix_fmt;
    yuvFrame->width = encoderCtx->width;
    yuvFrame->height = encoderCtx->height;
    av_frame_get_buffer(yuvFrame, 0);

    swsCtx = sws_getContext(decoderCtx->width, decoderCtx->height, decoderCtx->pix_fmt,
                            encoderCtx->width, encoderCtx->height, encoderCtx->pix_fmt,
                            SWS_BICUBIC, nullptr, nullptr, nullptr);
    // Таймер для периодического обновления (например, 30 FPS)
    flag_is_Shared_ = true;
    this->capture_frame();
    //connect(timer_, &QTimer::timeout, this, &Video_camera::capture_frame);
    //connect(this, &Video_camera::stop, timer_, &QTimer::stop);
    //timer_->start(50);  // ~30 FPS (1000ms / 30 ≈ 33ms)
}

void Video_camera::stop()
{
    flag_is_Shared_ = false;
    if (swsCtx) sws_freeContext(swsCtx);
    if (decoderCtx) avcodec_free_context(&decoderCtx);
    if (encoderCtx) avcodec_free_context(&encoderCtx);
    if (h264DecoderCtx) avcodec_free_context(&h264DecoderCtx);
    if (fmtCtx) avformat_close_input(&fmtCtx);

    av_frame_free(&frame);
    av_frame_free(&yuvFrame);
    av_frame_free(&decodedFrame);
    av_packet_free(&packet);
    //disconnect(timer_, &QTimer::timeout, this, &Video_camera::capture_frame);
    //emit stop_share_signal();
    //timer_->stop();
}

bool Video_camera::is_shared()
{
    return flag_is_Shared_;
}
