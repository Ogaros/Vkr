#ifndef XMLSAVELOAD_H
#define XMLSAVELOAD_H

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <exception>
#include <list>
#include <FileObject.h>

using FileList = std::list<FileObject>;

class XmlSaveLoad
{
public:
    XmlSaveLoad();
    ~XmlSaveLoad();
    qint64 saveFileListAsXml(const FileList &fileStructure, const QString &path) const;
    const QString getFileName() const {return fileName;}

private:
    const QString fileName;
};

#endif // XMLSAVELOAD_H
