#ifndef ENCRYPTIONALGORITHM_H
#define ENCRYPTIONALGORITHM_H

#include <QByteArray>

class EncryptionAlgorithm
{
public:
    virtual int getBlockSize() = 0;
    virtual QByteArray encrypt(QByteArray data, QByteArray key) = 0;
    virtual QByteArray decrypt(QByteArray data, QByteArray key) = 0;
};

#endif // ENCRYPTIONALGORITHM_H
