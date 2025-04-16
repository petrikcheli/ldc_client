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
    explicit ScreenStreamer(std::function<void( const QByteArray & )> send_callback,
                             QObject *parent = nullptr);

    // Метод для обновления кадра
    void update_frame(const QByteArray &image_data);

    //включает трансляцию
    void start();

    //выключает трансляцию
    void stop();

    //выводит транслируется ли экран
    bool is_shared();

    //установка label для трансляции
    void set_labels(QLabel *label_self, QLabel *label_peer);

signals:
    void stop_share_signal();

private:

    // QLabel для отображения
    QLabel *label_self_;

    QLabel *label_peer_;

    // Таймер для обновления
    QTimer *timer_;

    // отправка записанного экрана собеседнику
    // будет использоваться метод из p2p_conneciton send_data_p2p(QByteArray data)
    std::function<void( const QByteArray & )> send_callback_;

    //обновление, запись, сжатие кадра
    void capture_screen();

    // тут обрабатываются данные и потом через send_callback_ данные отправляются
    void send_image(const QPixmap &pixmap);

    //flag который говорит о том, запущена ли трансляция экрана
    bool flag_is_Shared_{false};

};
#endif // SCREEN_STREAMER_H

