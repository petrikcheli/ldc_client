#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "p2p.h"

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

private slots:
    void on_pb_reg_clicked();
    void on_pd_offer_clicked();

private:
    Ui::MainWindow *ui;

};
#endif // MAINWINDOW_H
