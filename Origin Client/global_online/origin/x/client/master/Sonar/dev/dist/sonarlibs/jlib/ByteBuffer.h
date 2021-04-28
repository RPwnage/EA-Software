#pragma once

#include <jlib/types.h>

namespace jlib
{

	class ByteBuffer
	{
	public:
		ByteBuffer (size_t _cbMaxSize);
		~ByteBuffer (void);
		void flush (void);
		size_t popUntilChar (void *_pOutData, size_t _cbMaxLength, int _ch);
		size_t pop (void *_pOutData, size_t _cbMaxLength);
		size_t pop (size_t cbMaxLength);
		size_t push (const void *_pData, size_t _cbLength);
		size_t push (size_t cbLength);
		UINT8 *getPushPtr();
		UINT8 *getEndPtr();
		UINT8 *getPopPtr(void);

	private:

		UINT8 *m_pBuffer;
		size_t m_cbMaxLength;
		size_t m_iOffset;
	};
}