#pragma once
#include <stdio.h>

namespace sonar
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
		char *getPushPtr();
		char *getEndPtr();
		char *getPopPtr(void);

	private:

		char *m_pBuffer;
		size_t m_cbMaxLength;
		size_t m_iOffset;
	};
}
