#ifndef COMBINER_H
#define COMBINER_H

#include <QObject>
#include <list>
#include <QFile>
#include <QString>
#include <string>
#include <memory>
#include <exception>
#include <windows.h>
#include <EncryptionAlgorithm/EncryptionAlgorithm.h>
#include <FileSystemObject.h>
#include <FileStructure.h>
#include <XmlSaveLoad.h>

using std::list;

class Combiner : public QObject
{
    Q_OBJECT
public:
    explicit Combiner(QObject *parent = 0);
    ~Combiner();
    void combine(const QString &path);
    void setAlgorithm(std::unique_ptr<EncryptionAlgorithm> alg);

private:
    FileStructure createFileStructure(const QString &path, int &numberOfFiles);
    QByteArray getBlock(const size_t &size);
    void openCurrentFile();
    std::unique_ptr<EncryptionAlgorithm> algorithm;
    std::unique_ptr<QFile> currentFile;
    FileSystemObject *p_currentFileObject;
    FileSystemObject *p_prevFile;

signals:
    void fileEncrypted();
    void filesCounted(int number);

public slots:

};

#endif // COMBINER_H
