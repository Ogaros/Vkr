#ifndef XMLSAVELOAD_H
#define XMLSAVELOAD_H

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <exception>
#include <list>
#include <FileStructure.h>

class XmlSaveLoad
{
public:
    XmlSaveLoad();
    ~XmlSaveLoad();
    size_t saveStructureAsXml(const FileStructure &fileStructure, const QString &path) const;
    const QString getFileName() const {return fileName;}

private:
    void saveObjectsAsXmlRecursive(const std::list<FileSystemObject> &fileStructure, QXmlStreamWriter &xml) const;
    const QString fileName;
};

#endif // XMLSAVELOAD_H
