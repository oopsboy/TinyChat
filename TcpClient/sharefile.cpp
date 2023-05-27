﻿#include "sharefile.h"
#include "tcpclient.h"
#include "opewidget.h"

ShareFile::ShareFile(QWidget *parent)
    : QWidget{parent}
{
    m_pSelectAllPB = new QPushButton(QString::fromLocal8Bit("全选"));
    m_pCancelSelectPB = new QPushButton(QString::fromLocal8Bit("取消选择"));

    m_pOKPB = new QPushButton(QString::fromLocal8Bit("确定"));
    m_pCancelPB = new QPushButton(QString::fromLocal8Bit("取消"));

    m_pSA = new QScrollArea;
    m_pFriendW = new QWidget;
    m_pFriendWVBL = new QVBoxLayout(m_pFriendW);
    m_pButtonGroup = new QButtonGroup(m_pFriendW);
    m_pButtonGroup->setExclusive(false);//false就是多选

    QHBoxLayout *pTopHBL = new QHBoxLayout;
    pTopHBL->addWidget(m_pSelectAllPB);
    pTopHBL->addWidget(m_pCancelSelectPB);
    pTopHBL->addStretch();

    QHBoxLayout *pDownHBL = new QHBoxLayout;
    pDownHBL->addWidget(m_pOKPB);
    pDownHBL->addWidget(m_pCancelPB);

    QVBoxLayout *pMainVBL = new QVBoxLayout;
    pMainVBL->addLayout(pTopHBL);
    pMainVBL->addWidget(m_pSA);
    pMainVBL->addLayout(pDownHBL);
    setLayout(pMainVBL);

    connect(m_pCancelSelectPB, SIGNAL(clicked(bool))
            , this, SLOT(cancelSelect()));

    connect(m_pSelectAllPB, SIGNAL(clicked(bool))
            , this, SLOT(selectAll()));

    connect(m_pOKPB, SIGNAL(clicked(bool))
            , this, SLOT(okShare()));

    connect(m_pCancelPB, SIGNAL(clicked(bool))
            , this, SLOT(cancelShare()));

}

ShareFile &ShareFile::getInstance()
{
    static ShareFile instance;
    return instance;
}

void ShareFile::test()
{
    QVBoxLayout *p = new QVBoxLayout(m_pFriendW);
    QCheckBox *pCB = NULL;
    for (int i = 0; i<15; i++)
    {
        pCB = new QCheckBox("jack");
        p->addWidget(pCB);
        m_pButtonGroup->addButton(pCB);
    }
    m_pSA->setWidget(m_pFriendW);
}

void ShareFile::updateFriend(QListWidget *pFriendList)
{
    if (NULL == pFriendList)
    {
        return;
    }
    QAbstractButton *tmp = NULL;
    QList<QAbstractButton*> preFriendList = m_pButtonGroup->buttons();//之前显示的好友
    for (int i = 0; i<preFriendList.size(); i++)
    {
        //移除之前的
        tmp = preFriendList[i];
        m_pFriendWVBL->removeWidget(tmp);
        m_pButtonGroup->removeButton(tmp);
        preFriendList.removeOne(tmp);
        delete tmp;
        tmp = NULL;
    }
    QCheckBox *pCB = NULL;
    for (int i = 0; i<pFriendList->count(); i++)
    {
        pCB = new QCheckBox(pFriendList->item(i)->text());
        m_pFriendWVBL->addWidget(pCB);
        m_pButtonGroup->addButton(pCB);
    }
    m_pSA->setWidget(m_pFriendW);
}

void ShareFile::cancelSelect()
{
    QList<QAbstractButton*> cbList = m_pButtonGroup->buttons();//获得所有指针列表
    for (int i=0; i<cbList.size(); i++)
    {
        if (cbList[i]->isChecked())
        {
            cbList[i]->setChecked(false);
        }
    }
}

void ShareFile::selectAll()
{
    QList<QAbstractButton*> cbList = m_pButtonGroup->buttons();
    for (int i=0; i<cbList.size(); i++)
    {
        if (!cbList[i]->isChecked())
        {
            cbList[i]->setChecked(true);
        }
    }
}

void ShareFile::okShare()
{
    QString strName = TcpClient::getInstance().loginName();//获得分享者
    QString strCurPath = TcpClient::getInstance().curPath();//当前路径
    QString strShareFileName = OpeWidget::getInstance().getBook()->getShareFileName();//分享的文件名

    QString strPath = strCurPath+"/"+strShareFileName;

    QList<QAbstractButton*> cbList = m_pButtonGroup->buttons();
    int num = 0;
    for (int i=0; i<cbList.size(); i++)
    {
        if (cbList[i]->isChecked())
        {
            num++;
        }
    }

    PDU *pdu = mkPDU(32*num+strPath.size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_REQUEST;
    sprintf(pdu->caData, "%s %d", strName.toStdString().c_str(), num);
    int j = 0;
    for (int i=0; i<cbList.size(); i++)
    {
        //获取被分享者的名字
        if (cbList[i]->isChecked())
        {
            memcpy((char*)(pdu->caMsg)+j*32, cbList[i]->text().toStdString().c_str(), cbList[i]->text().size());
            j++;
        }
    }

    memcpy((char*)(pdu->caMsg)+num*32, strPath.toStdString().c_str(), strPath.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);

    free(pdu);
    pdu = NULL;
}

void ShareFile::cancelShare()
{
    hide();
}