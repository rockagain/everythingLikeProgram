#include "checkfileclass.h"

using namespace std;

#define NORMAL 0
#define FUZZ   1
#define REG    2


checkFileClass::checkFileClass()
{


    letsGo();
    emit itsOK();

}



//判断是否是NTFS盘
bool checkFileClass::isNTFS(string path){//"C:/"
    char sysNameBuf[MAX_PATH];
    int status = GetVolumeInformationA(path.c_str(),
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       sysNameBuf,
                                       MAX_PATH);

    if (0 != status){
        if (0 == strcmp(sysNameBuf, "NTFS")){
            //printf(" 文件系统名 : %s\n", sysNameBuf);
            qDebug() << "盘符：" << path.c_str() << "\n文件系统名：" << sysNameBuf;
            return true;
        }
        else {
            qDebug() <<" 该驱动盘非 NTFS 格式 \n";
            return false;
        }

    }
    return false;
}

/**
  * step 02. 获取驱动盘句柄
  */
HANDLE checkFileClass::getHandle(string volName){

    char fileName[MAX_PATH];
    fileName[0] = '\0';

    // 传入的文件名必须为\\.\C:的形式
    strcpy_s(fileName, "\\\\.\\");
    strcat_s(fileName, volName.c_str());
    // 为了方便操作，这里转为string进行去尾
    string fileNameStr = (string)fileName;
    fileNameStr.erase(fileNameStr.find_last_of(":") + 1);

    qDebug()<<"驱动盘地址: "<< fileNameStr.data();

    // 调用该函数需要管理员权限   //CreateFileA
    //Creates or opens a file or I/O device.
    //The most commonly used I/O devices are as follows:
    //file, file stream, directory, physical disk, volume, c
    //onsole buffer, tape drive, communications resource, mailslot,
    //and pipe. The function returns a handle that can be used
    //to access the file or device for various types of I/O depending on
    //the file or device and the flags and attributes specified.
    HANDLE hVol = CreateFileA(fileNameStr.data(),
                              GENERIC_READ | GENERIC_WRITE, // 可以为0
                              FILE_SHARE_READ | FILE_SHARE_WRITE, // 必须包含有FILE_SHARE_WRITE
                              NULL, // 这里不需要
                              OPEN_EXISTING, // 必须包含OPEN_EXISTING, CREATE_ALWAYS可能会导致错误
                              FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL可能会导致错误
                              NULL); // 这里不需要

    if (INVALID_HANDLE_VALUE != hVol){
        //getHandleSuccess = true;
        qDebug() << "获取驱动盘句柄成功！\n";
        return hVol;
    }
    else{
        qDebug()<< "获取驱动盘句柄失败 —— handle:" <<hVol <<"error :"<< GetLastError();
        return 0;
    }
    return 0;
}

/**
  * step 03. 初始化USN日志文件
  */
bool checkFileClass::createUSN(HANDLE hVol, CREATE_USN_JOURNAL_DATA& cujd){
    DWORD br;
    cujd.MaximumSize = 0; // 0表示使用默认值
    cujd.AllocationDelta = 0; // 0表示使用默认值
    bool status = DeviceIoControl(hVol,
                                  FSCTL_CREATE_USN_JOURNAL,
                                  &cujd,
                                  sizeof(cujd),
                                  NULL,
                                  0,
                                  &br,
                                  NULL);

    if (0 != status){
        //initUsnJournalSuccess = true;
        return true;
    }
    else{
        qDebug()<< "初始化USN日志文件失败 —— "<<status<<"error :"<< GetLastError();
        return false;
    }
    return false;
}

/**

 * step 04. 获取USN日志基本信息(用于后续操作)

 * msdn:http://msdn.microsoft.com/en-us/library/aa364583%28v=VS.85%29.aspx

 */
bool checkFileClass::getUSNInfo(HANDLE hVol, USN_JOURNAL_DATA& ujd){
    bool getBasicInfoSuccess = false;
    DWORD br;
    bool status = DeviceIoControl(hVol,
                                  FSCTL_QUERY_USN_JOURNAL,
                                  NULL,
                                  0,
                                  &ujd,
                                  sizeof(ujd),
                                  &br,
                                  NULL);
    if (0 != status){
        getBasicInfoSuccess = true;
        qDebug()<<"获取USN日志基本信息成功\n";
        return true;
    }
    else{
        qDebug()<<"获取USN日志基本信息失败 —— "<<status<<"error: ", GetLastError();
        return false;
    }
    return false;
}

bool checkFileClass::getUSNJournal(HANDLE hVol, USN_JOURNAL_DATA& ujd){
    //MFT_ENUM_DATA_V0 med;
    MFT_ENUM_DATA med;
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0 ; //ujd.FirstUsn;  要赋零才可以
    med.HighUsn = ujd.NextUsn;
#define BUF_LEN 4096
    CHAR buffer[BUF_LEN]; // 用于储存记录的缓冲 , 尽量足够地大
    DWORD usnDataSize = 0;
    PUSN_RECORD UsnRecord;
    //建立一个这样的结构体来吧hashtable 分开，因为估计每个盘里的数字有重叠，那样的话就对不上了，每个盘一套 引用数，里面应该有重叠
    singleTableAndFindParent singleTable;
    //每个盘都清空一次
    table.clear();
    findParent.clear();

    while (0 != DeviceIoControl(hVol,
                                FSCTL_ENUM_USN_DATA,
                                &med,
                                sizeof (med),
                                buffer,
                                BUF_LEN,
                                &usnDataSize,
                                NULL))
    {
        DWORD dwRetBytes = usnDataSize - sizeof (USN);
        // 找到第一个 USN 记录
        // from MSDN(http://msdn.microsoft.com/en-us/library/aa365736%28v=VS.85%29.aspx ):
        // return a USN followed by zero or more change journal records, each in a USN_RECORD structure.
        UsnRecord = (PUSN_RECORD)(((PCHAR)buffer) + sizeof (USN));
        //printf(" ********************************** \n");
        while (dwRetBytes>0){
            // 打印获取到的信息
            static int i = 0;
            const int strLen = UsnRecord->FileNameLength;

            char fileName[MAX_PATH] = { 0 };
            WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);
            //把这个禁用了提速到10秒 左右
            //printf("%d: %s\n", i,fileName);

            i++;
            //if(i>30000)     //为了调试搞的
            //goto ww;     //为了调试写的
            //printf("FileReferenceNumber: %d\n", UsnRecord->FileReferenceNumber);
            //printf("ParentFileReferenceNumber: %d\n", UsnRecord->ParentFileReferenceNumber);

            QString thefileName = QString::fromLocal8Bit(fileName);
            fileInfo info;
            info.ownInt = UsnRecord->FileReferenceNumber;
            info.fatherInt = UsnRecord->ParentFileReferenceNumber;
            info.rootDir = this->rootDir;

            table.insert(thefileName,info);
            selfNameAndParentNode nameAndNode;
            nameAndNode.selfName = QString::fromLocal8Bit(fileName);
            nameAndNode.parentNode = UsnRecord->ParentFileReferenceNumber;
            findParent.insert(info.ownInt,nameAndNode);
            // 获取下一个记录
            DWORD recordLen = UsnRecord->RecordLength;
            dwRetBytes -= recordLen;
            UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + recordLen);
        }
        // 获取下一页数据， MTF 大概是分多页来储存的吧？
        // from MSDN(http://msdn.microsoft.com/en-us/library/aa365736%28v=VS.85%29.aspx ):
        // The USN returned as the first item in the output buffer is the USN of the next record number to be retrieved.
        // Use this value to continue reading records from the end boundary forward.
        med.StartFileReferenceNumber = *(USN *)&buffer;
    }
    //给这个结构体赋值
    singleTable.table = table;
    singleTable.findParent = findParent;
    singleTable.key = this->rootDir;
    //把内容添加到everyTable里面
    everyTable.append(singleTable);
    //ww:  //调试用的
    return true;
}

/**
 * step 06. 删除 USN 日志文件 ( 当然也可以不删除 )
 */
bool checkFileClass::deleteUSN(HANDLE hVol, USN_JOURNAL_DATA& ujd){
    DELETE_USN_JOURNAL_DATA dujd;
    dujd.UsnJournalID = ujd.UsnJournalID;
    dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;
    DWORD br;
    int status = DeviceIoControl(hVol,
                                 FSCTL_DELETE_USN_JOURNAL,
                                 &dujd,
                                 sizeof (dujd),
                                 NULL,
                                 0,
                                 &br,
                                 NULL);
    if (0 != status){
        CloseHandle(hVol);
        printf(" 成功删除 USN 日志文件 !\n");
        return true;
    }
    else {
        CloseHandle(hVol);
        printf(" 删除 USN 日志文件失败 —— status:%x error:%d\n", status, GetLastError());
        return false;
    }
    return false;
}


void checkFileClass::letsGo(){

    this->rootDir = "E:/";
    database = new MyDatabase("everyThing_.db");

    //把所有有效的盘符列出来
    checkVolumeName();
    //把所有的信息都保存在结构体里
    everyVolumeGo();
    //把所有的信息都保存在数据库里
    saveEverythinToDB();

}

void checkFileClass::showFilePathAndName(QString name,int type)
{
    //三种不同的手段
        if(type == NORMAL){
            showFilePathAndNameNormal(name);
        } else if(type == FUZZ){
            showFilePathAndNameFuzz(name);
        } else if(type == REG){
            showFilePathAndNameReg(name);
        }

    //之前是读的  数据结构里的， 现在不这么处理了， 现在是只要是把所有信息得到后，就全部保存到数据库里，然后在去进行搜索

}


//查找每一个有效的盘符，并把 盘符 和 配对的句柄 联合着 存起来
void checkFileClass::checkVolumeName()
{
    //这些可以把这些放在别的地方了
    QStringList fullVolumeNames;
    fullVolumeNames<<"A:/"<<"B:/"<<"C:/"<<"D:/"<<"E:/"<<
                     "F:/"<<"G:/"<<"H:/"<<"I:/"<<"J:/"<<
                     "K:/"<<"M:/"<<"L:/"<<"N:/"<<"O:/"<<
                     "P:/"<<"Q:/"<<"R:/"<<"S:/"<<"T:/"<<
                     "U:/"<<"V:/"<<"W:/"<<"X:/"<<"Y:/"<<
                     "Z:/";

    QVector<QString> VolumeName;
    for(QString i :fullVolumeNames) {
        //判断是否能返回
        if(isNTFS(i.toStdString())){
            VolumeName.append(i);
            qDebug()<<i;
        }
    }
    HANDLE hVol;
    for(QString i : VolumeName) {
        hVol = getHandle(i.toStdString());
        if(hVol != 0) {
            VolumeNameAndHandle.insert(i,hVol);
        } else {
            qDebug()<<"句柄没有读取到";
        }
    }
    ////////////////////上面是把所有的盘都读进去///////////////////////////
}


//把所有有用的数据都存进结构体内
void checkFileClass::everyVolumeGo()
{
    QList<QString> keys = VolumeNameAndHandle.keys();
    //用到的结构体
    for(QString key: keys) {
        CREATE_USN_JOURNAL_DATA* cujd = new CREATE_USN_JOURNAL_DATA;
        USN_JOURNAL_DATA* ujd = new USN_JOURNAL_DATA;

        this->rootDir = key;
        HANDLE hVol = VolumeNameAndHandle.value(key);
        createUSN(hVol, *cujd);
        getUSNInfo(hVol, *ujd);
        // 在这个函数里面把所有信息填进去了
        getUSNJournal(hVol, *ujd);
    }
}


void checkFileClass::showFilePathAndNameNormal(QString name)
{
    //在循环内处理信息 ,每个盘里都有他的一套hashTable，否则内部数字重叠，导致结果出错误
    for(auto singleTable: everyTable) {
        //要返回所有名字为这个的文件来,可以返回一个或者多个
        //fileInfo info =  table.value(name);     //无重名模式
        QList<fileInfo> in_fo =  singleTable.table.values(name);   //重名模式

        for(fileInfo info :in_fo) {
            QString filePath = name;
            int upperNumber = info.fatherInt;
            while(upperNumber != 5)
            {
                selfNameAndParentNode nameAndNode =  singleTable.findParent.value(upperNumber);
                filePath = nameAndNode.selfName + "/" + filePath;
                upperNumber = nameAndNode.parentNode;
            }
            filePath = info.rootDir +  filePath;
            //qDebug()<<filePath;
            //qDebug()<<filePath;//.toStdString().c_str();
            //            if(filePath.contains(info.rootDir + "Windows"))
            //                continue;
            finalAnswer.append(filePath);
        }

    }
    qDebug()<<"+++++++++++++++++++++++++++++++++++++++++++++++";
    QString text;
    int num =0;
    for(auto i :finalAnswer) {
        if(text == "")
            text = text  + i;
        else {
            text = text + "\n" + i;
        }
        num = num + 1;
    }
    qDebug() <<num;
    emit toShow(text);
}

void checkFileClass::showFilePathAndNameFuzz(QString name)
{
    //由normal mode 改的
    QStringList manyNames;
    //把所有的名字都放进来
    if(name == "")
        return;
    if(name == " ")
        return;
    if(name == "  ")
        return;
    if(name == ".")
        return;
    if(name.size()<=2)
        return;

    for(auto singleTable: everyTable) {
        for(auto whole_name:singleTable.table.keys()) {
            if(whole_name.contains(name)) {
                manyNames.append(whole_name);
            }
        }
    }

    for(QString single_name:manyNames) {
        //
        for(auto singleTable: everyTable) {
            //要返回所有名字为这个的文件来,可以返回一个或者多个
            QList<fileInfo> in_fo =  singleTable.table.values(single_name);   //重名模式

            for(fileInfo info :in_fo) {
                QString filePath = single_name;
                int upperNumber = info.fatherInt;
                while(upperNumber != 5)
                {
                    selfNameAndParentNode nameAndNode =  singleTable.findParent.value(upperNumber);
                    filePath = nameAndNode.selfName + "/" + filePath;
                    upperNumber = nameAndNode.parentNode;
                }
                filePath = info.rootDir + filePath;
                if(filePath.contains(info.rootDir + "Windows"))  //把这个盘排除了，太多没用的信息了
                    continue;
                finalAnswer.append(filePath);
            }

        }

    }
    qDebug()<<"+++++++++++++++++++++++++++++++++++++++++++++++";
    QString text;
    for(auto i :finalAnswer) {
        if(text != "")
            text = text + "\n"+ i;
        else
            text = text + i;
    }
    emit toShow(text);
}


void checkFileClass::showFilePathAndNameReg(QString name)
{
    //由normal mode 改的
    QStringList manyNames;
    //把所有的名字都放进来
    if(name == "")
        return;
    if(name == " ")
        return;
    if(name == "  ")
        return;
    if(name == ".")
        return;
    if(name.size()<=2)
        return;

    QRegExp reg(name);
    reg.setCaseSensitivity(Qt::CaseInsensitive);

    for(auto singleTable: everyTable) {
        for(QString whole_name:singleTable.table.keys()) {
            if(reg.exactMatch(whole_name)) {
                manyNames.append(whole_name);
            }
        }
    }

    for(QString single_name:manyNames) {
        //
        for(auto singleTable: everyTable) {
            //要返回所有名字为这个的文件来,可以返回一个或者多个
            QList<fileInfo> in_fo =  singleTable.table.values(single_name);   //重名模式

            for(fileInfo info :in_fo) {
                QString filePath = single_name;
                int upperNumber = info.fatherInt;
                while(upperNumber != 5)
                {
                    selfNameAndParentNode nameAndNode =  singleTable.findParent.value(upperNumber);
                    filePath = nameAndNode.selfName + "/" + filePath;
                    upperNumber = nameAndNode.parentNode;
                }
                filePath = info.rootDir + filePath;
                //if(filePath.contains(info.rootDir + "Windows"))  //把这个盘排除了，太多没用的信息了
                //continue;
                finalAnswer.append(filePath);
            }

        }

    }
    qDebug()<<"+++++++++++++++++++++++++++++++++++++++++++++++";
    QString text;
    for(auto i :finalAnswer) {
        if(text != "")
            text = text + "\n"+ i;
        else
            text = text + i;
    }
    emit toShow(text);
}

void checkFileClass::saveEverythinToDB()
{
//    //在循环内处理信息 ,每个盘里都有他的一套hashTable，否则内部数字重叠，导致结果出错误
//    for(auto singleTable: everyTable) {
//        //要返回所有名字为这个的文件来,可以返回一个或者多个
//        //fileInfo info =  table.value(name);     //无重名模式
//        QList<fileInfo> in_fo =  singleTable.table.values(name);   //重名模式

//        for(fileInfo info :in_fo) {
//            QString filePath = name;
//            int upperNumber = info.fatherInt;
//            while(upperNumber != 5)
//            {
//                selfNameAndParentNode nameAndNode =  singleTable.findParent.value(upperNumber);
//                filePath = nameAndNode.selfName + "/" + filePath;
//                upperNumber = nameAndNode.parentNode;
//            }
//            filePath = info.rootDir +  filePath;
//            //qDebug()<<filePath;
//            //qDebug()<<filePath;//.toStdString().c_str();
//            //            if(filePath.contains(info.rootDir + "Windows"))
//            //                continue;
//            finalAnswer.append(filePath);
//        }

//    }
}




