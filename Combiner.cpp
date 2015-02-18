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

FileStructure Combiner::createFileStructure(const QString &path)
{
    FileStructure fileStructure;
    char search_path[200];
    sprintf(search_path, "%s*.*", path.toStdString().c_str());
    WIN32_FIND_DATAA fd;
    HANDLE hFind = ::FindFirstFileA(search_path, &fd);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            QString name = fd.cFileName;
            if(name != "." && name != "..")
            {
                bool isFolder = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                quint64 size = isFolder ? 0 : (fd.nFileSizeHigh * (MAXDWORD + 1)) + fd.nFileSizeLow;
                fileStructure.objects.emplace_back(isFolder, path, name, size);
                if(isFolder)
                    fileStructure.objects.back().folderContents = createFileStructure(path + name + "\\").objects;
                else
                {
                    numberOfFiles++;
                    if(fileStructure.firstFile == nullptr)
                        fileStructure.firstFile = &fileStructure.objects.back();
                    if(p_prevFile != nullptr)
                        p_prevFile->nextFile = &fileStructure.objects.back();
                    p_prevFile = &fileStructure.objects.back();
                }
            }
        }while(::FindNextFileA(hFind, &fd));
        ::FindClose(hFind);
    }
    return fileStructure;
}

void Combiner::combine(const QString &path)
{
    p_prevFile = nullptr;
    numberOfFiles = 0;
    FileStructure fileStructure = createFileStructure(path);
    emit filesCounted(numberOfFiles);
    p_currentFileObject = fileStructure.firstFile;
    if(p_currentFileObject == nullptr)
        throw std::runtime_error("No files to encrypt");

    XmlSaveLoad xml;
    size_t xmlSize;
    try{xmlSize = xml.saveStructureAsXml(fileStructure, path);}
    catch(...){throw;}

    FileSystemObject xmlFile(false, path, xml.getFileName(), 0);
    xmlFile.nextFile = p_currentFileObject;
    p_currentFileObject = &xmlFile;

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
        while(!(block = getBlock(blockSize)).isEmpty());
    }
    catch(...){throw;}
    containerFile.flush();
    containerFile.close();
}

QByteArray Combiner::getBlock(const size_t &size)
{
    QByteArray block;
    if(currentFile == nullptr)
    {
        try {openCurrentFile();}
        catch(...){throw;}
    }
    block = currentFile->read(size);
    if(static_cast<size_t>(block.size()) < size)
    {
        emit fileEncrypted();
        if(p_currentFileObject->nextFile != nullptr)
        {            
            currentFile->close();
            p_currentFileObject = p_currentFileObject->nextFile;
            try {openCurrentFile();}
            catch(...){throw;}
            block.append(getBlock(size - static_cast<size_t>(block.size())));
        }
        else
        {
            if(!block.isEmpty())
            {
                block.append(QByteArray(size - static_cast<size_t>(block.size()), 0));
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

void Combiner::openCurrentFile()
{
    currentFile.reset(new QFile(p_currentFileObject->path + p_currentFileObject->name));
    if(!currentFile->open(QIODevice::ReadOnly))
        throw std::runtime_error("Failed to open file for encryption");
}
