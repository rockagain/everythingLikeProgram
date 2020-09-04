#include "mydatabase.h"
/**
 * @brief MyDatabase::MyDatabase  这是一个has a 类   里面进行多种操作，注意 database 类的用法，看例子写代码就运行成功了，要么就用不对啊
 * @param name
 */
MyDatabase::MyDatabase(QString name)
{
    this->DBName = name;
    createDataBase();
}

//建立数据库连接
bool MyDatabase::createDataBase()
{
    db = QSqlDatabase::database();

    db = QSqlDatabase::addDatabase("QSQLITE");
    qDebug()<<db.isValid();

    db.setDatabaseName(this->DBName);
    db.setUserName("123");
    db.setPassword("123");
    if(!db.open()) {
        qDebug()<< QStringLiteral("数据库没打开~!!!!!!!!!!!!!!!!!!!!!!!");
        qDebug()<< db.lastError();
        return false;
    }
    query = new QSqlQuery(db);
    //qDebug()<< query->exec("CREATE TABLE tb_book (filecode varchar(64),fullname varchar(64),filename varchar(64),filepath varchar(1024),isFininshedOrNot varchar(32),hocr varchar(1024))");
    query->exec("CREATE TABLE tb_book (fileName varchar(128),filePath varchar(256))");
    return true;
}

/**
 * @brief query一下，看看数据库是不是空的，假如是空的，editboard那边网络申请，要是不为空那就把信息提取出来，放在侧栏上
 * @return
 */
bool MyDatabase::isDataBaseEmptyOrNot()
{
    qDebug()<< query->exec("SELECT * FROM tb_book");
    return !query->first();
}


















