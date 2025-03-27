#include "screen_streamer.h"
#include <QApplication>
#include <QScreen>
#include <QBuffer>

//ScreenStreamer::ScreenStreamer() {}

ScreenStreamer::ScreenStreamer(QLabel *label, std::function<void( const QByteArray & )> send_callback,
                               QObject *parent)
    : QObject(parent), label_(label), timer_(new QTimer(this))
{
    this->send_callback_ = send_callback;

    // Настройка QLabel
    label_->setScaledContents(true);    // Масштабируем изображение
    label_->setMinimumSize(640, 480);   // Устанавливаем размер
}

void ScreenStreamer::start(){
    // Таймер для периодического обновления (например, 30 FPS)
    connect(timer_, &QTimer::timeout, this, &ScreenStreamer::capture_screen);
    timer_->start(33);  // ~30 FPS (1000ms / 30 ≈ 33ms)
}

void ScreenStreamer::update_frame(const QByteArray &image_data)
{
    QImage image;
    if (image.loadFromData(image_data)) {
        // Преобразуем QImage → QPixmap и устанавливаем в QLabel
        label_->setPixmap(QPixmap::fromImage(image));
    } else {
        qWarning("Ошибка: не удалось загрузить изображение из данных.");
    }
}

void ScreenStreamer::capture_screen()
{
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QPixmap pixmap = screen->grabWindow(0);
        //setPixmap(pixmap);
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
