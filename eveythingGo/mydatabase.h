#ifndef MYDATABASE_H
#define MYDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QDebug>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>


class MyDatabase
{
public:
    MyDatabase(QString name);

    QSqlQuery* query;

    //检测数据库中是否还留有数据
    bool isDataBaseEmptyOrNot();

private:
    QString DBName;
    //   QFileInfo* file;


    QSqlDatabase db;
    
    bool createDataBase();

};

#endif // MYDATABASE_H














