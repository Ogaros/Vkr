#include "XmlSaveLoad.h"

XmlSaveLoad::XmlSaveLoad() : fileName("FileStructure.xml")
{

}

XmlSaveLoad::~XmlSaveLoad()
{

}

size_t XmlSaveLoad::saveStructureAsXml(const FileStructure &fileStructure, const QString &path) const
{
    QFile file(path + fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        throw std::runtime_error("Failed to open " + fileName.toStdString());
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("FileStructure");
    saveObjectsAsXmlRecursive(fileStructure.objects, xml);
    xml.writeEndElement();
    xml.writeEndDocument();
    file.flush();
    size_t size = static_cast<size_t>(file.size());
    file.close();
    return size;
}

void XmlSaveLoad::saveObjectsAsXmlRecursive(const std::list<FileSystemObject> &fileStructure, QXmlStreamWriter &xml) const
{
    for (auto a : fileStructure)
    {
        xml.writeStartElement(a.name);
        xml.writeAttribute("isFolder", QString::number(a.isFolder));
        xml.writeAttribute("size", QString::number(a.size));
        if(a.isFolder)
            saveObjectsAsXmlRecursive(a.folderContents, xml);
        xml.writeEndElement();
    }
}

