/*

	JDepFile.h - Class to hold a parsed representation of a header file

*/

#ifndef __JDEPFILE_H__
#define __JDEPFILE_H__

#include "ccList.h"
#include "JDefine.h"
#include "JPrefilteredFile.h"
#include "JCondition.h"


class JDepFile : public ccNode
{
public:
	JDepFile() : pParent(0), bSysInclude(false), bError(false), Defines(4091), MasterIncludes(37) {pTopParent = this;}
	JDepFile(JDepFile *pPar) : pParent(pPar), bSysInclude(false), bError(false), Defines(1), MasterIncludes(1) {pTopParent = pPar->pTopParent;}
	~JDepFile() {;}

	//JDefine		*GetFirstDefine(void) {return (JDefine *)Defines.GetHead();}
	JDepFile	*GetFirstInclude(void) {return (JDepFile *)Includes.GetHead();}

	JDefine		*FindDefine(const char *pSymbol) {return (JDefine *)pTopParent->Defines.Find(pSymbol);}
	int			EvaluateDefine(JDefine *pDefine);

	JDepFile	*GetNext(void) {return (JDepFile *)ccMinNode::GetNext();}


	JDepFile	*pParent, *pTopParent;
	bool		bSysInclude, bError;

	JPrefiltState		Filt;
	ConditionStack		Cond;

	ccList		Includes;
	JDefineList	Defines;
	ccHashList	MasterIncludes;

	friend class JDep;
};

#endif
