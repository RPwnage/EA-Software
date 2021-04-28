/*

	JCondition.h - Node type for tracking condition states

*/

#ifndef JCONDITION_H_
#define JCONDITION_H_

#include "ccList.h"

enum ConditionState
{
	Cond_False,
	Cond_True,
	Cond_Finished,
};


class ConditionStack
{
public:
	ConditionStack() : StackIndex(0) {Stack[0] = Cond_True;}	// Start with "True" state to simplify "Push" code

	void Push(int NewState);
	void Pop(void)
	{
		if ( StackIndex > 0 )
		{
			StackIndex--;
		}
	}

	void SetConditionTrue(void) {Stack[StackIndex] = Cond_True;}
	void SetConditionFinished(void) {Stack[StackIndex] = Cond_Finished;}

	int GetState(void) const {return Stack[StackIndex];}
	bool InCondition(void) const {return StackIndex > 0;}

private:
	char		Stack[1024];
	int			StackIndex;
};

#endif
