#ifndef FILESYSTEMOBJECT
#define FILESYSTEMOBJECT

#include <QString>
#include <list>
#include <QtGlobal>

struct FileObject
{
    FileObject(const QString &path, const QString &name, const quint64 &size)
        : path(path), name(name), size(size), nextFile(nullptr){}

    QString path;
    QString name;    
    quint64 size;
    FileObject *nextFile;
    int test;
};

#endif // FILESYSTEMOBJECT

