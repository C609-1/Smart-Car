#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QAbstractSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();

    void newConnect();
    void readMessage();
    void displayError(QAbstractSocket::SocketError);

private:
    Ui::MainWindow *ui;
    QTcpSocket *tcpSocketClient;      //tcp客户端
    QString message;                  //接收数据
    quint16 blockSize;                //存放数据的大小
};

#endif // MAINWINDOW_H
