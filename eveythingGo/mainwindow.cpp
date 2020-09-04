#include "mainwindow.h"
#include "ui_mainwindow.h"



#define NORMAL 0
#define FUZZ   1
#define REG    2

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    init();

}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::emitFindWhat(){
    ui->textEdit->clear();
    checkFileObject->finalAnswer.clear(); //把里面的东西清一清啊
    if(ui->radioButton->isChecked()){
        emit findWhat(ui->lineEdit->text(),NORMAL);
    } else if(ui->fuzz_radioButton->isChecked()) {
        emit findWhat(ui->lineEdit->text(),FUZZ);
    } else if(ui->reg_radioButton->isChecked()) {
        emit findWhat(ui->lineEdit->text(),REG);
    }
}


void MainWindow::listLocation(QString location)
{
    //QString currentText = ui->textEdit->toPlainText();
    //ui->textEdit->clear();
    //ui->textEdit->setPlainText(currentText + location + '\n');
    //    if(ui->textEdit->toPlainText() != "")
    //        ui->textEdit->append("\n");
    ui->textEdit->append(location);
}

void MainWindow::setFindOK()
{
    //ui->pushButton->setEnabled(true);
    //把框子设位可以操控状态
    qDebug()<<"*OK*";
}

void MainWindow::init()
{
    this->setWindowTitle("findWhat");
    ui->radioButton->setChecked(true);
    QMenu *fileMenu = new QMenu(QStringLiteral("文件"));
    this->ui->menuBar->addMenu(fileMenu);
    QAction *saveAction = new QAction(QStringLiteral("导出"));
    fileMenu->addAction(saveAction);
    connect(saveAction,&QAction::triggered,this,&MainWindow::saveFileAction);


    checkFileObject = new checkFileClass;
    connect(ui->lineEdit,&QLineEdit::textChanged,this,&MainWindow::emitFindWhat);
    connect(this,&MainWindow::findWhat,checkFileObject,&checkFileClass::showFilePathAndName);
    connect(checkFileObject,&checkFileClass::toShow,this,&MainWindow::listLocation);
    connect(checkFileObject,&checkFileClass::itsOK,this,&MainWindow::setFindOK);
}

void MainWindow::saveFileAction()
{
    QString filepath = QFileDialog::getSaveFileName(this, "选择保存文件路径", "默认文件名.txt");
    qDebug()<< filepath;
    QFile TheFile(filepath);
    TheFile.open(QIODevice::ReadWrite);
    QByteArray text;
    text.append(ui->textEdit->toPlainText());

    TheFile.write(text);
}


