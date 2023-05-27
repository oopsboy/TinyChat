#include "protocol.h"
#define _CRT_SECURE_NO_WARNINGS
#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>
#include "privatechat.h"

TcpClient::TcpClient(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpClient)
{
    ui->setupUi(this);
    this->resize(400, 200);
    loadConfig();

    connect(&m_tcpSocket, SIGNAL(connected())
            ,this,SLOT(showConnect()));
    //1.信号发送方，2.发送什么信号（连接成功后发出），3.信号接收方，4.用那个函数进行处理
    connect(&m_tcpSocket, SIGNAL(readyRead())
            , this, SLOT(recvMsg()));

    //连接服务器
    m_tcpSocket.connectToHost(QHostAddress(m_strIP), m_usPort);
}

TcpClient::~TcpClient()
{
    delete ui;
}

void TcpClient::loadConfig()
{
    QFile file(":/client.config");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray baData = file.readAll(); //字符类型
        QString strData = baData.toStdString().c_str(); //转换为char*形的
        //qDebug() << strData;
        file.close();

        strData.replace("\r\n", " ");   //替换
        //qDebug() << strData;

        QStringList strList = strData.split(" ");
    //    for (const QString &m:strList)
    //    {
    //        qDebug() << m;
    //    }
        m_strIP = strList.at(0);
        m_usPort = strList.at(1).toUShort();
        qDebug() << "ip:" << m_strIP << "port:" << m_usPort;
    }
    else
    {
        QMessageBox::critical(this, "open config", "open config failed");
    }
}

TcpClient &TcpClient::getInstance()
{
    static TcpClient instance;
    return instance;
}

QTcpSocket &TcpClient::getTcpSocket()
{
    return m_tcpSocket;
}

QString TcpClient::loginName()
{
    return m_strLoginName;
}

QString TcpClient::curPath()
{
    return m_strCurPath;
}

void TcpClient::setCurPath(QString strCurPath)
{
    m_strCurPath = strCurPath;
}

void TcpClient::showConnect()
{
    QMessageBox::information(this,QString::fromLocal8Bit("连接服务器"), QString::fromLocal8Bit("连接服务器成功"));
}

void TcpClient::recvMsg()
{
    if (!OpeWidget::getInstance().getBook()->getDownloadStatus())//是否在下载
    {
        qDebug() << m_tcpSocket.bytesAvailable();
        uint uiPDULen = 0;
        m_tcpSocket.read((char*)&uiPDULen, sizeof(uint));
        uint uiMsgLen = uiPDULen-sizeof(PDU);
        PDU *pdu = mkPDU(uiMsgLen);
        m_tcpSocket.read((char*)pdu+sizeof(uint), uiPDULen-sizeof(uint));
        //qDebug() << pdu->uiMsgType << (char*)(pdu->caMsg);
        //判断消息类型
        switch(pdu->uiMsgType)
        {
        case ENUM_MSG_TYPE_REGIST_RESPOND:  //注册回复消息
        {
            if (0 == strcmp(pdu->caData, REGIST_OK))    //验证回复信息是否正确
            {
                QMessageBox::information(this,QString::fromLocal8Bit("注册")
                                         ,REGIST_OK);
            }
            else if (0 == strcmp(pdu->caData, REGIST_FAILED))
            {
                QMessageBox::warning(this,QString::fromLocal8Bit("注册")
                                         ,REGIST_FAILED);
            }
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_RESPOND:  //登入回复消息
        {
            if (0 == strcmp(pdu->caData, LOGIN_OK))    //验证回复信息是否正确
            {
                m_strCurPath = QString("./%1").arg(m_strLoginName);
                QMessageBox::information(this,QString::fromLocal8Bit("登入")
                                         ,LOGIN_OK);
                OpeWidget::getInstance().show();
                this->hide();
            }
            else if (0 == strcmp(pdu->caData, LOGIN_FAILED))
            {
                QMessageBox::warning(this,QString::fromLocal8Bit("登入")
                                         ,LOGIN_FAILED);
            }
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND:
        {
            OpeWidget::getInstance().getFriend()->showAllOnlineUsr(pdu);
            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_RESPOND:
        {
            if (0 == strcmp(SEARCH_USR_NO, pdu->caData))
            {
                QMessageBox::information(this, QString::fromLocal8Bit("搜索")
                                         , QString("%1: not exist").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
            }
            else if (0 == strcmp(SEARCH_USR_ONLINE, pdu->caData))
            {
                QMessageBox::information(this, QString::fromLocal8Bit("搜索")
                                         , QString("%1: online").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
            }
            else if (0 == strcmp(SEARCH_USR_OFFLINE, pdu->caData))
            {
                QMessageBox::information(this, QString::fromLocal8Bit("搜索")
                                         , QString("%1: offline").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
            }
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData+32, 32);
            int ret = QMessageBox::information(this, QString::fromLocal8Bit("添加好友")
                                               , QString("%1 want to add you as friend ?").arg(caName)
                                               , QMessageBox::Yes, QMessageBox::No);
            PDU *respdu = mkPDU(0);
            memcpy(respdu->caData, pdu->caData, 64);
            if (QMessageBox::Yes == ret)
            {
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGGREE;
            }
            else
            {
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REFUSE;
            }
            m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("添加好友"), pdu->caData);
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_AGGREE:
        {
            char caPerName[32] = {'\0'};
            memcpy(caPerName, pdu->caData, 32);
            QMessageBox::information(this, QString::fromLocal8Bit("添加好友"), QString("add %1 friend succeed").arg(caPerName));
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        {
            char caPerName[32] = {'\0'};
            memcpy(caPerName, pdu->caData, 32);
            QMessageBox::information(this, QString::fromLocal8Bit("添加好友"), QString("add %1 friend failed").arg(caPerName));
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:
        {
            OpeWidget::getInstance().getFriend()->updateFriendList(pdu);
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            memcpy(caName, pdu->caData, 32);
            QMessageBox::information(this, QString::fromLocal8Bit("删除好友"), QString("%1 delete friend").arg(caName));
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("删除好友"), QString::fromLocal8Bit("删除好友成功"));
            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            if (PrivateChat::getInstance().isHidden())
            {
                PrivateChat::getInstance().show();
            }
            char caSendName[32] = {'\0'};
            memcpy(caSendName, pdu->caData, 32);
            QString strSendName = caSendName;
            PrivateChat::getInstance().setChatName(strSendName);

            PrivateChat::getInstance().updateMsg(pdu);

            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            OpeWidget::getInstance().getFriend()->updateGroupMsg(pdu);
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("创建文件"), pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:
        {
            OpeWidget::getInstance().getBook()->updateFileList(pdu);
            QString strEnterDir = OpeWidget::getInstance().getBook()->enterDir();
            if (!strEnterDir.isEmpty())
            {
                m_strCurPath = m_strCurPath+"/"+strEnterDir;
                qDebug() << "enter dir:" << m_strCurPath;
            }
            break;
        }
        case ENUM_MSG_TYPE_DEL_DIR_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("删除文件夹"), pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("重命名文件"), pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_ENTER_DIR_RESPOND:
        {
            OpeWidget::getInstance().getBook()->clearEnterDir();
            QMessageBox::information(this, QString::fromLocal8Bit("进入文件夹"), pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_DEL_FILE_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("删除文件"), pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("上传文件"), pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND:
        {
            qDebug() << pdu->caData;
            char caFileName[32] = {'\0'};
            //从字符串读取格式化输入
            sscanf(pdu->caData, "%s %lld", caFileName, &(OpeWidget::getInstance().getBook()->m_iTotal));
            if (strlen(caFileName) > 0 && OpeWidget::getInstance().getBook()->m_iTotal > 0)
            {
                OpeWidget::getInstance().getBook()->setDownloadStatus(true);
                //不用局部变量，函数执行完会自动释放
                m_file.setFileName(OpeWidget::getInstance().getBook()->getSaveFilePath());
                if (!m_file.open(QIODevice::WriteOnly))
                {
                    QMessageBox::warning(this, QString::fromLocal8Bit("下载文件")
                                         , QString::fromLocal8Bit("获得保存文件的路径失败"));
                }
            }
            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("共享文件"), pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTE:
        {
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            // //aa//bb/cc
            char *pos = strrchr(pPath, '/');//从后面找一个字符
            if (NULL != pos)
            {
                pos++;
                QString strNote = QString("%1 share file->%2 \n Do you accept ?").arg(pdu->caData).arg(pos);
                int ret = QMessageBox::question(this, QString::fromLocal8Bit("共享文件"), strNote);
                if (QMessageBox::Yes == ret)
                {
                    PDU *respdu = mkPDU(pdu->uiMsgLen);
                    respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND;
                    memcpy(respdu->caMsg, pdu->caMsg, pdu->uiMsgLen);
                    QString strName = TcpClient::getInstance().loginName();
                    strcpy(respdu->caData, strName.toStdString().c_str());
                    m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
                }
            }
            break;
        }
        case ENUM_MSG_TYPE_MOVE_FILE_RESPOND:
        {
            QMessageBox::information(this, QString::fromLocal8Bit("移动文件"), pdu->caData);
        }
        default:
            break;
        }
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QByteArray buffer = m_tcpSocket.readAll();
        m_file.write(buffer);
        Book *pBook = OpeWidget::getInstance().getBook();
        pBook->m_iRecved += buffer.size();
        if (pBook->m_iTotal == pBook->m_iRecved)
        {
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);
            QMessageBox::information(this, QString::fromLocal8Bit("下载文件")
                                     , QString::fromLocal8Bit("下载文件成功"));
        }
        else if (pBook->m_iTotal < pBook->m_iRecved)
        {
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);

            QMessageBox::critical(this, QString::fromLocal8Bit("下载文件")
                                  , QString::fromLocal8Bit("下载文件失败"));
        }
    }
}

#if 0
void TcpClient::on_send_pd_clicked()
{
    QString strMsg = ui->lineEdit->text();
    if (!strMsg.isEmpty())
    {
        PDU *pdu = mkPDU(strMsg.size()+1);
        pdu->uiMsgType = 8888;
        memcpy(pdu->caMsg, strMsg.toStdString().c_str(), strMsg.size());
        qDebug() << (char*)(pdu->caMsg);
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("信息发送")
                             , QString::fromLocal8Bit("发送的信息不能为空"));
    }
}
#endif

void TcpClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text();  //获得输入的用户名
    QString strPwd = ui->pwd_le->text();
    if(!strName.isEmpty()&&!strPwd.isEmpty())
    {
        m_strLoginName = strName;
        PDU *pdu = mkPDU(0);    //消息部分为0
        pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;
        strncpy(pdu->caData, strName.toStdString().c_str(), 32);    //前32字节放用户名
        strncpy(pdu->caData+32, strPwd.toStdString().c_str(), 32);
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::critical(this,QString::fromLocal8Bit("登入")
                              , QString::fromLocal8Bit("登入失败：用户名或密码为空"));
    }
}


void TcpClient::on_regist_pb_clicked()
{
    QString strName = ui->name_le->text();  //获得输入的用户名
    QString strPwd = ui->pwd_le->text();
    if(!strName.isEmpty()&&!strPwd.isEmpty())
    {
        PDU *pdu = mkPDU(0);    //消息部分为0
        pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;
        strncpy(pdu->caData, strName.toStdString().c_str(), 32);    //前32字节放用户名
        strncpy(pdu->caData+32, strPwd.toStdString().c_str(), 32);
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::critical(this,QString::fromLocal8Bit("注册")
                              , QString::fromLocal8Bit("注册失败：用户名或密码为空"));
    }
}



void TcpClient::on_cancel_pb_clicked()
{

}

