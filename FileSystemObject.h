#ifndef FILESYSTEMOBJECT
#define FILESYSTEMOBJECT

#include <QString>
#include <list>
#include <QtGlobal>

struct FileSystemObject
{
    FileSystemObject(const bool isFolder, const QString &path, const QString &name, const quint64 &size)
        : isFolder(isFolder), path(path), name(name), size(size), nextFile(nullptr){}

    bool isFolder;
    QString path;
    QString name;    
    quint64 size;
    FileSystemObject *nextFile;
    std::list<FileSystemObject> folderContents;
};

#endif // FILESYSTEMOBJECT

