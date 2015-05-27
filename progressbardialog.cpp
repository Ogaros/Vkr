#include "progressbardialog.h"
#include "ui_progressbardialog.h"

ProgressBarDialog::ProgressBarDialog(const dialogType type, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressBarDialog), type(type)
{
    ui->setupUi(this);    
    setWindowFlags(Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_DeleteOnClose);
}

ProgressBarDialog::~ProgressBarDialog()
{
    delete ui;
}

void ProgressBarDialog::addOne()
{
    if(type == Encryption && skippedFirstFile == false)
        skippedFirstFile = true;
    else
    {
        int value = ui->progressBar->value() + 1;
        if(value <= ui->progressBar->maximum())
            ui->progressBar->setValue(value);
    }
}

void ProgressBarDialog::setup(const int &maximum)
{
    ui->progressBar->setMaximum(maximum);
    ui->progressBar->setValue(0);
    ui->closeButton->setDisabled(true);
    skippedFirstFile = false;
    timer.start(500);
	elapsedTime.start();
    connect(&timer, SIGNAL(timeout()), this, SLOT(setTitleDots()));
	connect(&timer, SIGNAL(timeout()), this, SLOT(updateElapsedTime()));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
    show();
}

void ProgressBarDialog::finish()
{
    timer.stop();		
    type == Encryption ? setWindowTitle("Encryption finished") :
                         setWindowTitle("Decryption finished");
    MessageBeep(MB_OK);
    ui->closeButton->setEnabled(true);
}

void ProgressBarDialog::setTitleDots()
{
    static int dots = 0;
    type == Encryption ? setWindowTitle("Encrypting" + QString(dots, '.')) :
                         setWindowTitle("Decrypting" + QString(dots, '.'));
    dots > 2 ? dots = 0 : dots++;
}

void ProgressBarDialog::updateElapsedTime()
{	
	QTime t = QTime(0, 0, 0).addMSecs(elapsedTime.elapsed());
	ui->elapsedTimeLabel->setText("Elapsed Time: " + t.toString("mm:ss"));
}