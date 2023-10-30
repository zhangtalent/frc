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
#include <QNetworkInterface>
#include <QFileInfo>
#include <QNetworkProxy>
#include <QTcpServer>
#include <QDebug>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <zlib.h>
#include <QTimer>
#include <QThread>
#include <QTextCodec>


class FrcApp : public QMainWindow
{
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

        QString connect_to_server = connect_udp();
        //qDebug() << "error:" << connect_to_server;
        if (connect_to_server != "200")
        {
            QMessageBox message_box(this);
            message_box.setWindowTitle("提示");
            message_box.setText("出错，重启软件！" + connect_to_server);
            // 设置消息框的按钮
            QPushButton* confirmButton = message_box.addButton("确认", QMessageBox::AcceptRole);
            // 设置消息框的默认按钮
            message_box.setDefaultButton(confirmButton);
            // 显示消息框并处理按钮点击事件
            message_box.exec();
            if (message_box.clickedButton() == confirmButton)
            {
               QApplication::quit(); // 退出应用程序
            }
            return;
        }

        QByteArray message = "1";
        udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
        // 定时器
        connect(&timer, &QTimer::timeout, this, &FrcApp::sendUdpMessage);

        // 设置定时器间隔，单位为毫秒
        int interval = 1000; // 每1秒发送一次
        timer.setInterval(interval);

        // 启动定时器
        timer.start();

        socket_processor();
    }

private slots:
    void showMessage(const QString& message)
    {
        // 在主线程中显示消息框
        QMessageBox::information(nullptr, "消息", message);
    }
    void handle_confirmation(QAbstractButton *button)
    {
        QCoreApplication::quit();
    }
    void sendUdpMessage()
    {
        if(server_ip!=""){
            QByteArray message = QString("5%1*%2*%3").arg(server_ip, "", QDir::homePath() + "/").toUtf8();
            udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
            timer.stop();
            return;
        }
        // 每次定时器超时，发送消息
        QByteArray message = "1";
        udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
    }
    void handleNewConnection()
    {
        // 接受客户端连接
        QTcpSocket *clientSocket = server.nextPendingConnection();
        //qDebug() << "Client connected:" << clientSocket->peerAddress().toString() << clientSocket->peerPort();

        // 处理数据接收
        connect(clientSocket, &QTcpSocket::readyRead, this, &FrcApp::handleReadyRead);
        connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
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
    QTimer timer;
    QByteArray newData;
    QString server_ip;
    QString group_ip = "224.0.0.1";
    quint16 port = 20000;
    QString user_code;
    QString dictory;
    QUdpSocket udpSocket;
    // 创建服务器对象
    QTcpServer server;
    QString filename;
    QString file_size_and_crc_32;
    QFile file;
    bool receivingFileName = true;
    bool receivingFileData = false;
    bool receivingFileOver = false;
    quint64 fileSize = 0;
    quint64 receivedSize = 0;

    void get_exam_info()
    {
        QString exam_number = exam_input->text();
        QString folder_path = folder_input->text();
//        qDebug() << "准考证号:" << exam_number;
//        qDebug() << "工作文件夹:" << folder_path;
//        qDebug() << "IP:" << server_ip;
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

    QString connect_udp()
    {
//         qDebug() << "xxxxxx" << "111";
        try
        {
            udpSocket.bind(QHostAddress::AnyIPv4, port);
            QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
            QString result = "端口被占用或网错误";
            foreach (const QNetworkInterface& interface, interfaces) {
                if (interface.flags() & QNetworkInterface::IsUp) {
                    if (udpSocket.joinMulticastGroup(QHostAddress(group_ip), interface)) {
//                        qDebug() << "Joined multicast group on interface:" << interface.name();
                        result  = "200";
                        break;
                    } else {
//                        qDebug() << "Failed to join multicast group on interface:" << interface.name();
                    }
                }
            }
            return result;
        }
        catch (const std::exception &e)
        {
//            qDebug() << "xxxxxx" << e.what();
            return QString::fromStdString(e.what());
        }
    }

    void socket_processor()
    {
        connect(&udpSocket, &QUdpSocket::readyRead, this, &FrcApp::receive_thread);
    // 创建服务器对象

        // 监听连接请求
        connect(&server, &QTcpServer::newConnection, this, &FrcApp::handleNewConnection);

        // 启动服务器
        if (!server.listen(QHostAddress::AnyIPv4, 20001))
        {
//            qDebug() << "Failed to start server:" << server.errorString();
            return;
        }

//        qDebug() << "Server started. Waiting for client connections...";
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
                QString code_and_ip = QString(datagram).split("*")[0];
                code = code_and_ip[0];
                ip_addr = code_and_ip.mid(1);
            }

//            qDebug() << datagram << code << get_local_ip() << sender << ip_addr;

            if (code == "D" && ip_addr == get_local_ip())
            {
                server_ip = sender.toString();
//                qDebug() << server_ip;
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
//                qDebug() << "2";
                QByteArray message = QString("3%1").arg(server_ip).toUtf8();
                udpSocket.writeDatagram(message, QHostAddress(group_ip), port);
            }
            else if (code == "B" && ip_addr == get_local_ip()) {
               // 处理字符串变成path_list
               QStringList path_data = QString(datagram).split("*");
               QStringList file_paths;
               for (int i = 2; i < path_data.size(); ++i) {
                   file_paths.append(path_data[i]);
               }
               send_file(file_paths);
               // 发完数据，使用7通知
               QString message = "7" + server_ip;
               udpSocket.writeDatagram(message.toUtf8(), QHostAddress(group_ip), port);
            } else {
               // 其他情况的处理
            }
        }
    }

    qint64 get_file_size(const QString& file_path)
    {
        QFileInfo file_info(file_path);
        if (!file_info.exists() || !file_info.isFile()) {
//            qDebug() << "无法获取文件大小：";
            return 0;
        }

        return file_info.size();
    }

    void send_file(const QStringList& file_paths)
    {
        for (const QString& file_path : file_paths) {
            quint16 server_port = 20001;
            // 创建套接字并连接到服务器
            QTcpSocket client_socket;
            // 设置代理类型
            QNetworkProxy::ProxyType proxyType = QNetworkProxy::NoProxy; // 替换为所需的代理类型
            client_socket.setProxy(proxyType);
            client_socket.connectToHost(server_ip, server_port);
            if (!client_socket.waitForConnected(5000)) {
                // 连接失败的处理
                // 连接失败的处理
//                qDebug() << "连接失败：" << client_socket.errorString();
                client_socket.disconnectFromHost();
                QThread::sleep(1);
                continue;
            }
            QString linux_path = file_path;
            QString windows_path = file_path;
            linux_path.replace("\\", "/");
            QString liunx_filename = dictory + "" + user_code + "/" + linux_path;
            QString windows_filename = dictory + "\\" + user_code + "\\" + windows_path;
            bool exists = QFile::exists(liunx_filename);
            if (!exists) {
                client_socket.disconnectFromHost();
                QThread::sleep(1);
                continue;
            }
            qint64 file_size = get_file_size(liunx_filename);
            // 将文件大小转换为十六进制
            QString hex_string = "*" + calculate_crc32(liunx_filename);
//            qDebug() << liunx_filename;
            // 发送文件名
            QString message = QString("/%1/%2|%3%4\r\n").arg(user_code).arg(file_path).arg(file_size).arg(hex_string);

            client_socket.write(message.toUtf8());
            if (!client_socket.waitForBytesWritten(5000))
            {
//               qDebug() << "Failed to send packet.";
              client_socket.disconnectFromHost();
              QThread::sleep(1);
              continue;
            }
            // 读取文件数据并发送给服务器
            QFile file(liunx_filename);
            try {
                    if (!file.open(QIODevice::ReadOnly)) {
                        throw QString("Failed to open the file");
                    }
                    while (!file.atEnd()) {
                        QByteArray data = file.readAll();
//                        qDebug() << data;
                        client_socket.write(data);
                        if (!client_socket.waitForBytesWritten(5000))
                       {
//                           qDebug() << "Failed to send packet.";
                          break;
                       }
                    }
                    // 文件打开成功，可以在这里执行读取操作或其他操作
                    file.close();
                } catch (const QString& error) {
//                    qDebug() << "Error: " << error;
                }

            // 关闭连接
            client_socket.disconnectFromHost();
            QThread::sleep(1);
        }
//        qDebug() << "文件发送完成。";
    }


    void handleReadyRead()
    {
        QObject *senderObj = sender();
        if (!senderObj)
            return;
        QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
        if (!clientSocket){
            return;
        }


        while (clientSocket->bytesAvailable() > 0)
        {


            //qDebug() << "Received data:" << newData;
            if (receivingFileName)
            {
                QByteArray chunk = clientSocket->readLine(1024);
                newData.append(chunk);
                QTextCodec* codec = QTextCodec::codecForName("GBK");
                QString decodedString = codec->toUnicode(newData);
                //qDebug() << newData << decodedString;
                if(decodedString.contains("\r\n")){

                    int endIndex = decodedString.indexOf("\r\n");
                    QString recData = decodedString.left(endIndex);
                    QString filePath = recData.split('|').at(0);
                    file_size_and_crc_32 = recData.split("|").at(1);
                    // 提取文件名
                    filename = QDir::homePath() + "/" + QRegularExpression("[^\\\\/:*?\"<>|\\r\\n]+$").match(filePath).captured();
                    //filename = filename
                    //qDebug() << filename;
                   //qDebug() << "Received tes:" << newData;
                    receivingFileName = false;
                    receivingFileData = true;
                    newData.clear();
                    // 打开文件
                    file.setFileName(filename);
                    if (!file.open(QIODevice::WriteOnly))
                    {
                        qDebug() << "Failed to open file for writing:" << filename;
                        return;
                    }
                }

                //break;
            } else if (receivingFileData){
                QByteArray chunk = clientSocket->readAll();
                newData.append(chunk);
                bool ok;
                quint32 file_size_receive = file_size_and_crc_32.split("*").at(0).toUInt(&ok);
                if (!ok) {
//                    qDebug() << "convert_fail:";
                    // 转换失败的处理逻辑
                }
                if(!newData.isEmpty()){
                    file.write(newData);
                }

                receivedSize += newData.size();
                //qDebug() << "size" << receivedSize;
                newData.clear();
                if(receivedSize >= file_size_receive){

                 file.close();
                 QString crc_32_receive = file_size_and_crc_32.split("*").at(1);
                 QString crc_32_file = calculate_crc32(filename).toUpper();
                 qint64 file_size = get_file_size(filename);
                 qint64 fileSize = file_size_and_crc_32.split("*").at(0).toUInt();
//                 qDebug() << crc_32_receive << crc_32_file << file_size << fileSize;
                 if(file_size != fileSize || crc_32_receive != crc_32_file){
                     //qDebug() << "File 111:" << filename;
                     //showMessage(QString("收到文件，已保存到%1,CRC校验失败【%2】").arg(filename, crc_32_file));
                     //message_box.exec();
                 } else{
                     //qDebug() << "File 222:" << filename;
                     //showMessage(QString("收到文件，已保存到%1,CRC校验通过【%2】").arg(filename, crc_32_file));
                     //message_box.exec();
                 }
                 //qDebug() << "File received and saved:" << filename;
                 // 关闭当前客户端连接
//
                 receivingFileName = true;
                 receivingFileData = false;
                 receivingFileOver = true;
                 receivedSize=0;
                 break;
                } else{
                        //qDebug() << "waiting data:" ;
                }
                //qDebug() << "Received data:" ;
            } else {
            }
        }

        if(receivingFileOver){
            receivedSize=0;
            receivingFileOver = false;
        }

    }

    QString calculate_crc32(const QString& file_path)
    {
        QFile file(file_path);
        if (!file.open(QIODevice::ReadOnly)) {
//            qDebug() << "无法打开文件：" << file_path;
            return QString();
        }

        QByteArray fileData = file.readAll();
        file.close();

        uLong crc_value = crc32(0L, reinterpret_cast<const Bytef*>(fileData.constData()), fileData.size());

        QString crc_string = QString("%1").arg(crc_value, 8, 16, QChar('0')).right(8);

        return crc_string;
    }

    void receiveFileThread()
    {

//        qDebug() << "Server started. Waiting for client connections...";

        while (true)
        {
            // 等待客户端连接
            server.waitForNewConnection();

            // 接受客户端连接
            QTcpSocket *clientSocket = server.nextPendingConnection();
            //qDebug() << "Client connected:" << clientSocket->peerAddress().toString() << clientSocket->peerPort();

            QByteArray receivedData;
            bool endMarkerFound = false;

            while (clientSocket->state() == QAbstractSocket::ConnectedState)
            {
                // 接收数据
                if (clientSocket->bytesAvailable() > 0)
                {
                    QByteArray chunk = clientSocket->read(1024);
                    receivedData += chunk;

                    if (receivedData.contains("\r\n"))
                    {
                        endMarkerFound = true;
                        break;
                    }
                }

                // 检查连接状态
                if (clientSocket->state() == QAbstractSocket::UnconnectedState)
                    break;


            }

            if (endMarkerFound)
            {
//                qDebug() << "Received data:" << receivedData;
//qDebug() << "Received datadddd:" << receivedData;
                QString filePath = receivedData;
                filePath = filePath.split('|').at(0);
                QString file_size_and_crc_32 = filePath.split("|").at(1);

                // 提取文件名
                QString filename = QDir::homePath() + "/" + QRegularExpression("[^\\\\/:*?\"<>|\\r\\n]+$").match(filePath).captured();
//                qDebug() << filename;
                // 接收文件数据并写入文件
                QFile file(filename);
                if (file.open(QIODevice::WriteOnly))
                {
                    while (clientSocket->state() == QAbstractSocket::ConnectedState)
                    {
//                         qDebug() << clientSocket->state();
                        // 接收数据
                        if (clientSocket->bytesAvailable() > 0)
                        {
                            QByteArray data = clientSocket->readAll();
                            file.write(data);
                        }

                        // 检查连接状态
                        if (clientSocket->state() == QAbstractSocket::UnconnectedState)
                            break;


                    }
                    bool ok;
                    quint32 file_size_receive = file_size_and_crc_32.split("*").at(0).toUInt(&ok);
                    if (!ok) {
//                        qDebug() << "convert_fail:";
                        // 转换失败的处理逻辑
                    }
                    QString crc_32_receive = file_size_and_crc_32.split("*").at(1);
                    QString crc_32_file = calculate_crc32(filename);
                    qint64 file_size = get_file_size(filename);
//                    qDebug() <<  "1111";
                    //qDebug() << file_size << "=" << file_size_receive << "=" << crc_32_receive << "=" << crc_32_file;
                    if(file_size != file_size_receive || crc_32_receive != crc_32_file){
                        //qDebug() << "File 111:" << filename;
                        QMessageBox message_box(this);
                        message_box.setWindowTitle("提示");
                        message_box.setText(QString("收到文件，已保存到%1,CRC校验失败【%2】").arg(filename, crc_32_file));
                        message_box.exec();
                    } else{
                        //qDebug() << "File 222:" << filename;
                        QMessageBox message_box(this);
                        message_box.setWindowTitle("提示");
                        message_box.setText(QString("收到文件，已保存到%1,CRC校验通过【%2】").arg(filename, crc_32_file));
                        message_box.exec();
                    }

                    //calculate_crc32()
                    //qDebug() << "File received:" << filename;
                }
                else
                {
                    //qDebug() << "Failed to open file:" << filename;
                }

                file.close();
            }

            // 关闭当前客户端连接
            clientSocket->close();
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    FrcApp frcApp;
    frcApp.show();

    return app.exec();
}
