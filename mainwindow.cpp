#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	checkUSBSerial();
    ui->setupUi(this);
    refreshDeviceList();
    connect(ui->refreshListButton, SIGNAL(clicked()), this, SLOT(refreshDeviceList()));
    connect(ui->encryptButton, SIGNAL(clicked()), this, SLOT(encrypt()));
    connect(ui->decryptButton, SIGNAL(clicked()), this, SLOT(decrypt()));
    connect(ui->openButton, SIGNAL(clicked()), this, SLOT(openSelectedDir()));
    connect(ui->deviceList, SIGNAL(itemSelectionChanged()), this, SLOT(changeButtonStatus()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> MainWindow::detectDevices()
{
    //                                     letter   name   capacity  availible
    std::unique_ptr<std::vector<std::tuple<QString, QString, size_t, size_t>>> pDevices(new std::vector<std::tuple<QString, QString, size_t, size_t>>);    
	char thisDrive = QCoreApplication::applicationFilePath().at(0).toLatin1();
    DWORD dwDrives = GetLogicalDrives();
    std::wstring path = L"A:\\";
    while(dwDrives)
        {
            if(dwDrives & 1 && GetDriveType(path.c_str()) == DRIVE_REMOVABLE && path.c_str()[0] != thisDrive)
            {
				wchar_t label[MAX_PATH];
				if (!GetVolumeInformationW(path.c_str(), label, sizeof(label), nullptr, nullptr, nullptr, nullptr, 0))
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
        // device name
        item->setData(0, Qt::DisplayRole, name + " (" + static_cast<QString>(std::get<0>(pDevices->at(i))).left(2) + ")");
        // device capacity(availible size)
        item->setData(1, Qt::DisplayRole, QString::number(std::get<2>(pDevices->at(i))) + "(" + QString::number(std::get<3>(pDevices->at(i))) + ") MB");        
        // is encrypted?
        item->setData(2, Qt::DisplayRole, hasEncryptedContainer(std::get<0>(pDevices->at(i))));
        // device letter (D:\\)
        item->setData(0, Qt::UserRole, std::get<0>(pDevices->at(i)));
        // device capacity
        item->setData(1, Qt::UserRole, std::get<2>(pDevices->at(i)));
        ui->deviceList->addTopLevelItem(item);
    }
}

bool MainWindow::hasEncryptedContainer(const QString &devicePath)
{
    return QFile::exists(devicePath + Combiner::getContainerName());
}

void MainWindow::refreshDeviceList()
{
    ui->deviceList->clear();
    fillDeviceList(detectDevices());
    ui->deviceList->resizeColumnToContents(0);
    // Add some space between device name and capacity
    ui->deviceList->header()->resizeSection(0, ui->deviceList->header()->sectionSize(0) + 20);
    ui->encryptButton->setEnabled(false);
    ui->decryptButton->setEnabled(false);
    ui->openButton->setEnabled(false);
}

void MainWindow::openSelectedDir()
{
    QTreeWidgetItem *item = ui->deviceList->selectedItems().front();
    QDesktopServices::openUrl(QUrl("file:///" + item->data(0, Qt::UserRole).toString()));
}

void MainWindow::encrypt()
{
    QTreeWidgetItem *item = ui->deviceList->selectedItems().front();    
    ProgressBarDialog *pb = new ProgressBarDialog(ProgressBarDialog::Encryption);
    connect(&combiner, SIGNAL(fileProcessed()), pb, SLOT(addOne()));
    connect(&combiner, SIGNAL(filesCounted(int)), pb, SLOT(setup(int)));
    connect(&combiner, SIGNAL(processingFinished()), pb, SLOT(finish()));
    connect(&combiner, SIGNAL(processingFinished()), this, SLOT(refreshEncryptionStatus()));
    QtConcurrent::run(&combiner, &Combiner::combine, QString(item->data(0, Qt::UserRole).toString()));
}

void MainWindow::decrypt()
{
    QTreeWidgetItem *item = ui->deviceList->selectedItems().front();    
    ProgressBarDialog *pb = new ProgressBarDialog(ProgressBarDialog::Decryption);
    connect(&combiner, SIGNAL(fileProcessed()), pb, SLOT(addOne()));
    connect(&combiner, SIGNAL(filesCounted(int)), pb, SLOT(setup(int)));
    connect(&combiner, SIGNAL(processingFinished()), pb, SLOT(finish()));
    connect(&combiner, SIGNAL(processingFinished()), this, SLOT(refreshEncryptionStatus()));
    QtConcurrent::run(&combiner, &Combiner::separate, QString(item->data(0, Qt::UserRole).toString()));
}

void MainWindow::changeButtonStatus()
{
    if(ui->deviceList->selectedItems().size() == 0)
        return;
    auto item = ui->deviceList->selectedItems().first();
    if(item->data(2, Qt::DisplayRole).toBool())
    {
        ui->encryptButton->setEnabled(false);
        ui->decryptButton->setEnabled(true);
    }
    else
    {
        ui->encryptButton->setEnabled(true);
        ui->decryptButton->setEnabled(false);
    }
    ui->openButton->setEnabled(true);
}

void MainWindow::refreshEncryptionStatus()
{
    auto item = ui->deviceList->selectedItems().first();
    item->setData(2, Qt::DisplayRole, hasEncryptedContainer(item->data(0, Qt::UserRole).toString()));
    changeButtonStatus();
}

void MainWindow::checkUSBSerial()
{
	QDir dir;
	QByteArray serialNumber(usbAdapter::getSerialNumber(dir.absolutePath().at(0).toLatin1()));	

	QFile f("./" + Combiner::getKeyFileName());
	f.open(QFile::ReadOnly);
	f.seek(32);
	QByteArray encryptedSerial(f.readAll());
	f.close();

	Gost g;
	g.setKey(Combiner::getBaseKey());
	g.simpleDecrypt(encryptedSerial);
	int index = encryptedSerial.indexOf('\0');
	if (index > 0)
		encryptedSerial.resize(index);
	if (encryptedSerial != serialNumber)
	{
		QMessageBox box;
		box.setIcon(QMessageBox::Critical);
		box.setText("You are trying to run this encryptor copy from the wrong usb device");
		box.setStandardButtons(QMessageBox::Cancel);
		box.setModal(Qt::ApplicationModal);
		MessageBeep(MB_OK);
		box.exec();
		exit(EXIT_FAILURE);
	}
}
