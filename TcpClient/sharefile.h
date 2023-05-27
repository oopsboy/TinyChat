#ifndef SHAREFILE_H
#define SHAREFILE_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QScrollArea>
#include <QCheckBox>
#include <QListWidget>

class ShareFile : public QWidget
{
    Q_OBJECT
public:
    explicit ShareFile(QWidget *parent = nullptr);

    static ShareFile &getInstance();

    void test();

    void updateFriend(QListWidget *pFriendList);

signals:

public slots:
    void cancelSelect();
    void selectAll();

    void okShare();
    void cancelShare();

private:
    QPushButton *m_pSelectAllPB;//全选
    QPushButton *m_pCancelSelectPB;//反选

    QPushButton *m_pOKPB;//确定
    QPushButton *m_pCancelPB;//取消

    QScrollArea *m_pSA;//展示区
    QWidget *m_pFriendW;//展示
    QVBoxLayout *m_pFriendWVBL;//
    QButtonGroup *m_pButtonGroup;//管理好友
};

#endif // SHAREFILE_H
