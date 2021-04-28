#ifndef CRC_H
#define CRC_H

#include <QObject>

namespace CRCMap
{

    quint32 GetCRCInitialValue();

    quint32 CalculateCRC(quint32 previous, const void* data, unsigned int dataLen);

    quint32 CalculateFinalCRC(quint32 crc);

    quint32 CalculateCRCForChunk(const void* data, unsigned int dataLen);

    quint32 CalculateCRCForFile(const QString& file, qint64 maxReadBytes, bool finalize);
}

#endif // CRC_H
