#ifndef CONTENTTAGUTILITY_H
#define CONTENTTAGUTILITY_H

#include <QString>
#include <QMap>

namespace Origin
{
    namespace Downloader
    {
        class ContentTagUtility
        {
            public:

                typedef QMap<QString, QString> TagBlock;
            
                ContentTagUtility();
                ~ContentTagUtility();

                void Start(TagBlock tag, QString filename, qint64 fileLen);
                void Resume(quint32 crc);
                void Update(void*& data, quint32& len, quint32 crc, qint64 filePos);

            private:
                void CreateTagData();
                static qint64 FindPattern(quint64& previous, const void* data, unsigned int dataLen);

            private:
                bool _active;
                bool _debug;
                bool _markerFound;
                bool _finished;
                TagBlock _tag;
                QString _filename;
                QByteArray _tagData;
                qint64 _fileLen;
                quint64 _lastMarkerWindow;
                quint32 _tagStartCRC;
                qint64 _tagStartPos;
                qint64 _currentPos;
                quint8 _tagBuffer[300];
                quint32 _tagBufferPos;
                quint8* _tagExtraBuffer;
        };

    } // Downloader
} // Origin

#endif //CONTENTTAGUTILITY_H