//
// Copyright 2008, Electronic Arts Inc
//
#ifndef MovingAverage_H
#define MovingAverage_H

#include <queue>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

template <typename TType> class MovingAverage
{
public:
	MovingAverage(int SizeLimit = 1);

	~MovingAverage();

	// Allow changing the history at any time. Of course, doing so while collecting data
	// will invalidate the moving average data until the collector is full again.
	void SetHistorySize(int SizeLimit);

	// Add data to the history
	void Add(TType Data);
	
	inline TType GetTotal() { return mTotal; }

	// Get the average of all the data
	TType GetAverage();

private:
	TType				mTotal;
	int					mSizeLimit;
	std::queue <TType>	mHistory;
};

template <typename TType>
MovingAverage<TType>::MovingAverage(int SizeLimit)
:	mTotal(TType(0)),
mSizeLimit(SizeLimit)
{
	Q_ASSERT(SizeLimit > 0);
}


template <typename TType>
MovingAverage<TType>::~MovingAverage()
{
	while (!mHistory.empty())	mHistory.pop();
	mTotal = TType(0);
}


template <typename TType>
void MovingAverage<TType>::SetHistorySize(int SizeLimit)
{

	Q_ASSERT(SizeLimit >= 0);

	mSizeLimit = SizeLimit;
	
	// Is the history to be emptied?
	if (mSizeLimit == 0)
	{
		mTotal = TType(0);

		while (!mHistory.empty())
			mHistory.pop();
	}
	else
	{
		// Determine the number of data points to throw away (if any)
		int	throw_away = int(mHistory.size()) - SizeLimit;
		
		// delete any data points that needs to be dumped
		while (throw_away-- > 0)
		{	
			mTotal -= mHistory.front();		
			mHistory.pop();
		}			
	}

} // HistorySize


template <typename TType>
void MovingAverage<TType>::Add(TType Data)
{
	if (int(mHistory.size()) >= mSizeLimit)
	{
		// remove the history point
		mTotal -= mHistory.front();
		mHistory.pop();
	}		

	// Add data to the history
	mTotal += Data;
	mHistory.push(Data);
} // Add


template <typename TType>
TType MovingAverage<TType>::GetAverage()
{
	return mHistory.size() > 0? TType(mTotal/mHistory.size()) : TType (0);
}

}

}

#endif
