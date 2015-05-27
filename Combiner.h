#ifndef COMBINER_H
#define COMBINER_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

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
#include <QDebug>
#include <QByteArray.h>
#include <QCoreApplication>
#include <DBManager.h>

using FileList = std::list<FileObject>;

class Combiner : public QObject
{
    Q_OBJECT
public:
    explicit Combiner(QObject *parent = 0);
    ~Combiner();
    void combine(const QString &path);
    void separate(const QString &path);	
    static QString getContainerName() {return containerName;}
	static QString getKeyFileName() { return keyFileName; }
	static QByteArray getBaseKey() { return baseKey; }

private:
    int fillFileList(const QString &path);
    QByteArray getBatch(const int &size);
    QByteArray getBatchFromContainer(const int &size, QFile &containerFile);
    void openCurrentFile();
    void encryptBatch(QByteArray &batch);
    void decryptBatch(QByteArray &batch);
    void removeCurrentFileDir();
    void restoreFile(QByteArray &batch, QFile &containerFile, const QDir &dir);
    int decryptXmlAndFillFileList(QFile &containerFile);
    void loadEncryptionKey();
	void nextFile();
	void removeFolders();
    qint64 encryptXml(QFile &container);
    Gost algorithm;
    std::unique_ptr<QFile> currentFile;
    FileList fileList;
    FileList::iterator currentFileIter;
    QString devicePath;
	DBManager db;
    static const QString containerName;
    static const int batchSize;
    static const QString keyFileName;
    static const QByteArray baseKey;

signals:
    void fileProcessed();
    void processingFinished();
    void filesCounted(int number);

public slots:

};

#endif // COMBINER_H
