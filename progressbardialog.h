#ifndef PROGRESSBARDIALOG_H
#define PROGRESSBARDIALOG_H

#include <QDialog>
#include <QTimer>
#include <windows.h>

namespace Ui {
class ProgressBarDialog;
}

class ProgressBarDialog : public QDialog
{
    Q_OBJECT

public:
    enum dialogType{Encryption, Decryption};
    explicit ProgressBarDialog(const dialogType type, QWidget *parent = 0);
    ~ProgressBarDialog();

public slots:
    void addOne();
    void setup(const int &maximum);
    void finish();

private slots:
    void setTitleDots();

private:
    Ui::ProgressBarDialog *ui;
    bool skippedFirstFile;
    QTimer timer;
    const dialogType type;
};

#endif // PROGRESSBARDIALOG_H
