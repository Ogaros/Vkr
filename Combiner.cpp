#include "Combiner.h"

const QString Combiner::containerName = "Encrypted.data";
const int Combiner::batchSize = 256 * 1024 * 1024; // 256 MB

Combiner::Combiner(QObject *parent) : QObject(parent)
{
    algorithm.setKey("h47skro;,sng89o3sy6ha2qwn89sk.er");
}

Combiner::~Combiner()
{

}

void Combiner::fillFileList(const QString &path)
{
    QString sPath = path + "*.*";
    std::unique_ptr<WCHAR[]> search_path(new WCHAR[sPath.size() + 1]);
    sPath.toWCharArray(search_path.get());
    search_path[sPath.size()] = '\0';
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(search_path.get(), &fd);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            QString name = QString::fromWCharArray(fd.cFileName);
            if(name != "." && name != "..")
            {
                if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {                    
                    fillFileList(path + name + "\\");

                }
                else
                {
                    qint64 size = (fd.nFileSizeHigh * (MAXDWORD + 1)) + fd.nFileSizeLow;
                    QString noDrivePath = path;
                    noDrivePath.replace(noDrivePath.indexOf(devicePath), devicePath.size(), "");
                    fileList.emplace_back(noDrivePath, name, size, 0);
                }
            }
        }
        while(FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
}

void Combiner::combine(const QString &path)
{
    fileList.clear();
    devicePath = path;
    fillFileList(devicePath);
    if(fileList.empty())
        throw std::runtime_error("No files to encrypt");
    emit filesCounted(fileList.size());

    currentFileIter = fileList.begin();

    QFile containerFile(devicePath + containerName);
    if(!containerFile.open(QIODevice::WriteOnly))
        throw std::runtime_error("Failed to open container file");

    QByteArray batch;
    currentFile.reset(nullptr);
    QByteArray initVector = algorithm.generateInitVector();
    try
    {
        do
        {
            batch = getBatch(batchSize);
            encryptBatch(batch);
            containerFile.write(batch);
        }
        while(batch.size() == batchSize);
    }
    catch(...){throw;}

    qint64 xmlSize = encryptXml(containerFile);

    batch.setNum(xmlSize);
    if(batch.size() < 8) // xml size takes 8 bytes
        batch.prepend(QByteArray(8 - batch.size(), '0'));
	qDebug() << algorithm.gamma;
	encryptBatch(batch);
    containerFile.write(batch);

    containerFile.write(initVector);

    containerFile.flush();
    containerFile.close();
    emit processingFinished();
}

void Combiner::separate(const QString &path)
{
    devicePath = path;
    fileList.clear();
    QFile containerFile(devicePath + containerName);
    if(!containerFile.open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open container file");

    QByteArray batch;
    int blockSize = algorithm.getBlockSize();

    batch = getBatchFromContainer(blockSize, containerFile);
    algorithm.setInitVector(batch);
    algorithm.setupGamma(containerFile.size(), batchSize);

    decryptXmlAndFillFileList(containerFile);
	if (containerFile.size() % batchSize)
	{
		batch = getBatchFromContainer(containerFile.size() % batchSize, containerFile);
		decryptBatch(batch);
	}
	else
		batch.clear();
		
    QDir dir(devicePath);
    for(currentFileIter = fileList.begin(); currentFileIter != fileList.end(); currentFileIter++)
    {
        restoreFile(batch, containerFile, dir);
    }
    containerFile.remove();
    emit processingFinished();
}

QByteArray Combiner::getBatch(const int &size)
{
    QByteArray batch;
    if(currentFile == nullptr)
    {
        try {openCurrentFile();}
        catch(...){throw;}
    }
    qint64 fileSize = currentFile->size();
    qDebug() << size << " - " << currentFile->size() << " - " << currentFileIter->name;

    if(fileSize < size)
    {        
        emit fileProcessed();
        currentFileIter->firstBlockSize = fileSize;
        currentFile->seek(0);
        batch = currentFile->read(fileSize);
        while(currentFile->exists())
            currentFile->remove();

        currentFile.reset(nullptr);
        removeCurrentFileDir();

        currentFileIter++;
        if(currentFileIter != fileList.end())        
			batch.append(getBatch(size - batch.size()));
    }
    else
    {
        qint64 newSize = fileSize - size;
        currentFile->seek(newSize);
        batch = currentFile->read(size);
        currentFile->resize(newSize);
    }
    return batch;
}

QByteArray Combiner::getBatchFromContainer(const int &size, QFile &containerFile)
{
    QByteArray batch;
    qint64 newSize = containerFile.size() - size;
    if(newSize < 0)
        newSize = 0;
    containerFile.seek(newSize);
    batch = containerFile.read(size);
    containerFile.resize(newSize);
    return batch;
}

void Combiner::openCurrentFile()
{
    QString filePath = devicePath + currentFileIter->path + currentFileIter->name;
    currentFile.reset(new QFile(filePath));    
    currentFile->setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser);
    if(!currentFile->open(QIODevice::ReadWrite))    
        throw std::runtime_error("Failed to open " + filePath.toStdString());
}

void Combiner::encryptBatch(QByteArray &batch)
{
    algorithm.experimentalEncrypt(batch.data(), batch.size());
}

void Combiner::decryptBatch(QByteArray &batch)
{
    algorithm.experimentalDecrypt(batch.data(), batch.size());
}

void Combiner::removeCurrentFileDir()
{
    if(currentFileIter->path.isEmpty() == false)
    {
        QDir dir(devicePath);
        dir.rmpath(currentFileIter->path);
    }
}

void Combiner::restoreFile(QByteArray &batch, QFile &containerFile, const QDir &dir)
{
    qint64 currentSize = 0;
    qint64 fileSize = currentFileIter->size;
    QString filePath = currentFileIter->path;
    if(!filePath.isEmpty())
        dir.mkpath(filePath);
    try{openCurrentFile();}
    catch(...){throw;}
    while(true)
    {
        if(batch.isEmpty())
        {
            batch = getBatchFromContainer(batchSize, containerFile);
            if(batch.isEmpty())
            {
                emit fileProcessed();
                break;
            }
            decryptBatch(batch);
        }
        currentSize += batch.size();
        if(currentSize <= fileSize)
        {
            qint64 firstBlockSize = currentFileIter->firstBlockSize;
            if(firstBlockSize < fileSize)
            {
                if(currentFile->size() == 0)
                {
                    currentFile->write(batch.right(firstBlockSize));
                    batch = batch.left(batch.size() - firstBlockSize);
                }
            }
            currentFile->write(batch);
            batch.clear();
        }
        else
        {
            qint64 batchPadding = currentSize - fileSize;
            currentFile->write(batch.right(batch.size() - batchPadding));
            batch = batch.left(batchPadding);
            emit fileProcessed();
            break;
        }
    }
    currentFile->close();
}

void Combiner::decryptXmlAndFillFileList(QFile &containerFile)
{
    QByteArray batch = getBatchFromContainer(sizeof(quint64), containerFile);
    algorithm.experimentalDecrypt(batch.data(), batch.size());
    quint64 xmlSize = batch.toULongLong();
    batch = getBatchFromContainer(xmlSize, containerFile);
    algorithm.experimentalDecrypt(batch.data(), batch.size());
    QString filePath = devicePath + XmlSaveLoad::getFileName();
    QFile file(filePath);
    file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser);
    if(!file.open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open " + filePath.toStdString());
    file.write(batch);
    file.flush();
    XmlSaveLoad xml;
    xml.loadFileListFromXml(fileList, &file);
    file.remove();
    emit filesCounted(fileList.size());
}

qint64 Combiner::encryptXml(QFile &container)
{
    XmlSaveLoad xml;
    qint64 fileSize;
    try{fileSize = xml.saveFileListAsXml(fileList, devicePath);}
    catch(...){throw;}
    QString filePath = devicePath + XmlSaveLoad::getFileName();
    QFile file(filePath);
    if(!file.open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open " + filePath.toStdString());
    QByteArray batch = file.readAll();
    encryptBatch(batch);
    container.write(batch);
    file.remove();
    emit fileProcessed();
    return fileSize;
}
