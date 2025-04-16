#include "screen_streamer.h"
#include <QApplication>
#include <QScreen>
#include <QBuffer>

//ScreenStreamer::ScreenStreamer() {}

ScreenStreamer::ScreenStreamer(std::function<void( const QByteArray & )> send_callback,
                               QObject *parent)
    : QObject(parent), timer_(new QTimer(this))
{
    this->send_callback_ = send_callback;
}

void ScreenStreamer::start(){
    // Таймер для периодического обновления (например, 30 FPS)
    flag_is_Shared_ = true;
    connect(timer_, &QTimer::timeout, this, &ScreenStreamer::capture_screen);
    connect(this, &ScreenStreamer::stop_share_signal, timer_, &QTimer::stop);
    timer_->start(33);  // ~30 FPS (1000ms / 30 ≈ 33ms)
}

void ScreenStreamer::stop()
{
    flag_is_Shared_ = false;
    disconnect(timer_, &QTimer::timeout, this, &ScreenStreamer::capture_screen);
    //emit stop_share_signal();
    timer_->stop();
}

bool ScreenStreamer::is_shared()
{
    return flag_is_Shared_;
}

void ScreenStreamer::set_labels(QLabel *label_self, QLabel *label_peer)
{
    this->label_self_ = label_self;
    this->label_peer_ = label_peer;
}

void ScreenStreamer::update_frame(const QByteArray &image_data)
{
    QImage image;
    if (image.loadFromData(image_data, "JPEG")) {
        // Преобразуем QImage → QPixmap и устанавливаем в QLabel
        auto pixmap = QPixmap::fromImage(image);
        QPixmap scaledPixmap = pixmap.scaled(label_self_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        label_peer_->setPixmap(scaledPixmap);
        label_peer_->setFixedSize(label_peer_->size());
    } else {
        qWarning("Error: failed download is data.");
    }
}

void ScreenStreamer::capture_screen()
{
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QPixmap pixmap = screen->grabWindow(0);
        //setPixmap(pixmap);
        QPixmap scaledPixmap = pixmap.scaled(label_self_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        label_self_->setPixmap(scaledPixmap);
        label_self_->setFixedSize(label_self_->size());
        send_image(pixmap);
    }
}

void ScreenStreamer::send_image(const QPixmap &pixmap)
{
    QImage image = pixmap.toImage();
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 10); // Сжатие до 50% качества

    send_callback_(byteArray);
    //int size = byteArray.size();
    //p2p_worker->send_data_p2p(size);
    //p2p_worker->send_data_p2p(byteArray); // Отправляем данные

}
