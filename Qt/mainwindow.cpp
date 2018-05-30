#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tcpSocketClient = new QTcpSocket(this);
    connect(tcpSocketClient, SIGNAL(readyRead()), this, SLOT(readMessage()));
    connect(tcpSocketClient, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(displayError(QAbstractSocket::SocketError)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::newConnect()
{
    // 初始化数据大小信息为0
    blockSize = 0;
    // 取消已有的连接
    tcpSocketClient->abort();
    tcpSocketClient->connectToHost(ui->hostLineEdit->text(),
                             ui->portLineEdit->text().toInt());
}

void MainWindow::readMessage()
{
    QByteArray byteArray;
    byteArray = tcpSocketClient->readAll();
    char *data_c = byteArray.data();
    qDebug() << data_c;
    QString str = QString(QLatin1String(data_c));
    ui->messageLineEdit->setText(str);
    /*
    char str[30];
    memset(str, 0, sizeof(str));
    (void *)strncpy(str, data_c, 20);
    str[29] = '\0';
    qDebug() << str;*/
    // 显示接收到的数据
    //qDebug() << message;
    //ui->messageLineEdit->setText(message);
}

void MainWindow::displayError(QAbstractSocket::SocketError)
{
    qDebug() << tcpSocketClient->errorString();
}

void MainWindow::on_connectButton_clicked()
{
    newConnect();
}
