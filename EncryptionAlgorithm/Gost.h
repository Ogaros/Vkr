#ifndef GOST_H
#define GOST_H

#include <QObject>
#include <QByteArray>
#include <random>
#include <limits>
#include <vector>
#include <math.h>
#include <QMutex>
#include <QDebug>

class Gost : public QObject
{
	Q_OBJECT
public:
	explicit Gost(QObject *parent = 0);
	void simpleDecrypt(QByteArray &data);
	void simpleEncrypt(QByteArray &data);
	int getBlockSize(){ return blockSize; }
	bool setKey(QByteArray newKey);
	bool setInitVector(QByteArray newInitVector);
	void encrypt(char *data, int size);
	void decrypt(char *data, int size);
	void clearGammaArrays();
	quint64 nextGamma(const quint64 amount);
	void setupGamma(qint64 containerSize);
	QByteArray generateInitVector();

private:
	inline quint64 replaceAndRotate(quint32 block) const;
	quint64 xorEncrypt(quint64 block);
	quint64 xorDecrypt(quint64 block);
	quint64 core32Encrypt(quint64 block) const;
	quint64 core32Decrypt(quint64 block) const;
	void fillOptimizedRepTable();
	void fillGammaBatch(const int batchSize);

	const int blockSize = 8;

	quint32 key[8];
	quint64 gamma;
	std::vector<quint64> gammaCheckpoints;
	std::vector<quint64> gammaBatch;
	int currentGammaIndex;
	int currentGammaCheckpoint;
	QMutex mutex;
	static quint8 replacementTable[8][16];
	quint8 repTableOptimized[4][256];
	static const int gammaBatchSize;
};

#endif // GOST_H
