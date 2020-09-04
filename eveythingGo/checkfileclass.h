#ifndef CHECKFILECLASS_H
#define CHECKFILECLASS_H

#include<Windows.h>
#include<iostream>
#include<stdio.h>
#include<string>
#include<tchar.h>
#include <QObject>
#include <QMap>
#include <QVector>
#include <QDebug>
#include <QStringList>
#include <WinIoCtl.h>
#include <map>
#include <QMultiHash>
#include <QHash>
#include <QObject>
#include <QString>
#include <QRegExp>

#include "mydatabase.h"


class checkFileClass : public QObject
{
    Q_OBJECT
public:
    checkFileClass();
    QList<QString> finalAnswer;

private:
    MyDatabase* database;
    QMap<QString,HANDLE> VolumeNameAndHandle; //盘符和对应的句柄
    void letsGo();
    bool isNTFS(std::string path);
    HANDLE getHandle(std::string volName);
    bool createUSN(HANDLE hVol, CREATE_USN_JOURNAL_DATA& cujd);
    bool getUSNInfo(HANDLE hVol, USN_JOURNAL_DATA& ujd);
    bool getUSNJournal(HANDLE hVol, USN_JOURNAL_DATA& ujd);
    bool deleteUSN(HANDLE hVol, USN_JOURNAL_DATA& ujd);
    //当前根目录
    QString rootDir;
    //fileInfo Object;
    struct fileInfo
    {
        int fatherInt;
        int ownInt;
        QString rootDir;
        //QString filePath;
        //int type; //先不管这个之后再对此进行运作
    };
    //本身当父节点的时候需要名字,自己的名字父节点的int
    struct selfNameAndParentNode
    {
        int parentNode;
        QString selfName;
    };

    //每一个盘都需要一个这样的结构体搞这么一个结构体数组
    struct singleTableAndFindParent
    {
        QString key;
        QMultiHash<QString,fileInfo> table;
        QHash<int,selfNameAndParentNode> findParent;

    };
    //每换一个盘就填写一个这个
    QList<singleTableAndFindParent> everyTable;

    QMultiHash<QString,fileInfo> table;
    //yourself name and upper nodes;
    QHash<int,selfNameAndParentNode> findParent; //kids father ,来通过节点找其包含路径
    //查找有用的盘符
    void checkVolumeName();
    //把每个盘的内容存到结构体内
    void everyVolumeGo();
signals:
    void toShow(QString location);
    void itsOK();
public slots:
    void showFilePathAndName(QString name,int type);

private:
    //三种不同的手段
    void showFilePathAndNameNormal(QString name);
    void showFilePathAndNameFuzz(QString name);
    void showFilePathAndNameReg(QString name);

    void saveEverythinToDB();
};

#endif // CHECKFILECLASS_H
