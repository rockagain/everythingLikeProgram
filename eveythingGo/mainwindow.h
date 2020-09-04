#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "checkfileclass.h"
#include <QAction>
#include <QMenu>
#include <QFileDialog>
#include <QFile>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void emitFindWhat();
    //显示条目的
    void listLocation(QString);
    void setFindOK();
    void saveFileAction();
private:
    Ui::MainWindow *ui;
    checkFileClass* checkFileObject ;
    //init 一下
    void init();
signals:
    void findWhat(QString what,int how);
};

#endif // MAINWINDOW_H
