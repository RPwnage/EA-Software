///////////////////////////////////////////////////////////////////////////////
// SingleFileChunkMap.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef SINGLEFILECHUNKMAP_H
#define SINGLEFILECHUNKMAP_H

#include <QString>

namespace Origin
{
namespace Downloader
{
/// \brief Chunk map header.
struct chunkmap_header
{
    /// \brief Version of this chunk map header.
	int	Version;
    
    /// \brief The size of the associated file that the chunkmap is expected to map.
	qint64 Filesize;
    
    /// \brief The size of the chunk that is being mapped.
	int	Chunksize;

    /// \brief Total number of chunks in the map.
	int TotalChunks;

    /// \todo CRC of the chunk map.
	// TODO: CRC of chunk map
};

/// \brief Implementation of writing chunk maps for random writing of files.  Used to indicate which chunks have been written and which ones have not.
class SingleFileChunkMap
{
public:
    /// \brief Builds a path to the chunk map file for the given target file.
    /// \param localTargetFile The file to build the chunk map path for.
    /// \return The path to the chunk map filef or the given target file.
	static QString GetChunkMapFilename(const QString& localTargetFile);

    /// \brief The SingleFileChunkMap constructor.
    /// \param SingleFileChunkMapName The full or relative path to the chunk map.
    /// \param Filesize The size of the associated file that the chunkmap is expected to map.
    /// \param Chunksize The size of the chunk that is being mapped.
    /// \param logId Unique identifier for the content that should appear in logs.
	SingleFileChunkMap(const QString& SingleFileChunkMapName, qint64 Filesize, int Chunksize = 0, const QString& logId = "");

    /// \brief The SingleFileChunkMap destructor.
	~SingleFileChunkMap();

    /// \brief Initializes the chunk map.
    /// \return True if the chunk map was initialized successfully.
	bool Init();

    /// \brief Unmarks all marked chunks.
	void ResetMarkedChunkMap();

    /// \brief Gets the size of the associated file that the chunkmap is expected to map.
    /// \return The size of the associated file that the chunkmap is expected to map.
	qint64 GetFilesize() const;
    
    /// \brief Saves the chunk map from memory to disk.
    /// \return True if the chunk map was saved successfully.
	bool SaveSingleFileChunkMap();

    /// \brief Checks if the given byte is marked.
    /// \param ByteNumber The index of the byte to check.
    /// \return True if the byte is marked.
	bool IsByteMarked(qint64 ByteNumber) const;

    /// \brief Marks the byte at ByteNumber.
    /// \param ByteNumber The index of the byte to mark.
    /// \return True if the byte was marked successfully.
	bool MarkByte(qint64 ByteNumber);
    
    /// \brief Marks all bytes in the given range.
    /// \param StartByteIndex The index of the byte to start from.
    /// \param EndByteIndex The index of the byte to finish at.
    /// \return True if the bytes were marked successfully.
	bool MarkRange(qint64 StartByteIndex, qint64 EndByteIndex);
	
    /// \brief Finds the next unmarked byte starting at MarkedByteNumber.
    /// \param MarkedByteNumber The byte index from which to start the search.
    /// \return The byte index of the next unmarked byte.
	qint64 FindNextUnmarkedByte(qint64 MarkedByteNumber) const;

    /// \brief Gets the number of unmarked chunks.
    /// \return The number of unmarked chunks.
	int CountUnmarkedChunks() const;
    
    /// \brief Gets the size of the chunk that is being mapped.
    /// \return The size of the chunk that is being mapped.
	int GetChunkSize() const;
	
    /// \brief Gets the size of all unmarked chunks.
    /// \return The size of all unmarked chunks.
	qint64 GetUnmarkedSize() const;
    
    /// \brief Gets the size of all marked chunks.
    /// \return The size of all marked chunks.
	qint64 GetMarkedSize() const;

private:
    /// \brief Opens the chunk map file and attempts to read the header.
    /// \return True if the header was read successfully.
	bool ReadSingleFileChunkMap_Header();

    /// \brief Calculates the optimal size for a chunk for this map.
	void DetermineChunkSize();

    /// \brief Allocates memory for the chunk map.
    /// \return True if memory was successfully allocated.
	bool AllocChunkMap();

    /// \brief Checks if the chunk at index ChunkIndex has been marked.
    /// \param ChunkIndex The index of the chunk to check.
    /// \return True if the chunk has been marked.
	bool IsChunkMarked(int ChunkIndex) const;

    /// \brief Marks the chunk at index ChunkIndex.
    /// \param ChunkIndex The index of the chunk to mark.
    /// \return True if the chunk was marked successfully.
	bool MarkChunk(int ChunkIndex);
    
    /// \brief Finds the index of the next unmarked chunk from MarkedChunkIndex.
    /// \param MarkedChunkIndex The index of the chunk from which to start the search.
    /// \return The index of the next unmarked chunk.
	qint64 FindNextUnmarkedChunk(int MarkedChunkIndex) const;

    /// \brief Converts a chunk index to a byte index.
    /// \param ChunkNo The index of the chunk.
    /// \return The index of the byte.
	qint64 ChunkToByte(int ChunkNo) const;
    
    /// \brief Converts a byte index to a chunk index.
    /// \param ByteNo The index of the byte.
    /// \return The index of the chunk.
	int ByteToChunk(qint64 ByteNo) const;

private:
    /// \brief Number of chunks to process between saves.
	static const int MAX_UNSAVED_CHUNKS;

    /// \brief File extension for all chunk map files.
	static const QString CHUNK_MAP_FILE_EXTENSION;

    /// \brief True if the map was initialized successfully.
	bool mbInitialized;
    
    /// \brief The full or relative path to the chunk map.
	QString mSingleFileChunkMapFilename;

    /// \brief Chunk map header.
	chunkmap_header mCMH;

    /// \brief Pointer to memory containing our chunk map.
	uchar *mpSingleFileChunkMap;

    /// \brief Length of the chunk map in bytes.
	int mSingleFileChunkMapLen;

    /// \brief Number of chunks that have been marked.
	int mChunksMarked;

    /// \brief Number of chunks processed since last save.
	int mChunksSinceLastSave;

    /// \brief Unique identifier for the content that should appear in logs.
	QString mLogId;
};
}
}

#endif

