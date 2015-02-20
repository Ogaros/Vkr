#include "Combiner.h"

Combiner::Combiner(QObject *parent) : QObject(parent)
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

    QFile containerFile(path + "Encrypted.data");    
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
        while(!(block = getBlockReverse(blockSize)).isEmpty());
    }
    catch(...){throw;}
    containerFile.flush();
    containerFile.close();
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
        block = currentFile->read(fileSize);
        currentFile->remove();

        currentFileIter++;
        if(currentFileIter != fileList.end())
        {
            emit fileEncrypted();
            try {openCurrentFile();}
            catch(...){throw;}
            block.append(getBlockReverse(size - block.size()));
        }
        else
        {
            currentFileIter--;
            emit fileEncrypted();
            if(!block.isEmpty())
            {
                block.append(QByteArray(size - block.size(), 0));
            }
            else
            {
                currentFile.reset(nullptr);
            }
        }
    }
    else
    {
        qint64 newSize = fileSize - static_cast<qint64>(size);
        currentFile->seek(newSize);
        block = currentFile->read(fileSize);
        currentFile->resize(newSize);
    }
    return block;
}

void Combiner::openCurrentFile()
{
    currentFile.reset(new QFile(currentFileIter->path + currentFileIter->name));
    if(!currentFile->open(QIODevice::ReadWrite))
        throw std::runtime_error("Failed to open file for encryption");
}
