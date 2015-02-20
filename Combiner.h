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
#include <FileObject.h>
#include <XmlSaveLoad.h>

using FileList = std::list<FileObject>;

class Combiner : public QObject
{
    Q_OBJECT
public:
    explicit Combiner(QObject *parent = 0);
    ~Combiner();
    void combine(const QString &path);
    void setAlgorithm(std::unique_ptr<EncryptionAlgorithm> alg);

private:
    void fillFileList(const QString &path);
    QByteArray getBlock(const int &size);
    QByteArray getBlockReverse(const int &size);
    void openCurrentFile();
    std::unique_ptr<EncryptionAlgorithm> algorithm;
    std::unique_ptr<QFile> currentFile;
    FileList fileList;
    FileList::iterator currentFileIter;


signals:
    void fileEncrypted();
    void filesCounted(int number);

public slots:

};

#endif // COMBINER_H
