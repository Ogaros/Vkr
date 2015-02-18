#ifndef FILESYSTEMOBJECT
#define FILESYSTEMOBJECT

#include <QString>
#include <list>
#include <QtGlobal>

struct FileObject
{
    FileObject(const QString &path, const QString &name, const quint64 &size)
        : path(path), name(name), size(size){}

    QString path;
    QString name;    
    quint64 size;
};

#endif // FILESYSTEMOBJECT

