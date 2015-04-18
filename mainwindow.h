#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <QMainWindow>
#include <vector>
#include <windows.h>
#include <memory>
#include <QString>
#include <QtConcurrent/QtConcurrent>
#include <QDesktopServices>
#include <QMessageBox>
#include <QFile>
#include <tuple>
#include <iostream>
#include <Combiner.h>
#include <progressbardialog.h>
#include <EncryptionAlgorithm/Gost.h>

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
    void openSelectedDir();
    void encrypt();
    void decrypt();

private slots:
    void changeButtonStatus();
    void refreshEncryptionStatus();

private:
    std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> detectDevices();
    void fillDeviceList(std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> pDevices);
    bool hasEncryptedContainer(const QString &devicePath);
    Ui::MainWindow *ui;
    Combiner combiner;
};

#endif // MAINWINDOW_H
