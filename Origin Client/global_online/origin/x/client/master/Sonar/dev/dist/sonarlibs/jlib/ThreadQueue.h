#pragma once

#include <assert.h>

#include "jlib/Queue.h"
#include "jlib/Condition.h"

namespace jlib
{

template <typename _Ty, size_t MAXITEMS>
class ThreadQueue
{
private:
	Condition m_cond;
	bool m_bStopped;


	Queue<_Ty, MAXITEMS> m_queue;

public:
	ThreadQueue ()
	{
		m_bStopped = false;

	}

	void LockQueue (void)
	{
		m_cond.WaitAndLock ();
	}

	void UnlockQueue (void)
	{
		m_cond.Unlock ();
	}

	void StopQueue (void)
	{
		m_cond.WaitAndLock ();
		m_bStopped = true;
	}

	void StartQueue (void)
	{
		m_cond.Signal ();
		m_cond.Unlock ();
		m_bStopped = false;
	}

	/*
	LOCK MUST BE AQUIRED BEFORE THIS! */
	Queue<_Ty, MAXITEMS> &GetQueue (void)
	{
		return m_queue;
	}

	_Ty WaitForItem (void)
	{
    _Ty ri;

		RETRY_WAIT:

		m_cond.WaitAndLock ();

    if (m_queue.size() > 0)
    {
  		ri = m_queue.popEnd ();
      m_cond.Unlock ();

      return ri;
    }

    m_cond.WaitForCondition ();

    if (m_queue.size() > 0)
    {
      ri = m_queue.popEnd ();
 			m_cond.Unlock ();

      return ri;
    }

		m_cond.Unlock ();
  	goto RETRY_WAIT;
	}

	bool PollForItem (_Ty &outItem)
	{
		bool bResult;

		m_cond.WaitAndLock ();

    if (m_queue.getItemCount() < 1)
    {
  		m_cond.Unlock ();
      return false;
    }

		outItem = m_queue.popEnd ();
		m_cond.Unlock ();

    return true;
	}

	void PostItem (const _Ty &inItem)
	{
		bool bResult; 

		m_cond.WaitAndLock ();
		bResult = m_queue.pushBegin (inItem);

		if (!m_bStopped)
		{
			m_cond.Signal ();
		}
		m_cond.Unlock ();

		assert (bResult);
	}

	int GetItemCount (void)
	{
    int ic;

    m_cond.WaitAndLock ();
		ic = m_queue.GetItemCount ();
    m_cond.Unlock();

    return ic;
	}

	int GetMaxSize (void)
	{
		return m_queue.GetMaxSize ();
	}
};

}