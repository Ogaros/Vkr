#include "Combiner.h"

const QString Combiner::containerName = "Encrypted.data";
const int Combiner::blockSize = 256 * 1024 * 1024; // 256 MB

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
    fillFileList(path);
    if(fileList.empty())
        throw std::runtime_error("No files to encrypt");
    emit filesCounted(fileList.size());

    XmlSaveLoad xml;
    qint64 xmlSize;
    try{xmlSize = xml.saveFileListAsXml(fileList, path);}
    catch(...){throw;}

    fileList.emplace_front(path, xml.getFileName(), 0, 0);
    currentFileIter = fileList.begin();

    QFile containerFile(path + containerName);
    if(!containerFile.open(QIODevice::WriteOnly))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    size_t blockSize = algorithm.getBlockSize();
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
    devicePath = path;
    fillFileList(devicePath);
    if(fileList.empty())
        throw std::runtime_error("No files to encrypt");
    emit filesCounted(fileList.size());

    currentFileIter = fileList.begin();

    QFile containerFile(devicePath + containerName);
    if(!containerFile.open(QIODevice::WriteOnly))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    currentFile.reset(nullptr);
    QByteArray initVector = algorithm.generateInitVector();
    try
    {
        do
        {
            block = getBlockReverse(blockSize);
            encryptBlock(block);
            containerFile.write(block);
        }
        while(block.size() == blockSize);
    }
    catch(...){throw;}

    qint64 xmlSize = fileList.back().size;

    block.setNum(xmlSize);
    if(block.size() < 8) // xml size takes 8 bytes
        block.prepend(QByteArray(8 - block.size(), '0'));
    containerFile.write(algorithm.encrypt(block));

    containerFile.write(initVector);

    containerFile.flush();
    containerFile.close();
    emit processingFinished();
}

void Combiner::separateReverse(const QString &path)
{
    devicePath = path;
    fileList.clear();
    QFile containerFile(devicePath + containerName);
    if(!containerFile.open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    int algBlockSize = algorithm.getBlockSize();

    block = getBlockFromContainerReverse(algBlockSize, containerFile);
    algorithm.setInitVector(block);
    algorithm.setupGamma(containerFile.size(), blockSize);

    block = getBlockFromContainerReverse(8, containerFile);
    block = algorithm.decrypt(block);
    fileList.emplace_back("", XmlSaveLoad::getFileName(), block.toULongLong(), block.toULongLong());
    block.clear();
    QDir dir(devicePath);
    for(currentFileIter = fileList.begin(); currentFileIter != fileList.end(); currentFileIter++)
    {
        restoreFile(block, containerFile, dir);
    }
    containerFile.remove();
    emit processingFinished();
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
            emit fileProcessed();
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
                emit fileProcessed();
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
    if(fileSize < size)
    {        
        emit fileProcessed();
        currentFileIter->firstBlockSize = fileSize;
        currentFile->seek(0);
        block = currentFile->read(fileSize);
        while(currentFile->exists())
            currentFile->remove();

        currentFile.reset(nullptr);
        removeCurrentFileDir();

        currentFileIter++;
        if(currentFileIter != fileList.end())
        {
            block.append(getBlockReverse(size - block.size()));
        }
        else
        {
            currentFileIter--;
            if(currentFileIter->name != XmlSaveLoad::getFileName())
            {
                try{saveFileList();}
                catch(...){throw;}
                currentFileIter++;
                block.append(getBlockReverse(size - block.size()));
            }
            else
                currentFileIter++;
        }        
    }
    else
    {
        qint64 newSize = fileSize - size;
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
    if(newSize < 0)
        newSize = 0;
    containerFile.seek(newSize);
    block = containerFile.read(size);
    containerFile.resize(newSize);
    return block;
}

void Combiner::openCurrentFile()
{
    QString filePath = devicePath + currentFileIter->path + currentFileIter->name;
    currentFile.reset(new QFile(filePath));    
    currentFile->setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser);
    if(!currentFile->open(QIODevice::ReadWrite))    
        throw std::runtime_error("Failed to open " + filePath.toStdString());
}

void Combiner::encryptBlock(QByteArray &block)
{
    int algBlockSize = algorithm.getBlockSize();
    for(int i = 0; i < block.size(); i += algBlockSize)
    {
        block.replace(i, algBlockSize, algorithm.encrypt(block.mid(i, algBlockSize)));
    }
}

void Combiner::decryptBlock(QByteArray &block)
{
    int algBlockSize = algorithm.getBlockSize();
    for(int i = 0; i < block.size(); i += algBlockSize)
    {
        block.replace(i, algBlockSize, algorithm.decrypt(block.mid(i, algBlockSize)));
    }
}

void Combiner::saveFileList()
{
    XmlSaveLoad xml;
    qint64 fileSize;
    try{fileSize = xml.saveFileListAsXml(fileList, devicePath);}
    catch(...){throw;}
    fileList.emplace_back("", XmlSaveLoad::getFileName(), fileSize, fileSize);    
}

void Combiner::removeCurrentFileDir()
{
    if(currentFileIter->path.isEmpty() == false)
    {
        QDir dir(devicePath);
        dir.rmpath(currentFileIter->path);
    }
}

void Combiner::restoreFile(QByteArray &block, QFile &containerFile, const QDir &dir)
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
        if(block.isEmpty())
        {
            block = getBlockFromContainerReverse(blockSize, containerFile);
            if(block.isEmpty())
            {
                emit fileProcessed();
                break;
            }
            decryptBlock(block);
        }
        currentSize += block.size();
        if(currentSize <= fileSize)
        {
            qint64 firstBlockSize = currentFileIter->firstBlockSize;
            if(firstBlockSize < fileSize)
            {
                if(currentFile->size() == 0)
                {
                    currentFile->write(block.right(firstBlockSize));
                    block = block.left(block.size() - firstBlockSize);
                    currentSize += blockSize - block.size();
                    QByteArray temp = getBlockFromContainerReverse(blockSize - block.size(), containerFile);
                    decryptBlock(temp);
                    block.prepend(temp);
                }
            }
            currentFile->write(block);
            block.clear();
        }
        else
        {
            qint64 blockPadding = currentSize - fileSize;
            currentFile->write(block.right(block.size() - blockPadding));
            block = block.left(blockPadding);
            if(currentFileIter == fileList.begin())
            {
                currentFile->flush();
                XmlSaveLoad xml;
                xml.loadFileListFromXml(fileList, currentFile.get());
                fileList.splice(fileList.begin(), fileList, std::prev(fileList.end(), 1));
                currentFile->remove();
                emit filesCounted(fileList.size() - 1);
            }
            else
                emit fileProcessed();
            break;
        }
    }
    currentFile->close();
}
