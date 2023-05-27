﻿#include "book.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include "opewidget.h"
#include "sharefile.h"

Book::Book(QWidget *parent) : QWidget(parent)
{
    m_strEnterDir.clear();

    m_bDownload = false;
    m_pTimer = new QTimer;

    m_pBookListW = new QListWidget;
    m_pReturnPB = new QPushButton(QString::fromLocal8Bit("返回"));
    m_pCreateDirPB = new QPushButton(QString::fromLocal8Bit("创建文件夹"));
    m_pDelDirPB = new QPushButton(QString::fromLocal8Bit("删除文件夹"));
    m_pRenamePB = new QPushButton(QString::fromLocal8Bit("重命名文件"));
    m_pFlushFilePB = new QPushButton(QString::fromLocal8Bit("刷新文件"));

    QVBoxLayout *pDirVBL = new QVBoxLayout; //垂直布局
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelDirPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);


    m_pUploadPB = new QPushButton(QString::fromLocal8Bit("上传文件"));
    m_pDownLoadPB = new QPushButton(QString::fromLocal8Bit("下载文件"));
    m_pDelFilePB = new QPushButton(QString::fromLocal8Bit("删除文件"));
    m_pShareFilePB = new QPushButton(QString::fromLocal8Bit("共享文件"));
    m_pMoveFilePB = new QPushButton(QString::fromLocal8Bit("移动文件"));
    m_pSelectDirPB = new QPushButton(QString::fromLocal8Bit("目标目录"));
    m_pSelectDirPB->setEnabled(false);

    QVBoxLayout *pFileVBL = new QVBoxLayout; //垂直布局
    pFileVBL->addWidget(m_pUploadPB);
    pFileVBL->addWidget(m_pDownLoadPB);
    pFileVBL->addWidget(m_pDelFilePB);
    pFileVBL->addWidget(m_pShareFilePB);
    pFileVBL->addWidget(m_pMoveFilePB);
    pFileVBL->addWidget(m_pSelectDirPB);

    QHBoxLayout *pMain = new QHBoxLayout;//水平布局
    pMain->addWidget(m_pBookListW);
    pMain->addLayout(pDirVBL);
    pMain->addLayout(pFileVBL);

    setLayout(pMain);

    connect(m_pCreateDirPB, SIGNAL(clicked(bool))
            , this, SLOT(createDir()));
    connect(m_pFlushFilePB, SIGNAL(clicked(bool))
            , this, SLOT(flushFile()));
    connect(m_pDelDirPB, SIGNAL(clicked(bool))
            , this, SLOT(delDir()));

    connect(m_pRenamePB, SIGNAL(clicked(bool))
            , this, SLOT(renameFile()));

    connect(m_pBookListW, SIGNAL(doubleClicked(QModelIndex))
            , this, SLOT(enterDir(QModelIndex)));

    connect(m_pReturnPB, SIGNAL(clicked(bool))
            , this, SLOT(returnPre()));

    connect(m_pDelFilePB, SIGNAL(clicked(bool))
            , this, SLOT(delRegFile()));

    connect(m_pUploadPB, SIGNAL(clicked(bool))
            , this, SLOT(uploadFile()));

    connect(m_pTimer, SIGNAL(timeout())
            , this, SLOT(uploadFileData()));

    connect(m_pDownLoadPB, SIGNAL(clicked(bool))
            , this, SLOT(downloadFile()));

    connect(m_pShareFilePB, SIGNAL(clicked(bool))
            , this, SLOT(shareFile()));

    connect(m_pMoveFilePB, SIGNAL(clicked(bool))
            , this, SLOT(moveFile()));

    connect(m_pSelectDirPB, SIGNAL(clicked(bool))
            , this, SLOT(selectDestDir()));
}

void Book::updateFileList(const PDU *pdu)
{
    if (NULL == pdu)
    {
        return;
    }

    QListWidgetItem *pItemTmp = NULL;
    int row = m_pBookListW->count();
    while (m_pBookListW->count()>0)
    {
        pItemTmp = m_pBookListW->item(row-1);
        m_pBookListW->removeItemWidget(pItemTmp);
        delete pItemTmp;
        row = row-1;
    }

    FileInfo *pFileInfo = NULL;
    int iCount = pdu->uiMsgLen/sizeof(FileInfo);
    for (int i=0; i<iCount; i++)
    {
        pFileInfo = (FileInfo*)(pdu->caMsg)+i;
        qDebug() << pFileInfo->caFileName << pFileInfo->iFileType;
        QListWidgetItem *pItem = new QListWidgetItem;
        if (0 == pFileInfo->iFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":/map/dir.jpg")));
        }
        else if (1 == pFileInfo->iFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":/map/reg.jpg")));
        }
        pItem->setText(pFileInfo->caFileName);
        m_pBookListW->addItem(pItem);
    }
}

void Book::clearEnterDir()
{
    m_strEnterDir.clear();
}

QString Book::enterDir()
{
    return m_strEnterDir;
}

void Book::setDownloadStatus(bool status)
{
    m_bDownload = status;
}

bool Book::getDownloadStatus()
{
    return m_bDownload;
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

QString Book::getShareFileName()
{
    return m_strShareFileName;
}

void Book::createDir()
{
    //得到新文件夹名字
    QString strNewDir = QInputDialog::getText(this, QString::fromLocal8Bit("新建文件夹")
                                              , QString::fromLocal8Bit("新文件夹名字"));
    if (!strNewDir.isEmpty())
    {
        if (strNewDir.size()>32)
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("新建文件夹")
                                 , QString::fromLocal8Bit("新文件夹名字不能超过32个字符"));
        }
        else
        {
            QString strName = TcpClient::getInstance().loginName(); //获得登入的名字
            QString strCurPath = TcpClient::getInstance().curPath();//路径
            PDU *pdu = mkPDU(strCurPath.size()+1);
            pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_REQUEST;
            strncpy(pdu->caData, strName.toStdString().c_str(), strName.size());
            strncpy(pdu->caData+32, strNewDir.toStdString().c_str(), strNewDir.size());
            memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());

            TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
    }
    else
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("新建文件夹")
                             , QString::fromLocal8Bit("新文件夹名字不能为空"));
    }
}

void Book::flushFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();    //获得当前路径
    PDU *pdu = mkPDU(strCurPath.size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    strncpy((char*)(pdu->caMsg), strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::delDir()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("删除文件")
                             , QString::fromLocal8Bit("请选择要删除的文件"));
    }
    else
    {
        QString strDelName = pItem->text();
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_REQUEST;
        strncpy(pdu->caData, strDelName.toStdString().c_str(), strDelName.size());  //拷贝需要删除的名字
        memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

void Book::renameFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("重命名文件")
                             , QString::fromLocal8Bit("请选择要重命名的文件"));
    }
    else
    {
        QString strOldName = pItem->text(); //旧文件名
        QString strNewName = QInputDialog::getText(this, QString::fromLocal8Bit("重命名文件")
                                                   , QString::fromLocal8Bit("请输入新的文件名"));
        if (!strNewName.isEmpty())
        {
            PDU *pdu = mkPDU(strCurPath.size()+1);  //当前路径
            pdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_REQUEST;
            strncpy(pdu->caData, strOldName.toStdString().c_str(), strOldName.size());
            strncpy(pdu->caData+32, strNewName.toStdString().c_str(), strNewName.size());
            memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());   //当前路径

            TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
        else
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("重命名文件")
                                 , QString::fromLocal8Bit("新文件名不能为空"));
        }
    }
}

void Book::enterDir(const QModelIndex &index)
{
    QString strDirName = index.data().toString();
    m_strEnterDir = strDirName;//保存进入的目录
    QString strCurPath = TcpClient::getInstance().curPath();//当前路径
    PDU *pdu = mkPDU(strCurPath.size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
    strncpy(pdu->caData, strDirName.toStdString().c_str(), strDirName.size()); //将要进入的路径拷贝到caData
    memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::returnPre()
{
    QString strCurPath = TcpClient::getInstance().curPath(); //获得当前路径
    QString strRootPath = "./"+TcpClient::getInstance().loginName();//最顶层路径
    qDebug() << strCurPath << strRootPath;
    if (strCurPath == strRootPath)//已经在最顶层目录
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("返回")
                             , QString::fromLocal8Bit("返回失败：已经在最开始的文件夹目录中"));
    }
    else
    { //  "./aa/bb/cc"  -->  "./aa/bb"
        int index = strCurPath.lastIndexOf('/'); //从后往前找斜杠
        strCurPath.remove(index, strCurPath.size()-index);//删除=总长度-最后一个斜杠位置
        qDebug () << "return --> " << strCurPath;
        TcpClient::getInstance().setCurPath(strCurPath);   //新的当前路径

        clearEnterDir();

        flushFile();
    }
}

void Book::delRegFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, "删除文件", "请选择要删除的文件");
    }
    else
    {
        QString strDelName = pItem->text();
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_REQUEST;
        strncpy(pdu->caData, strDelName.toStdString().c_str(), strDelName.size());
        memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

void Book::uploadFile()
{
    m_strUploadFilePath = QFileDialog::getOpenFileName();//获得上传文件，弹出一个窗口，选择文件
    qDebug() << m_strUploadFilePath;    //返回文件的绝对路径
    if (!m_strUploadFilePath.isEmpty())
    {
        //  aa/bb/cc  5  8
        int index = m_strUploadFilePath.lastIndexOf('/');
        QString strFileName = m_strUploadFilePath.right(m_strUploadFilePath.size()-index-1);//不要斜杠
        qDebug() << strFileName;

        QFile file(m_strUploadFilePath);
        qint64 fileSize = file.size();  //获得文件大小

        QString strCurPath = TcpClient::getInstance().curPath(); //当前路径
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
        memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
        sprintf(pdu->caData, "%s %lld", strFileName.toStdString().c_str(), fileSize);

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;

        m_pTimer->start(1000);//启动定时器
    }
    else
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("上传文件"),
                             QString::fromLocal8Bit("上传文件名字不能为空"));
    }
}

void Book::uploadFileData()
{
    m_pTimer->stop();
    QFile file(m_strUploadFilePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("上传文件")
                             , QString::fromLocal8Bit("打开文件失败"));
        return;
    }

    char *pBuffer = new char[4096]; //读写数据为4096的效率会比较高
    qint64 ret = 0;
    while (true)
    {
        ret = file.read(pBuffer, 4096);
        if (ret > 0 && ret <= 4096)
        {
            TcpClient::getInstance().getTcpSocket().write(pBuffer, ret);
        }
        else if (0 == ret)
        {
            break;
        }
        else
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("上传文件")
                                 , QString::fromLocal8Bit("上传文件失败:读文件失败"));
            break;
        }
    }
    file.close();
    delete []pBuffer;
    pBuffer = NULL;
}

void Book::downloadFile()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();   //名字
    if (NULL == pItem)
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("下载文件")
                             , QString::fromLocal8Bit("请选择要下载的文件"));
    }
    else
    {
        QString strSaveFilePath = QFileDialog::getSaveFileName();   //选择保存路径
        if (strSaveFilePath.isEmpty())
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("下载文件"),
                                 QString::fromLocal8Bit("请指定要保存的位置"));
            m_strSaveFilePath.clear();
        }
        else
        {
            m_strSaveFilePath = strSaveFilePath;
        }

        QString strCurPath = TcpClient::getInstance().curPath();
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;
        QString strFileName = pItem->text();
        strcpy(pdu->caData, strFileName.toStdString().c_str());
        memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        //m_pTimer->stop();
    }
}

void Book::shareFile()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("分享文件")
                             , QString::fromLocal8Bit("请选择要分享的文件"));
        return;
    }
    else
    {
        m_strShareFileName = pItem->text();
    }

    Friend *pFriend = OpeWidget::getInstance().getFriend();
    QListWidget *pFriendList = pFriend->getFriendList();
    ShareFile::getInstance().updateFriend(pFriendList);
    if (ShareFile::getInstance().isHidden())
    {
        ShareFile::getInstance().show();
    }
}

void Book::moveFile()
{
    QListWidgetItem *pCurItem = m_pBookListW->currentItem();
    if (NULL != pCurItem)
    {
        m_strMoveFileName = pCurItem->text();
        QString strCutPath = TcpClient::getInstance().curPath();
        m_strMoveFilePath = strCutPath+'/'+m_strMoveFileName;

        m_pSelectDirPB->setEnabled(true);
    }
    else
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("移动文件")
                             , QString::fromLocal8Bit("请选择要移动的文件"));
    }
}

void Book::selectDestDir()
{
    QListWidgetItem *pCurItem = m_pBookListW->currentItem();
    if (NULL != pCurItem)
    {
        QString strDestDir = pCurItem->text();
        QString strCutPath = TcpClient::getInstance().curPath();
        m_strDestDir = strCutPath+'/'+strDestDir;

        int srcLen = m_strMoveFilePath.size();
        int destLen = m_strDestDir.size();
        PDU *pdu = mkPDU(srcLen+destLen+2);
        pdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_REQUEST;
        sprintf(pdu->caData, "%d %d %s", srcLen, destLen, m_strMoveFileName.toStdString().c_str());

        memcpy(pdu->caMsg, m_strMoveFilePath.toStdString().c_str(), srcLen);
        memcpy((char*)(pdu->caMsg)+(srcLen+1), m_strDestDir.toStdString().c_str(), destLen);

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("移动文件")
                             , QString::fromLocal8Bit("请选择要移动的文件"));
    }
    m_pSelectDirPB->setEnabled(false);
}

