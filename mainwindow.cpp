#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    refreshDeviceList();
    connect(ui->refreshListButton, SIGNAL(clicked()), this, SLOT(refreshDeviceList()));
    connect(ui->encryptButton, SIGNAL(clicked()), this, SLOT(encrypt()));
    connect(ui->decryptButton, SIGNAL(clicked()), this, SLOT(decrypt()));
    connect(ui->openButton, SIGNAL(clicked()), this, SLOT(openSelectedDir()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> MainWindow::detectDevices()
{
    //                                     letter   name   capacity  availible
    std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> pDevices(new std::vector<std::tuple<QString, QString, size_t, size_t>>);    
    DWORD dwDrives = GetLogicalDrives();
    std::wstring path = L"A:\\";
    while(dwDrives)
        {
            if(dwDrives & 1 && GetDriveType(path.c_str()) == DRIVE_REMOVABLE)
            {
                wchar_t label[MAX_PATH];           
                if(!GetVolumeInformationW(path.c_str(), label, sizeof(label), nullptr, nullptr, nullptr, nullptr, 0))
                {
                    std::wcerr << L"GetVolumeInformationW(" << path << L") error: " << GetLastError() << std::endl;
                    //TODO:: add exception
                }
                else
                {
                    ULARGE_INTEGER capacity, availible;
                    GetDiskFreeSpaceExW(path.c_str(), nullptr, &capacity, &availible);
                    pDevices->emplace_back(QString::fromStdWString(path), QString::fromWCharArray(label),
                                           capacity.QuadPart / (1024 * 1024), availible.QuadPart / (1024 * 1024));
                }
            }
            dwDrives = dwDrives >> 1;
            path[0]++;
        }
    pDevices->emplace_back("D:\\Users\\Ogare\\Desktop\\TestFolder\\", "Test folder", 0, 0);
    return pDevices;
}

void MainWindow::fillDeviceList(std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> pDevices)
{
    for(size_t i = 0; i < pDevices->size(); i++)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        QString name = std::get<1>(pDevices->at(i));
        if(name.isEmpty())
            name = "Съемный диск";
        item->setData(0, Qt::DisplayRole, name + " (" + static_cast<QString>(std::get<0>(pDevices->at(i))).left(2) + ")");
        item->setData(1, Qt::DisplayRole, QString::number(std::get<2>(pDevices->at(i))) + "(" + QString::number(std::get<3>(pDevices->at(i))) + ") MB");
        item->setData(2, Qt::DisplayRole, "false");
        item->setData(0, Qt::UserRole, std::get<0>(pDevices->at(i)));
        ui->deviceList->addTopLevelItem(item);
    }
}

void MainWindow::refreshDeviceList()
{
    ui->deviceList->clear();
    fillDeviceList(detectDevices());
    ui->deviceList->resizeColumnToContents(0);
    ui->deviceList->header()->resizeSection(0, ui->deviceList->header()->sectionSize(0) + 20);
}

void MainWindow::openSelectedDir()
{
    QTreeWidgetItem *item = ui->deviceList->selectedItems().front();
    QString path = item->data(0, Qt::UserRole).toString();
    QDesktopServices::openUrl(QUrl("file:///" + item->data(0, Qt::UserRole).toString()));
}

void MainWindow::encrypt()
{
    QTreeWidgetItem *item = ui->deviceList->selectedItems().front();
    ProgressBarDialog *pb = new ProgressBarDialog();
    connect(&combiner, SIGNAL(fileEncrypted()), pb, SLOT(addOne()));
    connect(&combiner, SIGNAL(filesCounted(int)), pb, SLOT(setup(int)));
    QtConcurrent::run(&combiner, &Combiner::combineReverse, QString(item->data(0, Qt::UserRole).toString()));
    //QtConcurrent::run(&combiner, &Combiner::combine, QString("G:\\"));
}

void MainWindow::decrypt()
{
    QTreeWidgetItem *item = ui->deviceList->selectedItems().front();
    QtConcurrent::run(&combiner, &Combiner::separateReverse, QString(item->data(0, Qt::UserRole).toString()));
}
