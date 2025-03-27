#ifndef SCREEN_STREAMER_H
#define SCREEN_STREAMER_H

#include <memory>
#include <QPixmap>
#include <QLabel>
#include <QImage>
#include <QTimer>
#include <rtc/rtc.hpp>

class ScreenStreamer : public QObject {
    Q_OBJECT

public:
    explicit ScreenStreamer(QLabel *label, std::function<void( const QByteArray & )> send_callback,
                             QObject *parent = nullptr);

    // Метод для обновления кадра
    void update_frame(const QByteArray &image_data);

    //потом доделаю нужны для того чтобы включать и выключать транслляцию экрана
    void start();
    void stop();

    void set_label(QLabel *label){this->label_ = label;}

private:

    // QLabel для отображения
    QLabel *label_;

    // Таймер для обновления
    QTimer *timer_;

    // отправка записанного экрана собеседнику
    // будет использоваться метод из p2p_conneciton send_data_p2p(QByteArray data)
    std::function<void( const QByteArray & )> send_callback_;

    //обновление, запись, сжатие кадра
    void capture_screen();

    // тут обрабатываются данные и потом через send_callback_ данные отправляются
    void send_image(const QPixmap &pixmap);

};
#endif // SCREEN_STREAMER_H

