#include "Combiner.h"

Combiner::Combiner(QObject *parent) : QObject(parent), containerName("Encrypted.data")
{

}

Combiner::~Combiner()
{

}

void Combiner::setAlgorithm(std::unique_ptr<EncryptionAlgorithm> alg)
{
    this->algorithm.swap(alg);
}

void Combiner::fillFileList(const QString &path)
{
    QString sPath = path + "*.*";
    WCHAR search_path[sPath.size()+1];
    sPath.toWCharArray(search_path);
    search_path[sPath.size()] = '\0';
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(search_path, &fd);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            QString name = QString::fromWCharArray(fd.cFileName);
            if(name != "." && name != "..")
            {
                if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    fillFileList(path + name + "\\");
                else
                {
                    quint64 size = (fd.nFileSizeHigh * (MAXDWORD + 1)) + fd.nFileSizeLow;
                    fileList.emplace_back(path, name, size);
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
    fillFileList(path);
    if(fileList.empty())
        throw std::runtime_error("No files to encrypt");
    emit filesCounted(fileList.size());

    XmlSaveLoad xml;
    qint64 xmlSize;
    try{xmlSize = xml.saveFileListAsXml(fileList, path);}
    catch(...){throw;}

    fileList.emplace_front(path, xml.getFileName(), 0);
    currentFileIter = fileList.begin();

    QFile containerFile(path + containerName);
    if(!containerFile.open(QIODevice::WriteOnly))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    size_t blockSize = 64; // algorithm->getBlockSize();
    try
    {        
        block.setNum(xmlSize);
        if(static_cast<size_t>(block.size()) < blockSize)
            block.prepend(QByteArray(blockSize - static_cast<size_t>(block.size()), 0));
        do
        {
            containerFile.write(block);
        }
        while(!(block = getBlock(blockSize)).isEmpty());
    }
    catch(...){throw;}
    containerFile.flush();
    containerFile.close();
}

void Combiner::combineReverse(const QString &path)
{
    fileList.clear();
    fillFileList(path);
    if(fileList.empty())
        throw std::runtime_error("No files to encrypt");
    emit filesCounted(fileList.size());

    XmlSaveLoad xml;
    qint64 xmlSize;
    try{xmlSize = xml.saveFileListAsXml(fileList, path);}
    catch(...){throw;}

    fileList.emplace_back(path, xml.getFileName(), 0);
    currentFileIter = fileList.begin();

    QFile containerFile(path + containerName);
    if(!containerFile.open(QIODevice::WriteOnly))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    int blockSize = 64; // algorithm->getBlockSize();

    qint64 containerSize = xmlSize;
    for(auto &a : fileList)
        containerSize += a.size;
    if(containerSize % blockSize != 0)
    {
        block.fill(0, (((containerSize / blockSize) + 1) * blockSize) - containerSize);
    }

    try
    {
        block.append(getBlockReverse(blockSize - block.size()));
        containerFile.write(block);
        while((block = getBlockReverse(blockSize)).size() == blockSize)
        {
            containerFile.write(block);
        }
    }
    catch(...){throw;}

    block.setNum(xmlSize);
    if(block.size() < blockSize)
        block.prepend(QByteArray(blockSize - static_cast<size_t>(block.size()), '0'));
    containerFile.write(block);

    containerFile.flush();
    containerFile.close();
}

void Combiner::separateReverse(const QString &path)
{
    fileList.clear();
    QFile containerFile(path + containerName);
    if(!containerFile.open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    size_t blockSize = 64; // algorithm->getBlockSize();

    block = getBlockFromContainerReverse(blockSize, containerFile);
    qint64 xmlSize = block.toLongLong();
    XmlSaveLoad xml;
    QFile xmlFile(path + xml.getFileName());
    if(!xmlFile.open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open xml file");

    for(qint64 currentSize = 0;;)
    {
        block = getBlockFromContainerReverse(blockSize, containerFile);
        currentSize += block.size();
        if(currentSize <= xmlSize)
        {
            xmlFile.write(block);
        }
        else
        {
            xmlFile.write(block.right(blockSize - (currentSize - xmlSize)));
            break;
        }
    }
    xmlFile.flush();
    xml.loadFileListFromXml(fileList, &xmlFile);
    xmlFile.remove();
}

QByteArray Combiner::getBlock(const int &size)
{
    QByteArray block;
    if(currentFile == nullptr)
    {
        try {openCurrentFile();}
        catch(...){throw;}
    }
    block = currentFile->read(size);
    if(block.size() < size)
    {        
        currentFileIter++;
        if(currentFileIter != fileList.end())
        {
            emit fileEncrypted();
            currentFile->close();            
            try {openCurrentFile();}
            catch(...){throw;}
            block.append(getBlock(size - block.size()));
        }
        else
        {
            currentFileIter--;
            if(!block.isEmpty())
            {
                emit fileEncrypted();
                block.append(QByteArray(size - block.size(), 0));
            }
            else
            {
                currentFile->close();
                currentFile.reset(nullptr);
            }
        }
    }
    return block;
}

QByteArray Combiner::getBlockReverse(const int &size)
{
    QByteArray block;
    if(currentFile == nullptr)
    {
        try {openCurrentFile();}
        catch(...){throw;}
    }
    qint64 fileSize = currentFile->size();
    if(fileSize < static_cast<qint64>(size))
    {
        emit fileEncrypted();
        currentFile->seek(0);
        block = currentFile->read(fileSize);
        currentFile->remove();

        currentFileIter++;
        if(currentFileIter != fileList.end())
        {
            try {openCurrentFile();}
            catch(...){throw;}
            block.append(getBlockReverse(size - block.size()));
        }
        else
        {
            if(block.isEmpty())
            {
                currentFile.reset(nullptr);
            }
        }
    }
    else
    {
        qint64 newSize = fileSize - static_cast<qint64>(size);
        currentFile->seek(newSize);
        block = currentFile->read(size);
        currentFile->resize(newSize);
    }
    return block;
}

QByteArray Combiner::getBlockFromContainerReverse(const int &size, QFile &containerFile)
{
    QByteArray block;
    qint64 newSize = containerFile.size() - size;
    containerFile.seek(newSize);
    block = containerFile.read(size);
    containerFile.resize(newSize);
    return block;
}

void Combiner::openCurrentFile()
{
    currentFile.reset(new QFile(currentFileIter->path + currentFileIter->name));
    if(!currentFile->open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open file for encryption");
}
