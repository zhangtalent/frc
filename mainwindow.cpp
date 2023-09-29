#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QHostAddress>
#include <QFileInfo>
#include <QDebug>
#include <QNetworkInterface>

class FrcApp : public QMainWindow
{
    Q_OBJECT

public:
    FrcApp(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("NOI学生端");
        setGeometry(100, 100, 350, 220);

        exam_label = new QLabel("准考证号:", this);
        exam_label->setGeometry(20, 20, 100, 35);

        exam_input = new QLineEdit(this);
        exam_input->setGeometry(120, 20, 200, 35);

        folder_label = new QLabel("工作文件夹:", this);
        folder_label->setGeometry(20, 70, 100, 35);

        folder_input = new QLineEdit(this);
        folder_input->setGeometry(120, 70, 200, 35);
        folder_input->setText(QDir::homePath() + "/"); // 设置默认工作文件夹为主目录

        button = new QPushButton("确定", this);
        button->setGeometry(20, 120, 300, 30);

        local_ip_text = new QLabel(this);
        local_ip_text->setGeometry(20, 150, 200, 60);
        local_ip_text->setWordWrap(true); // 允许文本换行

        connect(button, &QPushButton::clicked, this, &FrcApp::get_exam_info);

        QUdpSocket udpSocket;
        QString connect_to_server = connect_udp(udpSocket);
        if (connect_to_server != "200")
        {
            QMessageBox message_box(this);
            message_box.setWindowTitle("提示");
            message_box.setText("出错，重启软件！" + connect_to_server);
            message_box.exec();
            return;
        }

        QByteArray message = "1";
        udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
        socket_processor();
    }

private slots:
    void handle_confirmation(QAbstractButton *button)
    {
        QCoreApplication::quit();
    }

    void get_exam_info()
    {
        QString exam_number = exam_input->text();
        QString folder_path = folder_input->text();
        qDebug() << "准考证号:" << exam_number;
        qDebug() << "工作文件夹:" << folder_path;

        QMessageBox message_box(this);
        message_box.setWindowTitle("提示");
        message_box.setText(QString("你的准考证号是: %1\n工作文件夹是: %2\n设置成功！").arg(exam_number, folder_path));
        if (server_ip.isEmpty())
        {
            message_box.setText("等待考试服务器连接，请稍后重试!");
            message_box.exec();
            return;
        }
        local_ip_text->setText(QString("本地ip: %1\n服务器ip:%2").arg(get_local_ip(), server_ip));

        user_code = exam_number;
        dictory = folder_path;
        QByteArray message = QString("5%1*%2*%3").arg(server_ip, exam_number, folder_path).toUtf8();
        udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
        message_box.exec();
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        QCoreApplication::quit();
        QMainWindow::closeEvent(event);
    }

private:
    QLabel *exam_label;
    QLineEdit *exam_input;
    QLabel *folder_label;
    QLineEdit *folder_input;
    QPushButton *button;
    QLabel *local_ip_text;

    QString server_ip;
    QString group_ip = "224.0.0.1";
    quint16 port = 20000;
    QString user_code;
    QString dictory;
    QUdpSocket udpSocket;

    QString connect_udp(QUdpSocket &socket)
    {
        try
        {
            socket.bind(QHostAddress::Any, port);
            socket.joinMulticastGroup(QHostAddress(group_ip));
            return "200";
        }
        catch (const std::exception &e)
        {
            return QString::fromStdString(e.what());
        }
    }

    void socket_processor()
    {
        connect(&udpSocket, &QUdpSocket::readyRead, this, &FrcApp::receive_thread);
    }

    void receive_thread()
    {
        while (udpSocket.hasPendingDatagrams())
        {
            QByteArray datagram;
            datagram.resize(udpSocket.pendingDatagramSize());
            QHostAddress sender;
            quint16 sender_port;
            udpSocket.readDatagram(datagram.data(),
            datagram.size(), &sender, &sender_port);

            QString code;
            QString ip_addr;

            if (datagram.size() > 0)
            {
                QStringList code_and_ip = QString(datagram).split("*")[0].split("");
                code = code_and_ip[0];
                ip_addr = code_and_ip.mid(1).join("");
            }

            qDebug() << datagram << code << get_local_ip() << sender << ip_addr;

            if (code == "D" && ip_addr == get_local_ip())
            {
                server_ip = sender.toString();
                qDebug() << server_ip;
            }
            else if (code == "4")
            {
                server_ip = sender.toString();
                if (!user_code.isEmpty() && !dictory.isEmpty())
                {
                    QByteArray message = QString("5%1*%2*%3").arg(server_ip, user_code, dictory).toUtf8();
                    udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
                }
            }
            else if (code == "2" && ip_addr == get_local_ip())
            {
                qDebug() << "2";
                QByteArray message = QString("3%1").arg(server_ip).toUtf8();
                udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
            }
            else if (code == "B" && ip_addr == get_local_ip())
            {
                QStringList path_data = QString(datagram).split("*");
                QStringList file_paths;
                for (int i = 2; i < path_data.size(); i++)
                {
                    file_paths.append(path_data[i]);
                }
                // 处理文件路径列表
            }
        }
    }

    QString get_local_ip()
    {
        QString local_ip;
        QList<QHostAddress> ip_list = QNetworkInterface::allAddresses();
        for (const QHostAddress &address : ip_list)
        {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress::LocalHost)
            {
                local_ip = address.toString();
                break;
            }
        }
        return local_ip;
    }
};
