#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QFile>
#include <QTcpSocket>
#include "protocol.h"
#include "opewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TcpClient; }
//QT_END_NAMESPACE

class TcpClient : public QWidget
{
    Q_OBJECT

public:
    TcpClient(QWidget *parent = nullptr);
    ~TcpClient();

    void loadConfig();

    static TcpClient &getInstance();
    QTcpSocket &getTcpSocket();     //返回TcpSocket
    QString loginName();
    QString curPath();
    void setCurPath(QString strCurPath);    //当前路径

public slots:
    void showConnect(); //槽函数
    void recvMsg();

private slots:
    //void on_send_pd_clicked();

    void on_login_pb_clicked();

    void on_regist_pb_clicked();

    void on_cancel_pb_clicked();

private:
    Ui::TcpClient *ui;
    QString m_strIP; //IP
    quint16 m_usPort; //Port

    //连接服务器，和服务器进行数据交互
    QTcpSocket m_tcpSocket;
    QString m_strLoginName; //登入的用户名

    QString m_strCurPath;   //当前路径
    QFile m_file;
};
#endif // TCPCLIENT_H
