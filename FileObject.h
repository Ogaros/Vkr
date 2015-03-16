#ifndef FILESYSTEMOBJECT
#define FILESYSTEMOBJECT

#include <QString>
#include <list>
#include <QtGlobal>

struct FileObject
{
    FileObject(const QString &path, const QString &name, const qint64 &size, const qint64 &firstBlockSize)
        : path(path), name(name), size(size), firstBlockSize(firstBlockSize){}

    QString path;
    QString name;    
    qint64 size;
    qint64 firstBlockSize;
};

#endif // FILESYSTEMOBJECT

