#ifndef XMLSAVELOAD_H
#define XMLSAVELOAD_H

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <exception>
#include <list>
#include <FileObject.h>
#include <memory>

using FileList = std::list<FileObject>;

class XmlSaveLoad
{
public:
    XmlSaveLoad();
    ~XmlSaveLoad();
    qint64 saveFileListAsXml(const FileList &fileList, const QString &path) const;
    void loadFileListFromXml(FileList &fileList, QFile * file) const;
    static const QString getFileName() {return fileName;}

private:
    static const QString fileName;
    FileObject parseFile(QXmlStreamReader &xml) const;
};

#endif // XMLSAVELOAD_H
