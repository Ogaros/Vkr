#ifndef COMBINER_H
#define COMBINER_H

#include <QObject>
#include <list>
#include <QFile>
#include <QDir>
#include <QString>
#include <string>
#include <memory>
#include <exception>
#include <windows.h>
#include <EncryptionAlgorithm/Gost.h>
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
    void combineReverse(const QString &path);
    void separateReverse(const QString &path);

private:
    void fillFileList(const QString &path);
    QByteArray getBlock(const int &size);
    QByteArray getBlockReverse(const int &size);
    QByteArray getBlockFromContainerReverse(const int &size, QFile &containerFile);
    void openCurrentFile();
    void encryptBlock(QByteArray &block);
    void decryptBlock(QByteArray &block);
    void saveFileList();
    void removeCurrentFileDir();
    void restoreFile(QByteArray &block, QFile &containerFile, const QDir &dir);
    Gost algorithm;
    std::unique_ptr<QFile> currentFile;
    FileList fileList;
    FileList::iterator currentFileIter;
    QString devicePath;
    static const QString containerName;
    static const int blockSize;


signals:
    void fileProcessed();
    void processingFinished();
    void filesCounted(int number);

public slots:

};

#endif // COMBINER_H
