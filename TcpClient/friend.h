#ifndef FRIEND_H
#define FRIEND_H

#include <QWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "online.h"

class Friend : public QWidget
{
    Q_OBJECT
public:
    explicit Friend(QWidget *parent = nullptr);
    void showAllOnlineUsr(PDU *pdu);
    void updateFriendList(PDU *pdu);    //刷新好友列表
    void updateGroupMsg(PDU *pdu);
    QString m_strSearchName;    //临时的名字
    QListWidget *getFriendList();
signals:

public slots:
    void showOnline();
    void searchUsr();
    void flushFriend();
    void delFriend();
    void privateChat();
    void groupChat();

private:
    QTextEdit *m_pShowMsgTE;    //显示信息
    QListWidget *m_pFriendListWidget;   //显示好友列表
    QLineEdit *m_pInputMsgLE;   //信息输入框

    QPushButton *m_pDelFriendPB;    //删除好友
    QPushButton *m_pFlushFriendPB;  //刷新好友列表
    QPushButton *m_pShowOnlineUsrPB;//查看在线用户
    QPushButton *m_pSearchUsrPB;    //查找好友
    QPushButton *m_pMsgSendPB;      //发送信息
    QPushButton *m_pPrivateChatPB;  //私聊

    Online *m_pOnline;
};

#endif // FRIEND_H
