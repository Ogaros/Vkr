#include "Combiner.h"

Combiner::Combiner(QObject *parent) : QObject(parent), containerName("Encrypted.data")
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
                    quint64 size = (fd.nFileSizeHigh * (MAXDWORD + 1)) + fd.nFileSizeLow;
                    QString noDrivePath = path;
                    noDrivePath.replace(noDrivePath.indexOf(drivePath), drivePath.size(), "");
                    fileList.emplace_back(noDrivePath, name, size);
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
    drivePath = path;
    fillFileList(path);
    if(fileList.empty())
        throw std::runtime_error("No files to encrypt");
    emit filesCounted(fileList.size());

    XmlSaveLoad xml;
    qint64 xmlSize;
    try{xmlSize = xml.saveFileListAsXml(fileList, path);}
    catch(...){throw;}

    fileList.emplace_back("", xml.getFileName(), 0);
    currentFileIter = fileList.begin();

    QFile containerFile(path + containerName);
    if(!containerFile.open(QIODevice::WriteOnly))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    int blockSize = algorithm.getBlockSize();

    qint64 containerSize = xmlSize;
    for(auto &a : fileList)
        containerSize += a.size;
    if(containerSize % blockSize != 0)
    {
        block.fill(0, (((containerSize / blockSize) + 1) * blockSize) - containerSize);
    }
    currentFile.reset(nullptr);
    QDir dir(path);
    try
    {
        block.append(getBlockReverse(blockSize - block.size(), dir));
        containerFile.write(algorithm.encrypt(block));
        while((block = getBlockReverse(blockSize, dir)).size() == blockSize)
        {
            containerFile.write(algorithm.encrypt(block));
        }
    }
    catch(...){throw;}

    block.setNum(xmlSize);
    if(block.size() < blockSize)
        block.prepend(QByteArray(blockSize - static_cast<size_t>(block.size()), '0'));
    containerFile.write(algorithm.encrypt(block));

    containerFile.flush();
    containerFile.close();
    emit processingFinished();
}

void Combiner::separateReverse(const QString &path)
{
    fileList.clear();
    drivePath = path;
    QFile containerFile(path + containerName);
    if(!containerFile.open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open container file");

    QByteArray block;
    size_t blockSize = algorithm.getBlockSize();

    block = getBlockFromContainerReverse(blockSize, containerFile);
    block = algorithm.decrypt(block);
    XmlSaveLoad xml;
    fileList.emplace_back("", xml.getFileName(), block.toLongLong());
    block.clear();
    QDir dir(path);
    for(currentFileIter = fileList.begin(); currentFileIter != fileList.end(); currentFileIter++)
    {
        qint64 currentSize = 0;
        qint64 fileSize = currentFileIter->size;
        const QString filePath = currentFileIter->path;
        if(!filePath.isEmpty())
            dir.mkpath(filePath);
        openCurrentFile();
        while(true)
        {
            if(block.isEmpty())
            {
                block = getBlockFromContainerReverse(blockSize, containerFile);
                block = algorithm.decrypt(block);
                //TODO:: add block size check
            }
            currentSize += block.size();
            if(currentSize <= fileSize)
            {
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
                    xml.loadFileListFromXml(fileList, currentFile.get());                    
                    fileList.splice(fileList.begin(), fileList, std::prev(fileList.end(), 1));
                    currentFile->remove();
                    emit filesCounted(fileList.size() - 1);
                }
                emit fileProcessed();
                break;
            }
        }
        currentFile->close();
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

QByteArray Combiner::getBlockReverse(const int &size, const QDir &dir)
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
        emit fileProcessed();
        currentFile->seek(0);
        block = currentFile->read(fileSize);
        currentFile->remove();
        if(!currentFileIter->path.isEmpty())
            dir.rmpath(currentFileIter->path);

        currentFileIter++;
        if(currentFileIter != fileList.end())
        {
            try {openCurrentFile();}
            catch(...){throw;}
            block.append(getBlockReverse(size - block.size(), dir));
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
    QString filePath = drivePath + currentFileIter->path + currentFileIter->name;
    currentFile.reset(new QFile(filePath));
    if(!currentFile->open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open " + filePath.toStdString());
}
