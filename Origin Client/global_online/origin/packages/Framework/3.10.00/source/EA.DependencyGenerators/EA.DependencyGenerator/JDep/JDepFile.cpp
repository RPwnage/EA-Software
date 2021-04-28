/*

	JDepFile.cpp - Class to hold a parsed representation of a header file

*/

#include "StdAfx.h"
#include "JDepFile.h"
#include "JExpression.h"

static int DefinesEvaluated = 0;


int JDepFile::EvaluateDefine(JDefine *pDefine)
{
JExpression Expr;

	ccNode *pNode = pDefine->Tokens.GetHead();
	int Result = Expr.Evaluate(pNode, this);
	if(Expr.Error())
		bError = true;
	else
	{
		pDefine->bEvaluated = true;
		pDefine->Value = Result;
	}
	pDefine->Tokens.Purge();
	DefinesEvaluated++;

	return Result;
}
