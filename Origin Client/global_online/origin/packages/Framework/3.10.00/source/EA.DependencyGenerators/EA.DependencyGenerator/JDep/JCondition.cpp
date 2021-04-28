/*

	JCondition.cpp - Node type for tracking condition states

*/

#include "StdAfx.h"
#include "JCondition.h"

void ConditionStack::Push(int NewState)
{
	if(Stack[StackIndex] != Cond_True)
		NewState = Cond_Finished;

	Stack[++StackIndex] = NewState;
}
