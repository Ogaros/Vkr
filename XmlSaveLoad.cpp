#include "XmlSaveLoad.h"

XmlSaveLoad::XmlSaveLoad() : fileName("FileStructure.xml")
{

}

XmlSaveLoad::~XmlSaveLoad()
{

}

qint64 XmlSaveLoad::saveFileListAsXml(const FileList &fileList, const QString &path) const
{
    QFile file(path + fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        throw std::runtime_error("Failed to open " + fileName.toStdString());
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("FileList");
    for (auto a : fileList)
    {
        xml.writeStartElement("file");
        xml.writeAttribute("name", a.name);
        xml.writeTextElement("path", a.path);
        xml.writeTextElement("size", QString::number(a.size));
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    file.flush();
    qint64 size = file.size();
    file.close();
    return size;
}
