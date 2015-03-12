#include "Gost.h"

QByteArray Gost::encrypt(QByteArray data)
{
    quint64 temp = 0;
    for(int i = 0; i < data.size(); i++)
    {
        temp <<= 8;
        temp |= static_cast<quint8>(data[i]);
    }
    temp = core32Encrypt(temp);
    data.fill(0);
    for(int i = data.size() - 1; temp; i--)
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
    temp = core32Decrypt(temp);
    data.fill(0);
    for(int i = data.size() - 1; temp; i--)
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
        initVector = 0;
        for(int i = 0; i < 8; i++)
        {
            initVector <<= 8;
            initVector |= static_cast<quint8>(newInitVector[i]);
        }
        initVector = core32Encrypt(initVector);
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
    quint64 vector = nextVector(1);
    vector = core32Encrypt(vector);
    return block ^ vector;
}

quint64 Gost::nextVector(const quint64 amount)
{
    for(quint64 i = 0; i < amount; i++)
    {
        quint32 low = initVector & 0x00000000FFFFFFFF;
        quint32 high = initVector >> 32;
        initVector = static_cast<quint64>(high) + 0x1010104;
        if(initVector > 0xFFFFFFFE)
            initVector += 1;
        low += 0x1010101;
        initVector <<= 32;
        initVector |= low;
    }
    return initVector;
}

QByteArray Gost::generateInitVector()
{
    std::mt19937_64 randGenerator;
    std::uniform_int_distribution<quint64> distribution(0, std::numeric_limits<quint64>::max());
    initVector = distribution(randGenerator);

    QByteArray v(8, 0);
    quint64 temp = initVector;
    for(int i = 7; i >= 0; i--)
    {
        v[i] = temp & 0xFF;
        temp >>= 8;
    }

    initVector = core32Encrypt(initVector);

    return v;
}
