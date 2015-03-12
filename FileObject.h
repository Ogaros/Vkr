#ifndef FILESYSTEMOBJECT
#define FILESYSTEMOBJECT

#include <QString>
#include <list>
#include <QtGlobal>

struct FileObject
{
    FileObject(const QString &path, const QString &name, const quint64 &size, const quint64 &firstBlockSize)
        : path(path), name(name), size(size), firstBlockSize(firstBlockSize){}

    QString path;
    QString name;    
    quint64 size;
    quint64 firstBlockSize;
};

#endif // FILESYSTEMOBJECT

