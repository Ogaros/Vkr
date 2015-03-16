#ifndef GOST_H
#define GOST_H

#include <QByteArray>
#include <random>
#include <limits>
#include <vector>

class Gost
{
public:
    QByteArray encrypt(QByteArray data);
    QByteArray decrypt(QByteArray data);
    int getBlockSize(){return blockSize;}
    bool setKey(QByteArray newKey);
    bool setInitVector(QByteArray newInitVector);

    quint64 nextGamma(const quint64 amount);
    void setupGamma(qint64 containerSize, int blockSize);
    QByteArray generateInitVector();

private:
    quint64 coreStep(const quint64 block, const int keyPart) const;
    quint64 core32Encrypt(quint64 block) const;
    quint64 core32Decrypt(quint64 block) const;
    quint64 xorEncrypt(quint64 block);
    quint64 xorDecrypt(quint64 block);
    void fillGammaBatch(const int batchSize);

    const int blockSize = 8;
    quint64 gamma;
    quint32 key[8];
    std::vector<quint64> gammaCheckpoints;
    std::vector<std::pair<quint64, int>> gammaBatch;
    std::vector<std::pair<quint64, int>>::reverse_iterator currentGammaIter;
    int currentGammaCheckpoint;
    int batchSize;
    //                                  0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    quint8 replacementTable[8][16] = {{0x4, 0xA, 0x9, 0x2, 0xD, 0x8, 0x0, 0xE, 0x6, 0xB, 0x1, 0xC, 0x7, 0xF, 0x5, 0x3}, // 0
                                      {0xB, 0xE, 0x4, 0xC, 0x6, 0xD, 0xF, 0xA, 0x2, 0x3, 0x8, 0x1, 0x0, 0x7, 0x5, 0x9}, // 1
                                      {0x5, 0x8, 0x1, 0xD, 0xA, 0x3, 0x4, 0x2, 0xE, 0xF, 0xC, 0x7, 0x6, 0x0, 0x9, 0xB}, // 2
                                      {0x7, 0xD, 0xA, 0x1, 0x0, 0x8, 0x9, 0xF, 0xE, 0x4, 0x6, 0xC, 0xB, 0x2, 0x5, 0x3}, // 3
                                      {0x6, 0xC, 0x7, 0x1, 0x5, 0xF, 0xD, 0x8, 0x4, 0xA, 0x9, 0xE, 0x0, 0x3, 0xB, 0x2}, // 4
                                      {0x4, 0xB, 0xA, 0x0, 0x7, 0x2, 0x1, 0xD, 0x3, 0x6, 0x8, 0x5, 0x9, 0xC, 0xF, 0xE}, // 5
                                      {0xD, 0xB, 0x4, 0x1, 0x3, 0xF, 0x5, 0x9, 0x0, 0xA, 0xE, 0x7, 0x6, 0x8, 0x2, 0xC}, // 6
                                      {0x1, 0xF, 0xD, 0x0, 0x5, 0x7, 0xA, 0x4, 0x9, 0x2, 0x3, 0xE, 0x6, 0xB, 0x8, 0xC}};// 7
};

#endif // GOST_H
