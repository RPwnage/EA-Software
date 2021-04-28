#pragma once

#include <string.h>
#include <stdio.h>
#include <list>

namespace sonar
{

  template <typename _Ty, size_t _LENGTH> class Queue
  {
    size_t m_iReadCursor;
    size_t m_iWriteCursor;
    size_t m_cItems;

    _Ty m_queueList[_LENGTH];
    size_t m_endIndex;
    size_t m_startIndex;
    size_t m_cQueueLength;

  public:

		bool empty()
		{
			return m_cItems == 0;
		}

		bool isFull()
		{
			return m_cItems == m_cQueueLength;
		}

		void clear()
		{
			m_cItems = 0;
			m_iReadCursor = 0;
			m_iWriteCursor = 0;
		}

    bool pushBegin (const _Ty &_inItem)
    {
      if (m_cItems == m_cQueueLength)
      {
        return false;
      }

      m_iReadCursor --;
      m_queueList[m_iReadCursor % m_cQueueLength] = _inItem;
      m_cItems ++;
      return true;
    }

    bool pushEnd (const _Ty &_inItem)
    {
      if (m_cItems == m_cQueueLength)
      {
        return false;
      }

      m_queueList[m_iWriteCursor % m_cQueueLength] = _inItem;
      m_iWriteCursor ++;
      m_cItems ++;
      return true;
    }

    _Ty begin (void)
    {
      return m_queueList[m_iReadCursor % m_cQueueLength];
    }

    _Ty end (void)
    {
      return m_queueList[ (m_iWriteCursor - 1) % m_cQueueLength];
    }

    _Ty popBegin (void)
    {
      _Ty ri;
      ri = m_queueList[m_iReadCursor % m_cQueueLength];
      m_iReadCursor ++;
      m_cItems --;

      return ri;
    }


    _Ty popEnd (void)
    {
      _Ty ri;
      m_iWriteCursor --;
      ri = m_queueList[m_iWriteCursor % m_cQueueLength];
      m_cItems --;
      return ri;
    }

		_Ty getItem (size_t index)
		{
			return m_queueList[(m_iReadCursor + index) % m_cQueueLength];
		}

    size_t size (void) const
    {
      return (int) m_cItems;
    }

    Queue ()
    {
      m_cQueueLength = _LENGTH;
      m_iReadCursor = 0;
      m_iWriteCursor = 0;
      m_cItems = 0;

    }

  };

}