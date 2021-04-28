//
// bChunk.hpp
//
// EA Black Box
//

#ifndef BCHUNK_HPP
#define BCHUNK_HPP

// How chunks work:
//
// - Chunks are intended as a way of defining a file format that is easy for all
//   our tools to work on.
// - Chunks are 8 byte headers with a 32 bit ID and a Size field.
// - The ID types are user defined and listed in this header file.  (NFS ID's are listed in SpeedChunks.hpp)
// - The size field contains the size of the data in the chunk not including the 8 byte header.
// - By convention, chunks contain either data or more nested chunks but not a mix of both.
//   Chunk ID's have a bit reserved (NESTED_BCHUNK) to indicate if the chunk contains data or nested chunks.
// - Optional alignment is implemented with chunk->GetAlignedData() and chunk->GetAlignedSize().  The tool
//   writing the chunk adds alignment padding.
// - If chunks need to be moved to a different file in the data pipeline it is important that the original alignment
//   be kept.  This needs to be done by adding a BCHUNK_NULL.
//

#if 0
// Here is some helpful advice on how to write chunks from a tool
// with alignment considerations.

// These functions exist in bWare\Inc\Parse.hpp and bWare\inc\bChunk.hpp


// Typical writing of a chunk.
int magic = WriteChunkHeader(file, BCHUNK_MYCHUNK);	// Writes the bChunk header to 'file'.
FWriteSafe(file, &my_header, sizeof(my_header); // FWriteSafe is a replacement for fwrite() that checks for errors.
FWriteSafe(file, data, sizeof_data); // Write some more stuff to the file.
WriteChunkSize(file, magic);	// This call puts the size of the chunk into the chunk header for you.


// Typical writing nested chunks.
// This example shows a nested BCHUNK_TEXTURE_LIST that contains a list of BCHUNK_TEXTURE chunks.
int root_chunk_magic = WriteChunkHeader(file, BCHUNK_TEXTURE_LIST);  // BCHUNK_TEXTURE_LIST must have BCHUNK_NESTED bit set in chunk ID.
for (Texture *texture = TextureList.GetHead(); texture != TextureList.EndOfList(); texture = texture->GetNext())
{
	// Write the children chunk
	int magic = WriteChunkHeader(file, BCHUNK_TEXTURE);
	FWriteSafe(file, texture->GetData(), texture->GetDataSize());
	WriteChunkSize(file, magic);
}
WriteChunkSize(file, root_chunk_magic);


// Run time loading and traversal of nested chunks.
// If your chunk data needs to be aligned, see the example below.
int32 size;
bChunk *chunks = (bChunk *)bGetFile(filename, &size);	// Load file
for (bChunk *chunk = chunks; chunk < GetLastChunk(chunks, size); chunk = chunk->GetNext())
{
	if (chunk->GetID() == BCHUNK_TEXTURE_LIST)
	{
		// BCHUNK_TEXTURE_LIST has nested chunks inside.  Here we traverse them.
		for (bChunk *texture_chunk = chunk->GetFirstChunk(); texture_chunk < chunk->GetLastChunk(); chunk = chunk->GetNext())
		{
			TextureImage *texture_image = (TextureImage *)chunk->GetData();
			LoadTextureImage(texture_image);
			texture_chunk = texture_chunk->GetNext();
		}
	}
}


// Important notes on alignment!
//
// Alignment of chunks is not very difficult if you use the provided functions.
// Here is how they work:
//
// - When loading your chunk file, the file itself must be aligned to your largest required alignment.
//
// - When writing your chunk, call WriteChunkHeaderAligned(int alignment_size) (In bWare\Inc\Parse.hpp).
//   This will write padding bytes after the chunk header to make your data aligned.  Now you must
//   use bChunk->GetAlignedData(int alignment_size) and bChunk->GetAlignedSize() to access your chunk.
//

// Example writing a 128 byte aligned chunk.
int magic = WriteChunkHeaderAligned(file, BCHUNK_PS2_TEXTURE, 128);
FWriteSafe(file, data, sizeof_data); // This data will become 128 byte aligned.
FWriteSafe(file, more_data, sizeof_more_data);
WriteChunkSize(file, magic);


// Example reading a chunk file containing chunks that require alignment
int32 size;
bChunk *chunks = (bChunk *)bGetFile(filename, &size, MEMORY_MAIN_ALIGN128);	// 'chunks' will be 128 byte aligned.
for (bChunk *chunk = chunks; chunk < GetLastChunk(chunks, size); chunk = chunk->GetNext())
{
	if (chunk->GetID() == BCHUNK_PS2_TEXTURE)
	{
		// This chunk was written with 128 byte alignment so we must access it with 128 byte alignment.
		uint128 *texture_data = (uint128 *)chunk->GetAlignedData(128);
		int sizeof_texture_data = chunk->GetAlignedSize(128);
		DMATextureData(texture_data, sizeof_texture_data);
	}
	else if (chunk->GetID() == BCHUNK_SOMEOTHER_CHUNK)
	{
		// This chunk was not saved with alignemnt so we can access it normally.
		char *data = (char *)chunk->GetData();
		int size = chunk->GetSize();
		...
	}
}

//
// Example using bChunkLoader functions
//
int MyLoader(bChunk *chunk)
{
	if (chunk->GetID() == BCHUNK_MYCHUNK)
	{
		... load
		return 1;
	}
	return 0;
}

int MyUnloader(bChunk *chunk)
{
	if (chunk->GetID() == BCHUNK_MYCHUNK)
	{
		... unload
		return 1;
	}
	return 0;
}

// Decare global instance
bChunkLoader bChunkLoaderMyChunk(BCHUNK_MYCHUNK, MyLoader, MyUnloader);

// Load file & call loaders
int32 size;
bChunk *chunks = (bChunk *)bGetFile(filename, &size, MEMORY_MAIN_ALIGN128);	// 'chunks' will be 128 byte aligned.
int error_id = bChunkLoader::CallLoaders(chunks, size);

// Unload
int error_id = bChunkLoader::CallUnloaders(chunks, size);

#endif


// Chunks are either NESTED_BCHUNK (contains no data) or DATA_BCHUNK
#define NESTED_BCHUNK				0x80000000
#define DATA_BCHUNK					0x00000000

// This value is used to pad chunks to their desired alignment.
#define BCHUNK_ALIGNMENT_PADDING	0x11111111

#ifndef BWARE_HPP
#define bFAssert  // Remove safety checking if somebody is not using bWare library
#endif


struct bChunk
{
	unsigned int	ID;		// Unique chunk ID. 
	int				Size;	// Size of data not including this chunk header.

	bChunk() {}
	bChunk(unsigned int id, int size)				{ID = id; Size = size;}
	unsigned int GetID()							{return ID;}
	int GetSize()									{return Size;}
	int IsNestedChunk()								{return GetID() & NESTED_BCHUNK;}
	int IsDataChunk()								{return !IsNestedChunk();}
	int CountChildren()								{if( IsNestedChunk()) { int ret = 0; for( bChunk* c = GetFirstChunk(); c < GetLastChunk(); c=c->GetNext()) ret++; return( ret);} else return( 0); }
	char *GetData()									{bFAssert(IsDataChunk()); return (char *)(this + 1);}  // Assuming the chunk contains data.
	char *GetAlignedData(int alignment_size)		{return (char *)(((int)GetData() + alignment_size - 1) & ~(alignment_size - 1));}
	int GetAlignedSize(int alignment_size)			{return Size - ((int)GetAlignedData(alignment_size) - (int)GetData());}	// Return size of aligned portion of chunk
	void VerifyAlignment(int alignment_size)		{unsigned int *pdata = (unsigned int *)GetAlignedData(alignment_size); bFAssert(*pdata != BCHUNK_ALIGNMENT_PADDING); if (pdata > (unsigned int *)GetData()) {bFAssert(*(pdata - 1) == BCHUNK_ALIGNMENT_PADDING);} }
	bChunk *GetFirstChunk()							{bFAssert(IsNestedChunk()); return (bChunk *)(this + 1);}	// Assuming chunk contains nested chunks
	bChunk *GetLastChunk()							{return (bChunk *)((int)this + Size + sizeof(bChunk));}		// Assuming we have nested chunks inside
	bChunk *GetNext()								{return GetLastChunk();}  // Points to next chunk.
};

// Here's how you can calculate the last chunk if you have a file containing chunks.
inline bChunk *GetLastChunk(bChunk *first_chunk, int sizeof_binary) {return (bChunk *)((int)first_chunk + sizeof_binary);}

// Call this to verify chunks are valid.
bool bValidateChunks(bChunk *first_chunk, int sizeof_chunks);

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//                              bChunkLoader
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

typedef int (*bChunkLoaderFunction)(bChunk *chunk);
#define BCHUNK_LOADER_TABLE_SIZE	64

class bChunkLoader
{
public:
	bChunkLoader(unsigned int id, bChunkLoaderFunction loader, bChunkLoaderFunction unloader);
	bChunkLoaderFunction GetLoaderFunction()		{return LoaderFunction;}
	bChunkLoaderFunction GetUnloaderFunction()		{return UnloaderFunction;}
	static unsigned int GetHash(unsigned int id)	{return (id + (id >> 6) + (id >> 12)) & (BCHUNK_LOADER_TABLE_SIZE - 1);}
	static bChunkLoader *FindLoader(unsigned int id);
	static unsigned int CallLoaders(bChunk *chunks, int sizeof_chunks, bool abort_on_error = true);		// If loader fails, returns chunk id of failed loader.
	static unsigned int CallUnloaders(bChunk *chunks, int sizeof_chunks, bool abort_on_error = true);	// If unloader fails, returns chunk id of failed loader.

private:
	bChunkLoader *Next;		// Singly linked list.
	unsigned int ID;
	bChunkLoaderFunction LoaderFunction;
	bChunkLoaderFunction UnloaderFunction;
	static bChunkLoader *sLoaderTable[BCHUNK_LOADER_TABLE_SIZE];
	static unsigned char sNumLoaders[BCHUNK_LOADER_TABLE_SIZE];	// Debugging stats
};


// ----------------------------------------------------------------------------------------------------

class bChunkCarpHeader
{
	public:

		enum kCarpHeaderFlags
		{
			kResolved = 1,
		};

		bChunkCarpHeader() :
		  mCrpSize(0),
		  mSectionNumber(0),
		  mFlags(0),
		  mLastAddress(0x0)
		{ 
			
		}
		~bChunkCarpHeader()		{ }

		int GetCarpSize(void) const						{ return mCrpSize; }
		int GetSectionNumber(void) const				{ return mSectionNumber; }
		int GetFlags(void) const						{ return mFlags; }
		bool IsResolved(void) const						{ return mFlags & kResolved; }
		void SetResolved(void)							{ mFlags|= kResolved; }
		void SetNotResolved(void)						{ mFlags= 0; }
		bChunkCarpHeader * GetLastAddress(void) const	{ return mLastAddress; }
		void SetLastAddress(bChunkCarpHeader * address) { mLastAddress = address; }
		void PlatformEndianSwap(void);

	protected:

	private:

		int mCrpSize;
		int mSectionNumber;
		int mFlags;
		bChunkCarpHeader * mLastAddress;
};


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//                              Chunk ID's
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// These are the user defined chunk ID's.  The numbers are arbitrary but make sure
// you don't accidentally make a duplicate.
//
// All ID's must have DATA_BCHUNK or NESTED_BCHUNK added to them to indicate
// whether the chunk contains a list of more chunks or data.
//

// Generic Chunks used by anybody
#define BCHUNK_NULL				0x00000	+ DATA_BCHUNK	// A chunk with undefined data.
#define BCHUNK_NAME				0x00001 + DATA_BCHUNK	// Null terminated string.
#define BCHUNK_FILE				0x00002 + DATA_BCHUNK	// Holds a file
#define BCHUNK_LONG_NUMBER		0x00003 + DATA_BCHUNK	// Holds a long
#define BCHUNK_FLOAT_NUMBER		0x00004 + DATA_BCHUNK	// Holds a float

// bGeom chunks.   See bGeom.cpp
#define BCHUNK_BPOLY			0x10001 + DATA_BCHUNK	// Memory image of bPoly class
#define BCHUNK_BMESH			0x10100 + NESTED_BCHUNK	// Holder of chunks for a bMesh
#define BCHUNK_BMESH_DATA		0x10101 + DATA_BCHUNK	// Data for bMesh.  Child of BCHUNK_BMESH
#define BCHUNK_TEXTURE_NAME		0x10102 + DATA_BCHUNK	// Null terminated string.  Child of BCHUNK_BMESH
#define BCHUNK_MATERIAL_NAME	0x10103 + DATA_BCHUNK	// Null terminated string.  Child of BCHUNK_BMESH
#define BCHUNK_ATTRIBUTES		0x10104 + DATA_BCHUNK	// Null terminated string.  Child of BCHUNK_BMESH

// Flailer chunks
#define BCHUNK_FLAILER_PROJECT	0x10200 + NESTED_BCHUNK	// Holder of chunks in a flailer file.
#define BCHUNK_FLAILER_SCREEN	0x10201 + NESTED_BCHUNK	// Holder of screen chunks.  First chunk is BCHUNK_FLAILER_SCREEN_INFO
#define BCHUNK_FLAILER_END_SCREEN 0x10202 + DATA_BCHUNK	// End marker.  subchunk of FLAILER_PROJECT
#define BCHUNK_FLAILER_SCREEN_INFO 0x10203 + DATA_BCHUNK // class FlailerScreenInfoChunk
#define BCHUNK_FLAILER_BITMAP	0x10210 + DATA_BCHUNK	// class FlailerPlacedBitMapChunk
#define BCHUNK_FLAILER_POINT	0x10211 + DATA_BCHUNK	// class FlailerPlacedPointChunk
#define BCHUNK_FLAILER_TEXT		0x10212 + DATA_BCHUNK	// class FlailerPlacedTextChunk
#define BCHUNK_FLAILER_PROJECT_INFO 0x10213 + DATA_BCHUNK // class flailerprojectInfoChunk

// Max export related chunks
	// The exporter configuration settings.
#define BCHUNK_EXPORTER_CONFIG	0x10F00 + DATA_BCHUNK	// Holds the config options used to export the bbg.

	// The scene
#define BCHUNK_BSCENE			0x11000 + NESTED_BCHUNK	// Holder of a scene
#define BCHUNK_BSCENE_VERSION	0x11001 + DATA_BCHUNK	// Holder of the scene version
#define BCHUNK_BSCENE_DATE		0x11002 + DATA_BCHUNK	// Holder of the save date and time of the scene
#define BCHUNK_BSCENE_DATA		0x11003 + DATA_BCHUNK	// Holder of scene data
#define BCHUNK_BSCENE_VERSION_EXPORTER 0x11004 + DATA_BCHUNK // Holder of the bbg exporter version used

	// The skeleton
#define BCHUNK_BANIM_TREE		0x11100 + NESTED_BCHUNK	// Holder of anim_node chunks
#define BCHUNK_BANIM_TREE_DATA	0x11100 + DATA_BCHUNK	// Holder of tree name and ID
#define BCHUNK_BANIM_TREE_NODE	0x11101 + DATA_BCHUNK	// Holds the TM and indicies into the model vlist, etc.
#define BCHUNK_BANIM_LOCAL_MAP	0x11102 + DATA_BCHUNK	// Holds the local index map

	// The geometry
#define BCHUNK_BGEOM			0x11200 + NESTED_BCHUNK // Holds info relevant to a geometry
#define BCHUNK_BGEOM_VLIST		0x11201 + DATA_BCHUNK	// Holds the vertex list for the model
#define BCHUNK_BGEOM_FLIST		0x11202 + DATA_BCHUNK	// Holds the face list for the model
#define BCHUNK_BGEOM_CLIST		0x11203 + DATA_BCHUNK	// Holds the colour list for the model
#define BCHUNK_BGEOM_NLIST		0x11204 + DATA_BCHUNK	// Holds the normal list for the model
#define BCHUNK_BGEOM_ULIST		0x11205 + DATA_BCHUNK	// Holds the UV list for the model
#define BCHUNK_BGEOM_MATRIX		0x11206 + DATA_BCHUNK	// Holds the object matrix
#define BCHUNK_BGEOM_DATA		0x11207 + DATA_BCHUNK	// Holds the geom name and ID
#define BCHUNK_BGEOM_PROPERTIES	0x11208 + DATA_BCHUNK	// Holds the user properties
#define BCHUNK_BGEOM_INSTANCE	0x11209 + DATA_BCHUNK	// Holds the name of the instance object
#define BCHUNK_BGEOM_EXTENTS	0x11210 + DATA_BCHUNK	// Holds the min max and sphere sizes of the geom
#define BCHUNK_BGEOM_LLIST		0x11211 + DATA_BCHUNK	// Holds the blending vertex list
#define BCHUNK_BGEOM_ELIST		0x11212 + DATA_BCHUNK	// Holds the edge list
#define BCHUNK_BGEOM_PLIST		0x11213 + DATA_BCHUNK	// Holds the patch list for the model
#define BCHUNK_BGEOM_BNLIST		0x11214 + DATA_BCHUNK	// Holds the blended normal list
#define BCHUNK_BGEOM_STRIP		0x11215 + DATA_BCHUNK	// Holds one tri-strip
#define BCHUNK_BGEOM_STRIPLIST	0x11216 + NESTED_BCHUNK	// Holds the tri-strip list
#define BCHUNK_BGEOM_SMOOTHLIST 0x11217 + DATA_BCHUNK	// Holds per-face smoothing group values
#define BCHUNK_BGEOM_AFULIST	0x11218 + NESTED_BCHUNK	// Holds additional frames of UV data
#define BCHUNK_BGEOM_AULIST		0x11219 + DATA_BCHUNK	// Holds additional UV data
#define BCHUNK_BGEOM_PIVOT_MATRIX 0x1121A + DATA_BCHUNK // Holds the pivot matrix.
#define BCHUNK_BGEOM_AFFLIST	0x1121B + NESTED_BCHUNK	// Holds additional frames of face data
#define BCHUNK_BGEOM_AFLIST		0x1121C + DATA_BCHUNK	// Holds additional face data
#define BCHUNK_BGEOM_ALIST		0x1121D + NESTED_BCHUNK	// Holds the data for animated geos
#define BCHUNK_BGEOM_ARLIST		0x1121E + DATA_BCHUNK	// Holds the rotation data for animated geos
#define BCHUNK_BGEOM_APLIST		0x1121F + DATA_BCHUNK	// Holds the position data for animated geos
#define BCHUNK_BGEOM_ASLIST		0x11220 + DATA_BCHUNK	// Holds the scale data for animated geos
#define	BCHUNK_BGEOM_PARENT		0x11221 + DATA_BCHUNK	// Holds the name of this nodes parent node
#define BCHUNK_BGEOM_ICLIST		0x11222 + DATA_BCHUNK	// Holds the illumination colour list for the model
#define BCHUNK_BGEOM_ICELIST	0x11223 + DATA_BCHUNK	// Holds the illumination colour face entry list

#define BCHUNK_BGEOM_CLRCHANNEL	0x11224 + DATA_BCHUNK	// Holds info about a color channel (channel desc, clr table, and face entries)

// Ranges for lists in geometries
#define BCHUNK_BGEOM_RVLIST		0x11401 + DATA_BCHUNK	// Holds a vertex list valid range 
#define BCHUNK_BGEOM_RFLIST		0x11402 + DATA_BCHUNK	// Holds a face list valid range 
#define BCHUNK_BGEOM_RCLIST		0x11403 + DATA_BCHUNK	// Holds a colour list valid range 
#define BCHUNK_BGEOM_RNLIST		0x11404 + DATA_BCHUNK	// Holds a normal list valid range 
#define BCHUNK_BGEOM_RULIST		0x11405 + DATA_BCHUNK	// Holds a UV list valid range 
#define BCHUNK_BGEOM_RLLIST		0x11411 + DATA_BCHUNK	// Holds a blending vertex list valid range
#define BCHUNK_BGEOM_RELIST		0x11412 + DATA_BCHUNK	// Holds an edge list valid range
#define BCHUNK_BGEOM_RPLIST		0x11413 + DATA_BCHUNK	// Holds a patch list valid range 
#define BCHUNK_BGEOM_RBNLIST	0x11414 + DATA_BCHUNK	// Holds a blended normal list valid range
#define BCHUNK_BGEOM_RSTRIPL	0x11216 + DATA_BCHUNK	// Holds a range for a tri-strip list
#define BCHUNK_BGEOM_RSMOOTH	0x11417 + DATA_BCHUNK	// Holds per-face smoothing group ranges
#define BCHUNK_BGEOM_RAFFLIST	0x11418 + DATA_BCHUNK	// Holds additional face range data (Not used yet)
#define BCHUNK_BGEOM_RICLIST	0x11419 + DATA_BCHUNK	// Holds a illum colour list valid range 
#define BCHUNK_BGEOM_RICELIST	0x11420 + DATA_BCHUNK	// Holds the illumination colour face entry valid range 

	// The Animation data
#define BCHUNK_BANIM			0x11300 + NESTED_BCHUNK	// Holds all the animation chunks
#define BCHUNK_BANIM_DATA		0x11300 + DATA_BCHUNK	// Holds all the animation name and ID
#define BCHUNK_BANIM_NODE		0x11301 + NESTED_BCHUNK	// Holds the anim data for a node
#define BCHUNK_BANIM_NODE_DATA	0x11302 + DATA_BCHUNK	// Holds the nodeID and num rot keys
#define BCHUNK_BANIM_ROT_LIST	0x11303 + DATA_BCHUNK	// Holds the rotational animation data (keys)
#define BCHUNK_BANIM_NODE_MAX	0x11304 + DATA_BCHUNK	// TEMPORARY -- max size of above in all of geom
#define BCHUNK_BANIM_POS_LIST	0x11305 + DATA_BCHUNK	// Holds the positional animation data (keys)


// Material related chunks
// materials also use BCHUNK_MATERIAL_NAME and BCHUNK_TEXTURE_NAME

#define BCHUNK_MATLIST			0x12000 + NESTED_BCHUNK	// Holds the scene Materials list
#define BCHUNK_MATERIAL			0x12001 + NESTED_BCHUNK	// Holds a material
#define BCHUNK_MATERIAL_ID		0x12003 + DATA_BCHUNK	// Holds a material id
#define BCHUNK_MATERIAL_COMPIDS	0x12004 + DATA_BCHUNK	// Holds the IDs of other materials in a composite mat
#define BCHUNK_MATERIAL_CUSTOMATTRIBUTE 0x12100 + DATA_BCHUNK	// Holds custom attributes for a material


// for backward compatibility only (prior to version 9 of bAnimDat.hpp)
#define BCHUNK_MATDATA			0x12002 + DATA_BCHUNK	// Holds the matID, etc


#define BCHUNK_TEXTURE			0x12004 + NESTED_BCHUNK	// Holds a texture
#define BCHUNK_TEXTURE_ID		0x12005 + DATA_BCHUNK	// Holds a texture id
#define BCHUNK_TEXTURE_TYPE		0x12006 + DATA_BCHUNK	// Holds a texture type
#define BCHUNK_TEXTURE_AMOUNT	0x12007 + DATA_BCHUNK	// Holds a texture type
#define BCHUNK_TEXTURE_COLOUR	0x12008 + DATA_BCHUNK	// Holds a texture type
#define BCHUNK_TEXTURE_VALUE	0x12009 + DATA_BCHUNK	// Holds a texture type


// Selection Set Chunks
#define BCHUNK_SELECTION_SET	0x13000 + NESTED_BCHUNK	// Holds the selection set data
#define BCHUNK_NAMED_SELECTION	0x13000 + DATA_BCHUNK	// Holds the selection set data
#define BCHUNK_SELECTION_NAME	0x13001 + DATA_BCHUNK	// Holds the selection set data

// Sound chunks
#define BCHUNK_VAG					0x14000		// PSX .vag file.  Header contains spu_address and size.
#define BCHUNK_SAMPLE_TABLE			0x14001		// PSX 'Sample' table.

// Camera Chunks
#define BCHUNK_BCAMERA				0x15000 + NESTED_BCHUNK // Holds the camera data
#define BCHUNK_BCAMERA_DATA			0x15000 + DATA_BCHUNK	// camera data
#define BCHUNK_BCAMERA_POS_LIST		0x15001 + DATA_BCHUNK	// position keys
#define BCHUNK_BCAMERA_ROT_LIST		0x15002 + DATA_BCHUNK	// rotational keys
#define BCHUNK_BCAMERA_FOV_LIST		0x15003 + DATA_BCHUNK	// fov keys
#define BCHUNK_BCAMERA_FOCAL_DISTANCE_LIST 0x15004 + DATA_BCHUNK // focal distance keys


// Light chunks
#define BCHUNK_LIGHT				0x18000 + NESTED_BCHUNK
#define BCHUNK_LIGHT_TYPE			0x18001 + DATA_BCHUNK
#define BCHUNK_LIGHT_ATTENUATION	0x18002 + DATA_BCHUNK
#define BCHUNK_LIGHT_OVERSHOOT		0x18003 + DATA_BCHUNK
#define BCHUNK_LIGHT_SHAPE			0x18004 + DATA_BCHUNK
#define BCHUNK_LIGHT_KEY_COUNT		0x18005 + DATA_BCHUNK
#define BCHUNK_LIGHT_KEY			0x18006 + DATA_BCHUNK
#define BCHUNK_LIGHT_NAMESET		0x18007 + DATA_BCHUNK
#define BCHUNK_LIGHT_RANGE_END		0x18100 + DATA_BCHUNK	// marks the end of the range for the light loader

// Spline chunks
#define BCHUNK_SPLINE				0x19000 + NESTED_BCHUNK
#define BCHUNK_SPLINE_ID			0x19001 + DATA_BCHUNK
#define BCHUNK_SPLINE_LENGTH		0x19002 + DATA_BCHUNK
#define BCHUNK_SPLINE_CLOSED		0x19003 + DATA_BCHUNK
#define BCHUNK_SPLINE_NUM_CVS		0x19004 + DATA_BCHUNK
#define BCHUNK_SPLINE_CV_TABLE		0x19005 + DATA_BCHUNK
#define BCHUNK_SPLINE_PROPERTIES	0x19006 + DATA_BCHUNK
#define BCHUNK_SPLINE_MATRIX		0x19007 + DATA_BCHUNK
#define BCHUNK_SPLINE_RANGE_END		0x19100 + DATA_BCHUNK	// marks the end of the range for the spline loader

// Handy chunks -- Handy is a HelperObject used by hockey.
//OBSOLETE #define BCHUNK_HANDY					0x16000 + NESTED_BCHUNK
//OBSOLETE #define BCHUNK_HANDY_DATA			0x16000 + DATA_BCHUNK
//OBSOLETE #define BCHUNK_HANDY_LEFT_TRACK		0x16001 + DATA_BCHUNK
//OBSOLETE #define BCHUNK_HANDY_RIGHT_TRACK		0x16002 + DATA_BCHUNK

// animtrigger chunks -- animtrigger is also a HelperObject used by hockey.
//OBSOLETE #define BCHUNK_ANIM_TRIGGER			0x16010 + NESTED_BCHUNK
//OBSOLETE #define BCHUNK_ANIM_TRIGGER_HEADER	0x16011 + DATA_BCHUNK
//OBSOLETE #define BCHUNK_ANIM_TRIGGER_DATA		0x16012 + DATA_BCHUNK
//OBSOLETE #define BCHUNK_ANIM_TRIGGER_PARAMS	0x16013 + DATA_BCHUNK
	
// bCar chunks....0x30000 - 0x30fff - Defined in \bCar\Indep\Inc\bCar.hpp
// Hockey chunks  0x20000 - 0x20fff - Defined in \project\as00\dcast\src\inventry\hChunk.hpp

// Caffeine chunks
#define BCHUNK_CAFFEINE_LAYER			0x17000	+ NESTED_BCHUNK
#define BCHUNK_CAFFEINE_LAYER_HEADER	0x17001	+ DATA_BCHUNK
#define BCHUNK_CAFFEINE_OBJECT			0x17002	+ DATA_BCHUNK
#define BCHUNK_CAFFEINE_ATTRIBUTE		0x17003	+ DATA_BCHUNK
#define BCHUNK_CAFFEINE_MESH			0x17004	+ DATA_BCHUNK

//Visual Effects curves from Curvulator
#define BCHUNK_VISUAL_EFFECTS_CURVES	0x17005	+ DATA_BCHUNK

#endif









