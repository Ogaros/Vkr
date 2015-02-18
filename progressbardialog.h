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
    explicit ProgressBarDialog(QWidget *parent = 0);
    ~ProgressBarDialog();

public slots:
    void addOne();
    void setup(const int &maximum);

private slots:
    void setTitleDots();

private:
    Ui::ProgressBarDialog *ui;
    bool skippedFirstFile;
    QTimer timer;

};

#endif // PROGRESSBARDIALOG_H
