/*
#ifndef Z_QBYTEARRAY
#define Z_QBYTEARRAY

#include <QByteArray>
#include <windows.h>

class Z_QByteArray : public QByteArray
{
public:
    using QByteArray::QByteArray;
    //using QByteArray::operator=;
    Z_QByteArray& operator=(Z_QByteArray const &other)
    {
		this->QByteArray::operator =(other);
		return *this;
    }

    ~Z_QByteArray()
    {
        SecureZeroMemory(this->data(), this->size());
    }
};

#endif // Z_QByteArray
*/

