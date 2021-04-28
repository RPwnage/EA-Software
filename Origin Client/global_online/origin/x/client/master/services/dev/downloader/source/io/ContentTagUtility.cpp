#include "io/ContentTagUtility.h"
#include "services/crypto/CryptoService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/downloader/CRCMap.h"
#include "services/settings/SettingsManager.h"

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

#define PATTERN_NOT_FOUND -999

namespace Origin
{
namespace Downloader
{
    const int kContentTagMarkerLength = 8;
    const int kContentTagMaxPlaintextLength = 110; // RSA lets us have max 117 with PKCS#1 v1.5 Padding, choose 110 to be safe
    const int kContentTagStartOffset = 64;
    const int kContentTagLength = 128; // Limited to 128 bytes of cipher text by RSA 1024bit
    const int kContentTagLengthWithFixup = kContentTagLength + 4; // CRC fixup requires 4 bytes
    const int kContentTagEndOffset = kContentTagStartOffset + kContentTagLengthWithFixup; // This value absolutely must be less than 250, since that is all we have to work with
    const quint8 kCheckBytes[4] = { 0x38, 0x38, 0x61, 0x39 }; // These are the bytes that should appear underneath the CRC fixup we will write.  If any values above are changed, this needs to be updated.

    ContentTagUtility::ContentTagUtility() : 
        _active(false),
        _debug(false),
        _markerFound(false),
        _finished(false),
        _fileLen(0),
        _lastMarkerWindow(0),
        _tagStartCRC(0),
        _tagStartPos(0),
        _currentPos(0),
        _tagBufferPos(0),
        _tagExtraBuffer(NULL)
    {
        // Clear the extra tag buffer
        memset(_tagBuffer, 0, sizeof(_tagBuffer));

        // If the QA 'force watermarks' setting is on, we want debug output ; Otherwise this module should do its work silently
        _debug = Origin::Services::readSetting(Origin::Services::SETTING_ForceContentWatermarking, Origin::Services::Session::SessionRef());
    }

    ContentTagUtility::~ContentTagUtility()
    {
        if (_tagExtraBuffer)
        {
            delete [] _tagExtraBuffer;
            _tagExtraBuffer = NULL;
        }
    }

    void ContentTagUtility::Start(TagBlock tag, QString filename, qint64 fileLen)
    {
        _active = true;
        _markerFound = false;
        _finished = false;
        _tag = tag;
        _filename = filename;
        _fileLen = fileLen;
    }

    void ContentTagUtility::Resume(quint32 crc)
    {
        // If we weren't activated, bail
        if (!_active)
            return;

        // In order to resume, we really only need to grab the last 8 bytes from the file and check them for the pattern
        // During normal operation, we would not write any bytes in the tag block until we had updated the entire tag block

        // If the file is missing, nothing we can do
        if (!QFile::exists(_filename))
	    {
            if (_debug)
                ORIGIN_LOG_WARNING << "[Fingerprint] Pattern scanning cannot resume for missing file: " << _filename;

		    return;
	    }

        // Symlinks are processed a bit differently than regular files - the content of a CRC is
        // the path to what it points to
        //
        QFileInfo finfo (_filename);    
        if (finfo.isSymLink())
        {
            // Not interested in symlinks
            return;
        }

        QFile resumeFile(_filename);
	    qint64 fileSize = resumeFile.size();

	    if (fileSize == 0)
        {
            if (_debug)
                ORIGIN_LOG_WARNING << "[Fingerprint] Pattern scanning cannot resume, file empty: " << _filename;
		    return;
        }

	    if (!resumeFile.open(QIODevice::ReadOnly))
	    {
            if (_debug)
                ORIGIN_LOG_WARNING << "[Fingerprint] Pattern scanning cannot resume, cannot open file: " << _filename;
		    return;
	    }

        qint64 seekOffset = fileSize - 8;
        if (seekOffset < 0)
            seekOffset = 0;

        if (!resumeFile.seek(seekOffset))
        {
            if (_debug)
                ORIGIN_LOG_WARNING << "[Fingerprint] Pattern scanning cannot resume, cannot seek to offset: " << seekOffset << " in file: " << _filename;
            return;
        }

        // Grab (up to) the last 8 bytes of the file
        QByteArray fileData = resumeFile.read(fileSize - seekOffset);
        if (fileData.size() != (fileSize - seekOffset))
        {
            if (_debug)
                ORIGIN_LOG_WARNING << "[Fingerprint] Pattern scanning cannot resume, could not read " << (fileSize - seekOffset) << " bytes from file: " << _filename;
            return;
        }

        // We're done with the file
        resumeFile.close();

        // Look for the pattern in the last bytes
        qint64 patternOffset = FindPattern(_lastMarkerWindow, fileData.constData(), fileData.size());
        if (patternOffset != PATTERN_NOT_FOUND)
        {
            if (_debug)
                ORIGIN_LOG_EVENT << "[Fingerprint] Pattern scanning resumed; Found pattern in file: " << _filename << " at relative chunk offset: " << patternOffset << " absolute offset: " << patternOffset + seekOffset;

            // We found the pattern, so prepare the tag data
            CreateTagData();

            _markerFound = true;

            _tagStartPos = patternOffset + seekOffset + kContentTagMarkerLength; // Don't include the marker itself (8 bytes)
            _currentPos = fileSize;

            // The tag should start right at the end of the file since we only read 8 bytes and the tag itself is 8 bytes
            ORIGIN_ASSERT(_tagStartPos == _currentPos);

            // The passed-in CRC is already the CRC we need
            _tagStartCRC = crc;
        }
        else
        {
            if (_debug)
                ORIGIN_LOG_EVENT << "[Fingerprint] Pattern scanning resumed; No pattern found yet in partial file: " << _filename;
        }
    }

    void ContentTagUtility::Update(void*& data, quint32& len, quint32 crc, qint64 filePos)
    {
        // Only bother if we've been activated
        if (!_active)
            return;

        // Nothing to do, we're already done
        if (_finished)
        {
            // We can dispose of the extra buffer if we no longer need it
            if (_tagExtraBuffer)
            {
                delete [] _tagExtraBuffer;
                _tagExtraBuffer = NULL;
            }
            return;
        }

        if (!_markerFound)
        {
            qint64 patternOffset = FindPattern(_lastMarkerWindow, data, len);
            if (patternOffset != PATTERN_NOT_FOUND)
            {
                if (_debug)
                    ORIGIN_LOG_EVENT << "[Fingerprint] Found pattern in file: " << _filename << " at relative chunk offset: " << patternOffset << " absolute offset: " << patternOffset + filePos;

                // We found the pattern, so prepare the tag data
                CreateTagData();

                _markerFound = true;

                _tagStartPos = patternOffset + filePos + kContentTagMarkerLength; // Don't include the marker itself (8 bytes)
                _currentPos = filePos;

                // Calculate the CRC up until the end of the marker
                _tagStartCRC = CRCMap::CalculateCRC(crc, data, patternOffset + kContentTagMarkerLength);
            }
        }

        // Allow fallthrough
        // If we found the marker and have valid tag data
        if (_markerFound && _tagData.size() == kContentTagLength)
        {
            // Are we missing part of the data we need for tagging?  If so, save the bit we need to a temporary location and truncate the buffer
            qint64 tagStartRelativeOffset = _tagStartPos - _currentPos;
            qint64 adjustedLen = ((_tagBufferPos > 0) ? len : (len - tagStartRelativeOffset)); // For the initial sliver, we aren't interested in the part before the tag start
            qint64 streamOffsetToEndOfTagAndFixup = _tagStartPos + kContentTagEndOffset;
            qint64 streamOffsetToStartOfCurrentBuffer = _currentPos + tagStartRelativeOffset + _tagBufferPos;
            qint64 streamOffsetToEndOfCurrentBuffer = streamOffsetToStartOfCurrentBuffer + adjustedLen;

            // Sanity check, make sure the tag would not extend beyond the end of the file
            if (streamOffsetToEndOfTagAndFixup > _fileLen)
            {
                // Don't process any data, mark ourselves finished and return
                _finished = true;
                return;
            }

            // See if the tag length goes beyond what we currently have in the input buffer (we need to accumulate and wait for more data)
            if (streamOffsetToEndOfTagAndFixup > streamOffsetToEndOfCurrentBuffer)
            {
                // Get the portion after the marker till the end
                qint64 sizeToCopy = streamOffsetToEndOfCurrentBuffer - streamOffsetToStartOfCurrentBuffer;

                // Make sure we aren't too large for our buffer (this should be impossible)
                ORIGIN_ASSERT(sizeToCopy > 0 && sizeToCopy < (sizeof(_tagBuffer) - _tagBufferPos) && (len - sizeToCopy) >= 0);

                // Copy from the source buffer to the temporary buffer (at the current position)
                quint8* tagBufferTarget = (quint8*)_tagBuffer + _tagBufferPos;
                quint8* tagBufferSource = (quint8*)data + (len - sizeToCopy);
                memcpy(tagBufferTarget, tagBufferSource, sizeToCopy);

                // Update our temporary buffer pointer
                _tagBufferPos += static_cast<quint32>(sizeToCopy);

                // Return the adjusted amount of data
                len = len - sizeToCopy;
                return;
            }

            // We made it this far, so we have at least enough buffer to do what we need to do
            // If we were holding onto temporary tag data, combine that with the new buffer before proceeding
            if (_tagBufferPos > 0)
            {
                // Allocate a new buffer that combines the newly received data with the temporary tag buffer
                size_t newSizeRequired = _tagBufferPos + len;
                _tagExtraBuffer = new quint8[newSizeRequired];

                // Copy the temporary tag data first
                memcpy(_tagExtraBuffer, _tagBuffer, _tagBufferPos);

                // Copy the newly received data in after
                memcpy(_tagExtraBuffer + _tagBufferPos, data, len);

                // Replace the data/len with our new buffer (these are passed by reference)
                // IMPORTANT: We cannot clean this up until the API client is done with them (maybe not until we are destructed, maybe sooner)
                data = _tagExtraBuffer;
                len = newSizeRequired;

                // Blank out the relative offset, because we don't store it in our temporary buffer
                tagStartRelativeOffset = 0;
            }

            // Get a pointer to the beginning of the tag block
            quint8 *tagBlock = ((quint8*)data) + tagStartRelativeOffset;

            // First sanity check to make sure that the last 4 (fixup) bytes contain what we think they should
            // Otherwise, we may have encountered a truncated fingerprint, and we won't write anything
            quint8 *checkBlock = tagBlock + kContentTagStartOffset + kContentTagLength;
            if ( kCheckBytes[0] != checkBlock[0]
              || kCheckBytes[1] != checkBlock[1]
              || kCheckBytes[2] != checkBlock[2]
              || kCheckBytes[3] != checkBlock[3] )
            {
                if (_debug)
                    ORIGIN_LOG_WARNING << "[Fingerprint] Unable to fingerprint " << _filename << " due to corrupt/truncated pattern block.";
            }
            else
            {
                // Calculate the partial CRC of the unmodified data at the end of what will contain the tag+fixup
                quint32 targetCRC = CRCMap::CalculateCRC(_tagStartCRC, tagBlock, kContentTagStartOffset + kContentTagLengthWithFixup);

                // Insert the tag payload at the start offset (beginning of tag + skip over existing watermark(s))
                memcpy(tagBlock + kContentTagStartOffset, _tagData.constData(), kContentTagLength);

                // Calculate the partial CRC of the now-modified data at the beginning of where we will place the fixup
                quint32 startCRC = CRCMap::CalculateCRC(_tagStartCRC, tagBlock, kContentTagStartOffset + kContentTagLength);

                // Calculate the 4 bytes required to fix up the CRC
                quint8 crcPatchBytes[4] = { 0 };
                CRCMap::CalculateCRCPatchBytes(startCRC, targetCRC, crcPatchBytes);

                // Insert the patch after the tag payload
                memcpy(tagBlock + kContentTagStartOffset + kContentTagLength, crcPatchBytes, 4);

                if (_debug)
                    ORIGIN_LOG_EVENT << "[Fingerprint] Successfully fingerprinted: " << _filename;
            }

            _finished = true;
        }
    }

    void ContentTagUtility::CreateTagData()
    {
        // Build the plaintext tag
        QJsonObject root;
        QJsonObject instanceObj;
        for (TagBlock::iterator iter = _tag.begin(); iter != _tag.end(); ++iter)
        {
            root[iter.key()] = iter.value();
        }
        QJsonDocument tagDoc(root);
        QByteArray tagDataPlaintext = tagDoc.toJson(QJsonDocument::Indented);

        // 1024-bit public key = Max 128 bytes of encrypted data, but because of padding, we are really limited to ~110 bytes
        // There is no need to keep this key secret, since we are using asymmetric encryption
        QString pubKey = "-----BEGIN PUBLIC KEY-----\n"
                            "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDC2G1W8RGZSo2zq2D5ksIFyI+I\n"
                            "vkPSB23oQufWhGZ3wxXWq9qLbh+dPfVKs8jGAk+P0L56PStvd5/BUp5271bxqpUO\n"
                            "j+T9sLw9VgnozzfaZ+mMWQ+GGrZxjlga39k+NDAaKk/HzdSZKL6jqajtMrr8e+Y6\n"
                            "IsDA4fzUfZ3BcncZ2QIDAQAB\n"
                            "-----END PUBLIC KEY-----\n";

        // RSA limits us to ~110 bytes with padding
        ORIGIN_ASSERT(tagDataPlaintext.length() <= kContentTagMaxPlaintextLength);
        if (tagDataPlaintext.length() > kContentTagMaxPlaintextLength)
        {
            tagDataPlaintext.truncate(kContentTagMaxPlaintextLength);
        }

        if (Services::CryptoService::encryptAsymmetricRSA(_tagData, tagDataPlaintext, pubKey.toLocal8Bit()) != -1)
        {
            if (_debug)
            {
                // For debugging purposes, write the watermark directly next to the file
                QString watermarkFile = _filename + "_wmk";
                QFile out(watermarkFile);
                out.open(QIODevice::WriteOnly);
                out.write(_tagData);
                out.close();
                ORIGIN_LOG_EVENT << "[Fingerprint] Wrote encrypted tag data to: " << watermarkFile;
            }
        }
        else
        {
            // Could not encrypt with RSA
            if (_debug)
                ORIGIN_LOG_WARNING << "[Fingerprint] RSA encryption of tag block failed for file: " << _filename;
        }
    }

    qint64 ContentTagUtility::FindPattern(quint64& previous, const void* data, unsigned int dataLen)
    {
        const quint64 pattern = 0x7861333764643435;

        unsigned char *dataPtr = (unsigned char*)data;

        // Start at -8 to account for the previous window, which may contain a partial match (sizeof quint64 == 8)
        int foundIdx = -8;

        // Move the new bytes into the window
        for (int c = 0; c < ((int)dataLen); ++c)
        {
            // Shift the previous data left by one byte
            previous <<= 8;

            // Promote the next byte to a 64-bit value and binary OR it on to the least significant digits of the previous window
            previous |= static_cast<quint64>(dataPtr[c]);

            // Move the possible found index up by one
            ++foundIdx;

            // If we matched the pattern we're looking for, just return now
            if (previous == pattern)
            {
                return foundIdx;
            }
        }

        // Didn't find it yet
        return PATTERN_NOT_FOUND;
    }
}//Downloader
}//Origin