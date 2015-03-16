#include "Gost.h"

QByteArray Gost::encrypt(QByteArray data)
{
    quint64 temp = 0;
    for(int i = 0; i < data.size(); i++)
    {
        temp <<= 8;
        temp |= static_cast<quint8>(data[i]);
    }
    temp = xorEncrypt(temp);
    data.fill(0);
    for(int i = data.size() - 1; i >= 0; i--)
    {
        data[i] = temp & 0xFF;
        temp >>= 8;
    }
    return data;
}

QByteArray Gost::decrypt(QByteArray data)
{
    quint64 temp = 0;
    for(int i = 0; i < data.size(); i++)
    {
        temp <<= 8;
        temp |= static_cast<quint8>(data[i]);
    }
    temp = xorDecrypt(temp);
    data.fill(0);
    for(int i = data.size() - 1; i >= 0; i--)
    {
        data[i] = temp & 0xFF;
        temp >>= 8;
    }
    return data;
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
        gamma = core32Encrypt(gamma);
        return true;
    }
    else
        return false;
}

quint64 Gost::coreStep(const quint64 block, const int keyPart) const
{
    quint64 result = 0L;
    quint32 low = block & 0x00000000FFFFFFFF;
    quint32 high = block >> 32;
    quint32 temp;

    result |= low;
    result <<= 32;

    low += key[keyPart];

    for(int i = 0; i < 8; i++)
    {
        temp = (low >> i * 4) & 0x0000000F;
        temp = replacementTable[i][temp];
        low = (low & ~(0x0000000F << i * 4)) | (temp << i * 4);
    }

    low = (low << 11) | (low >> 21);

    low ^= high;

    result |= low;

    return result;
}

quint64 Gost::core32Encrypt(quint64 block) const
{
    for(int i = 0; i < 24; i++)
    {
        block = coreStep(block, i % 8);
    }
    for(int i = 7; i >= 0; i--)
    {
        block = coreStep(block, i);
    }
    block = (block << 32) | (block >> 32);
    return block;
}

quint64 Gost::core32Decrypt(quint64 block) const
{
    for(int i = 0; i < 8; i++)
    {
        block = coreStep(block, i);
    }
    for(int i = 23; i >= 0; i--)
    {
        block = coreStep(block, i % 8);
    }
    block = (block << 32) | (block >> 32);
    return block;
}

quint64 Gost::xorEncrypt(quint64 block)
{
    quint64 vector = nextGamma(1);
    vector = core32Encrypt(vector);
    return block ^ vector;
}

quint64 Gost::xorDecrypt(quint64 block)
{
//    if(currentGammaIter == gammaBatch.rend())
//        fillGammaBatch(batchSize);
//    quint64 temp = core32Encrypt(*currentGammaIter);
//    currentGammaIter++;
    quint64 temp = nextGamma(1);
    temp = core32Encrypt(temp);
    currentGammaIter->second -= blockSize;
    if(currentGammaIter->second <= 0)
    {
        currentGammaIter++;
        if(currentGammaIter != gammaBatch.rend())
            gamma = currentGammaIter->first;
    }
    return block ^ temp;
}

void Gost::fillGammaBatch(const int size)
{
//    gammaBatch.clear();
//    gammaBatch.reserve(size);
//    gamma = gammaCheckpoints[currentGammaCheckpoint];
//    for(int i = 0; i < size; i += blockSize)
//    {
//        gammaBatch.push_back(nextGamma(1));
//    }
//    currentGammaIter = gammaBatch.rbegin();
}

quint64 Gost::nextGamma(const quint64 amount)
{
    static int x = 0;
    for(quint64 i = 0; i < amount; i++)
    {
        quint32 low = gamma & 0x00000000FFFFFFFF;
        quint32 high = gamma >> 32;
        gamma = static_cast<quint64>(high) + 0x1010104;
        if(gamma > 0xFFFFFFFE)
            gamma += 1;
        low += 0x1010101;
        gamma <<= 32;
        gamma |= low;
        x++;
    }    
    return gamma;
}

void Gost::setupGamma(qint64 containerSize, int batchSize)
{
    this->batchSize = batchSize;
    gammaBatch.clear();
    //gammaCheckpoints.clear();
    //currentGammaCheckpoint = 0;
    int lastBatchSize = containerSize % batchSize;
    if(lastBatchSize == 0)
        lastBatchSize = batchSize;
    int batchesAmount = containerSize / batchSize;
    if(lastBatchSize != batchSize)
        batchesAmount++;
    //gammaCheckpoints.push_back(gamma);
    gammaBatch.emplace_back(gamma, batchesAmount == 1 ? lastBatchSize : batchSize);
    for(int i = 1; i < batchesAmount; i++)
    {
        //gammaCheckpoints.push_back(nextGamma(batchSize));
        gammaBatch.emplace_back(nextGamma(batchSize / blockSize), batchSize);
    }
    gammaBatch.emplace_back(nextGamma(lastBatchSize / blockSize), 1);
    currentGammaIter = gammaBatch.rbegin();
    //currentGammaCheckpoint = gammaCheckpoints.size() - 1;
    //fillGammaBatch(lastBatchSize);
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

    gamma = core32Encrypt(gamma);

    return v;
}
