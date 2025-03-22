#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "p2p.h"

#include <QMainWindow>
#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include <QBuffer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void connection_to_the_server(io_context &ioc, ip::tcp::resolver &resolver);
    std::shared_ptr<p2p> p2p_worker;
    void update_screen(const QByteArray &image_data);

public slots:
    void captureScreen() {
        QScreen *screen = QApplication::primaryScreen();
        if (screen) {
            QPixmap pixmap = screen->grabWindow(0);
            //setPixmap(pixmap);
            sendImage(pixmap);
        }
    }

    void sendImage(const QPixmap &pixmap) {
        QImage image = pixmap.toImage();
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPEG", 10); // Сжатие до 50% качества

        int size = byteArray.size();
        p2p_worker->send_data_p2p(size);
        p2p_worker->send_data_p2p(byteArray); // Отправляем данные
        //socket->flush();
    }

private slots:
    void on_pb_reg_clicked();
    void on_pd_offer_clicked();
    void on_pb_share_clicked();

private:
    Ui::MainWindow *ui;

};
#endif // MAINWINDOW_H
