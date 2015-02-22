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
    const QString getFileName() const {return fileName;}

private:
    const QString fileName;
    FileObject parseFile(QXmlStreamReader &xml) const;
};

#endif // XMLSAVELOAD_H
