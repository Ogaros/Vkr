#include "progressbardialog.h"
#include "ui_progressbardialog.h"

ProgressBarDialog::ProgressBarDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressBarDialog)
{
    ui->setupUi(this);    
    setAttribute(Qt::WA_DeleteOnClose);
}

ProgressBarDialog::~ProgressBarDialog()
{
    delete ui;
}

void ProgressBarDialog::addOne()
{
    if(skippedFirstFile == false)
        skippedFirstFile = true;
    else
    {
        int value = ui->progressBar->value() + 1;
        ui->progressBar->setValue(value);
        if(value == ui->progressBar->maximum())
        {
            timer.stop();
            setWindowTitle("Encryption finished");
            MessageBeep(MB_OK);
            ui->closeButton->setEnabled(true);
        }
    }
}

void ProgressBarDialog::setup(const int &maximum)
{
    ui->progressBar->setMaximum(maximum);
    ui->progressBar->setValue(0);
    ui->closeButton->setDisabled(true);
    skippedFirstFile = false;
    timer.start(500);
    connect(&timer, SIGNAL(timeout()), this, SLOT(setTitleDots()));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
    show();
}

void ProgressBarDialog::setTitleDots()
{
    static int dots = 0;
    setWindowTitle("Encrypting" + QString(dots, '.'));
    dots > 2 ? dots = 0 : dots++;
}
