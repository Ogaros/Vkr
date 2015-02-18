#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <windows.h>
#include <memory>
#include <QString>
#include <QtConcurrent/QtConcurrent>
#include <tuple>
#include <iostream>
#include <Combiner.h>
#include <progressbardialog.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);    
    ~MainWindow();

public slots:
    void refreshDeviceList();
    void encrypt();
    void decrypt();

private:
    std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> detectDevices();
    void fillDeviceList(std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> pDevices);

    Ui::MainWindow *ui;
    Combiner combiner;
};

#endif // MAINWINDOW_H
