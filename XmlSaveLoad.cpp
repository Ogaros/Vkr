#include "XmlSaveLoad.h"

const QString XmlSaveLoad::fileName = "FileList.xml";

XmlSaveLoad::XmlSaveLoad()
{

}

XmlSaveLoad::~XmlSaveLoad()
{

}

quint64 XmlSaveLoad::saveFileListAsXml(const FileList &fileList, const QString &path) const
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
        xml.writeTextElement("firstBlockSize", QString::number(a.firstBlockSize));
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    file.flush();
    quint64 size = static_cast<quint64>(file.size());
    file.close();
    return size;
}

void XmlSaveLoad::loadFileListFromXml(FileList &fileList, QFile * file) const
{
    file->seek(0);
    QXmlStreamReader xml(file);
    while(!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;
        if(token == QXmlStreamReader::StartElement)
        {
            if(xml.name() == "FileList")
                continue;
            if(xml.name() == "file")
            {
                fileList.push_front(parseFile(xml));
            }
        }
    }
}

FileObject XmlSaveLoad::parseFile(QXmlStreamReader &xml) const
{
    QString name, path;
    qint64 size, firstBlockSize;
    QXmlStreamAttributes attributes = xml.attributes();
    if(attributes.hasAttribute("name"))
        name = attributes.value("name").toString();
    xml.readNext();
    while(xml.tokenType() != QXmlStreamReader::EndElement || xml.name() != "file")
    {
        if(xml.tokenType() == QXmlStreamReader::StartElement)
        {
            if(xml.name() == "path")
            {
                xml.readNext();
                path = xml.text().toString();
            }
            else if(xml.name() == "size")
            {
                xml.readNext();
                size = xml.text().toULongLong();
            }
            else if(xml.name() == "firstBlockSize")
            {
                xml.readNext();
                firstBlockSize = xml.text().toULongLong();
            }
        }
        xml.readNext();
    }
    return FileObject(path, name, size, firstBlockSize);
}
