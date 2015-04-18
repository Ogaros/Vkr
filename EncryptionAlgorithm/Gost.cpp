#include "Gost.h"

//								            0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
quint8 Gost::replacementTable[8][16] = { { 0x4, 0xA, 0x9, 0x2, 0xD, 0x8, 0x0, 0xE, 0x6, 0xB, 0x1, 0xC, 0x7, 0xF, 0x5, 0x3 },  // 0
										 { 0xB, 0xE, 0x4, 0xC, 0x6, 0xD, 0xF, 0xA, 0x2, 0x3, 0x8, 0x1, 0x0, 0x7, 0x5, 0x9 },  // 1
										 { 0x5, 0x8, 0x1, 0xD, 0xA, 0x3, 0x4, 0x2, 0xE, 0xF, 0xC, 0x7, 0x6, 0x0, 0x9, 0xB },  // 2
										 { 0x7, 0xD, 0xA, 0x1, 0x0, 0x8, 0x9, 0xF, 0xE, 0x4, 0x6, 0xC, 0xB, 0x2, 0x5, 0x3 },  // 3
										 { 0x6, 0xC, 0x7, 0x1, 0x5, 0xF, 0xD, 0x8, 0x4, 0xA, 0x9, 0xE, 0x0, 0x3, 0xB, 0x2 },  // 4
										 { 0x4, 0xB, 0xA, 0x0, 0x7, 0x2, 0x1, 0xD, 0x3, 0x6, 0x8, 0x5, 0x9, 0xC, 0xF, 0xE },  // 5
										 { 0xD, 0xB, 0x4, 0x1, 0x3, 0xF, 0x5, 0x9, 0x0, 0xA, 0xE, 0x7, 0x6, 0x8, 0x2, 0xC },  // 6
										 { 0x1, 0xF, 0xD, 0x0, 0x5, 0x7, 0xA, 0x4, 0x9, 0x2, 0x3, 0xE, 0x6, 0xB, 0x8, 0xC } };// 7

const int Gost::gammaBatchSize = 32 * 1024 * 1024; // 32 MB

Gost::Gost(QObject *parent) : QObject(parent)
{
    fillOptimizedRepTable();
}

void Gost::decryptKey(QByteArray &data)
{
    quint64 block;
    char *dataPtr = data.data();
    for(int i = 0; i < data.size(); i += blockSize)
    {
        block = 0;
        memcpy(&block, &dataPtr[i], blockSize);
        block = core32Decrypt(block);
        memcpy(&dataPtr[i], &block, blockSize);
    }
}

bool Gost::setKey(QByteArray newKey)
{
    if(newKey.size() == 256 / 8)
    {        
        char *c = newKey.data();
        for(int i = 0; i < 8; i++)
        {
            key[i] = c[i*4] + (c[i*4 + 1] << 8) + (c[i*4 + 2] << 16) + (c[i*4 + 3] << 24);
        }
        return true;
    }
    else
        return false;
}

bool Gost::setInitVector(QByteArray newInitVector)
{
    if(newInitVector.size() == 64 / 8)
    {
        gamma = 0;
        for(int i = 0; i < 8; i++)
        {
            gamma <<= 8;
            gamma |= static_cast<quint8>(newInitVector[i]);
        }
        return true;
    }
    else
        return false;
}

inline quint64 Gost::replaceAndRotate(quint32 block) const
{
    block = repTableOptimized[0][block >> 24 & 0xFF] << 24 | repTableOptimized[1][block >> 16 & 0xFF] << 16 |
            repTableOptimized[2][block >>  8 & 0xFF] <<  8 | repTableOptimized[3][block & 0xFF];

    return (block << 11) | (block >> 21);
}

quint64 Gost::core32Encrypt(quint64 block) const
{
    quint32 n1 = block & 0xFFFFFFFF;
    quint32 n2 = block >> 32;

    n2 ^= replaceAndRotate(n1 + key[0]);
    n1 ^= replaceAndRotate(n2 + key[1]);
    n2 ^= replaceAndRotate(n1 + key[2]);
    n1 ^= replaceAndRotate(n2 + key[3]);
    n2 ^= replaceAndRotate(n1 + key[4]);
    n1 ^= replaceAndRotate(n2 + key[5]);
    n2 ^= replaceAndRotate(n1 + key[6]);
    n1 ^= replaceAndRotate(n2 + key[7]);

    n2 ^= replaceAndRotate(n1 + key[0]);
    n1 ^= replaceAndRotate(n2 + key[1]);
    n2 ^= replaceAndRotate(n1 + key[2]);
    n1 ^= replaceAndRotate(n2 + key[3]);
    n2 ^= replaceAndRotate(n1 + key[4]);
    n1 ^= replaceAndRotate(n2 + key[5]);
    n2 ^= replaceAndRotate(n1 + key[6]);
    n1 ^= replaceAndRotate(n2 + key[7]);

    n2 ^= replaceAndRotate(n1 + key[0]);
    n1 ^= replaceAndRotate(n2 + key[1]);
    n2 ^= replaceAndRotate(n1 + key[2]);
    n1 ^= replaceAndRotate(n2 + key[3]);
    n2 ^= replaceAndRotate(n1 + key[4]);
    n1 ^= replaceAndRotate(n2 + key[5]);
    n2 ^= replaceAndRotate(n1 + key[6]);
    n1 ^= replaceAndRotate(n2 + key[7]);

    n2 ^= replaceAndRotate(n1 + key[7]);
    n1 ^= replaceAndRotate(n2 + key[6]);
    n2 ^= replaceAndRotate(n1 + key[5]);
    n1 ^= replaceAndRotate(n2 + key[4]);
    n2 ^= replaceAndRotate(n1 + key[3]);
    n1 ^= replaceAndRotate(n2 + key[2]);
    n2 ^= replaceAndRotate(n1 + key[1]);
    n1 ^= replaceAndRotate(n2 + key[0]);

    block = n2;
    return block << 32 | n1;
}

quint64 Gost::core32Decrypt(quint64 block) const
{
    quint32 n1 = block & 0xFFFFFFFF;
    quint32 n2 = block >> 32;

    n2 ^= replaceAndRotate(n1 + key[0]);
    n1 ^= replaceAndRotate(n2 + key[1]);
    n2 ^= replaceAndRotate(n1 + key[2]);
    n1 ^= replaceAndRotate(n2 + key[3]);
    n2 ^= replaceAndRotate(n1 + key[4]);
    n1 ^= replaceAndRotate(n2 + key[5]);
    n2 ^= replaceAndRotate(n1 + key[6]);
    n1 ^= replaceAndRotate(n2 + key[7]);

    n2 ^= replaceAndRotate(n1 + key[7]);
    n1 ^= replaceAndRotate(n2 + key[6]);
    n2 ^= replaceAndRotate(n1 + key[5]);
    n1 ^= replaceAndRotate(n2 + key[4]);
    n2 ^= replaceAndRotate(n1 + key[3]);
    n1 ^= replaceAndRotate(n2 + key[2]);
    n2 ^= replaceAndRotate(n1 + key[1]);
    n1 ^= replaceAndRotate(n2 + key[0]);

    n2 ^= replaceAndRotate(n1 + key[7]);
    n1 ^= replaceAndRotate(n2 + key[6]);
    n2 ^= replaceAndRotate(n1 + key[5]);
    n1 ^= replaceAndRotate(n2 + key[4]);
    n2 ^= replaceAndRotate(n1 + key[3]);
    n1 ^= replaceAndRotate(n2 + key[2]);
    n2 ^= replaceAndRotate(n1 + key[1]);
    n1 ^= replaceAndRotate(n2 + key[0]);

    n2 ^= replaceAndRotate(n1 + key[7]);
    n1 ^= replaceAndRotate(n2 + key[6]);
    n2 ^= replaceAndRotate(n1 + key[5]);
    n1 ^= replaceAndRotate(n2 + key[4]);
    n2 ^= replaceAndRotate(n1 + key[3]);
    n1 ^= replaceAndRotate(n2 + key[2]);
    n2 ^= replaceAndRotate(n1 + key[1]);
    n1 ^= replaceAndRotate(n2 + key[0]);

    block = n2;
    return block << 32 | n1;
}

quint64 Gost::xorEncrypt(quint64 block)
{
    quint64 encrGamma = core32Encrypt(nextGamma(1));
    return block ^ encrGamma;
}

quint64 Gost::xorDecrypt(quint64 block)
{
    if(currentGammaIndex < 0)
        fillGammaBatch(gammaBatchSize);

    quint64 encrGamma = core32Encrypt(gammaBatch.at(currentGammaIndex--));
    return block ^ encrGamma;
}

void Gost::encrypt(char *data, int size)
{
    quint64 block = 0;
    int bytesLeft;
    for(int i = 0; i < size; i += blockSize)
    {
        block = 0;
        bytesLeft = size - i < 8 ? size - i : 8;
        memcpy(&block, &data[i], bytesLeft);
        block = xorEncrypt(block);
        memcpy(&data[i], &block, bytesLeft);
    }
}

void Gost::decrypt(char *data, int size)
{
    quint64 block = 0;
    int bytesLeft;
    int i = size % blockSize != 0 ? size - (size % blockSize) : size - blockSize;
    for(; i >= 0; i -= blockSize)
    {
        block = 0;
        bytesLeft = size - i < 8 ? size - i : 8;
        memcpy(&block, &data[i], bytesLeft);
        block = xorDecrypt(block);
        memcpy(&data[i], &block, bytesLeft);
    }
}

void Gost::clearGammaArrays()
{
    gammaBatch.clear();
    gammaCheckpoints.clear();
}

void Gost::fillOptimizedRepTable()
{
    for(int x = 0; x < 8; x += 2)
    {
        for(int i = 0; i < 256; i++)
        {
            repTableOptimized[x / 2][i] = replacementTable[x][i >> 4] << 4 | replacementTable[x + 1][i & 0xF];
        }
    }
}

void Gost::fillGammaBatch(const int size)
{
    gammaBatch.clear();
	gammaBatch.reserve(size / blockSize + 1);
    gamma = gammaCheckpoints[currentGammaCheckpoint--];
    for(int i = 0; i < size; i += blockSize)
    {
        gammaBatch.push_back(nextGamma(1));
    }
    currentGammaIndex = gammaBatch.size() - 1;
}

#define C1 0x01010104
#define C2 0x01010101

quint64 Gost::nextGamma(const quint64 amount)
{    
    for(quint64 i = 0; i < amount; i++)
    {
        quint32 low = gamma & 0xFFFFFFFF;
        quint32 high = gamma >> 32;
        high += C1;
        if(high < C1)
            high++;
        low += C2;
        gamma = high;
        gamma = gamma << 32 | low;
    }    
    return gamma;
}

void Gost::setupGamma(qint64 containerSize)
{
    gammaCheckpoints.clear();
    currentGammaCheckpoint = 0;
    int lastBatchSize = containerSize % gammaBatchSize;
    if(lastBatchSize == 0)
        lastBatchSize = gammaBatchSize;
    int batchesAmount = containerSize / gammaBatchSize;
    if(lastBatchSize != gammaBatchSize)
        batchesAmount++;
    gammaCheckpoints.push_back(gamma);
    for(int i = 1; i < batchesAmount; i++)
    {
        gammaCheckpoints.push_back(nextGamma(gammaBatchSize / blockSize));
    }
    currentGammaCheckpoint = gammaCheckpoints.size() - 1;
    fillGammaBatch(lastBatchSize);
}

QByteArray Gost::generateInitVector()
{
    std::mt19937_64 randGenerator;
    std::uniform_int_distribution<quint64> distribution(0, std::numeric_limits<quint64>::max());
    gamma = distribution(randGenerator);

    QByteArray v(8, 0);
    quint64 temp = gamma;
    for(int i = 7; i >= 0; i--)
    {
        v[i] = temp & 0xFF;
        temp >>= 8;
    }

    return v;
}
