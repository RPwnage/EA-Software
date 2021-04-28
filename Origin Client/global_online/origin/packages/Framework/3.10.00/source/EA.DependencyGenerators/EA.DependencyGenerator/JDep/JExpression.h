/*

	JExpression.h - Node type for tracking expression states

*/

#ifndef JEXPRESSION_H_
#define JEXPRESSION_H_

#include "ccList.h"
class JDepFile;
class JExpression;
class JDefine;

int AtoI(const char *pString);
int AtoX(const char *pString);


// Types of operations supported by the preprocessor
typedef enum
{
	OP_Value = 0,
	OP_Defined,

	OP_LParen,
	OP_RParen,

	OP_Negate,
	OP_BitNot,
	OP_LogicNot,

	OP_Multiply,
	OP_Divide,
	OP_Modulo,

	OP_Add,
	OP_Subtract,

	OP_ShiftLeft,
	OP_ShiftRight,

	OP_Greater,
	OP_GreaterEqual,
	OP_Less,
	OP_LessEqual,

	OP_Equals,
	OP_NotEquals,

	OP_BitAnd,
	OP_BitXOR,
	OP_BitOr,

	OP_LogicAnd,
	OP_LogicOr,

	OP_Decimal,
	OP_Pointer,

	OP_OpCount
} JOpType;


typedef struct
{
	int		Pri;
	JOpType	Opcode;
	char	*pOpString;
} OpInfoStruct;


// Used for conversion from infix to postfix
class JExprNode : public ccMinNode
{
public:
	int		Value;	// Only used if JOpType == OP_Value
	JOpType Op;
};


class JExpressionStack
{
public:
	JExpressionStack() : Value(0), OpType(OP_Value) {;}

	void SetOwner(JExpression *pObj) {pOwner = pObj;}

	void Push(int NewValue, JOpType NewOp);		// Push to head
	void Queue(int NewValue, JOpType NewOp);	// Push to tail

	void Pop(void);
	JExprNode *PopNode(void);

	void PushNode(JExprNode *pNode);			// Push to head
	void QueueNode(JExprNode *pNode);			// Push to tail

	int		GetValue(void) const {return Value;}
	JOpType GetOpType(void) const {return OpType;}

	bool IsEmpty(void) const {return Stack.GetNumElements() == 0;}
	int Count(void) const {return (int)Stack.GetNumElements();}

private:
	ccMinList	Stack;
	JExpression	*pOwner;

	int 		Value;
	JOpType		OpType;
};


const int JExpr_Prealloc = 256*3;

class JExpression
{
public:
	JExpression();
	~JExpression();

	int Evaluate(ccNode *pToken, JDepFile *pFile);	// pToken is the first token in a list, terminated by an empty string
	bool Error(void) const {return bError;}

	JExprNode *AllocNode(void) {return (JExprNode *)NodeList.RemHead();}
	void FreeNode(JExprNode *pNode) {return NodeList.AddHead(pNode);}

private:

	void OutputWarning(const char *pFmt);							// display a warning
	void OutputWarning(const char *pFmt, const char*);							// display a warning
	void OutputWarning(const char *pFmt, const char*, const char*);							// display a warning

	JExpressionStack	Stack, Output;
	bool bError, bCantParse;

	JExprNode	NodePool[JExpr_Prealloc];
	ccMinList	NodeList;

	void InfixToPostfix(ccNode *pToken, JDepFile *pFile);	// Convert the expression into a more easily handled format
	JOpType GetOpType(const char *pString);
	bool  HasCircularDependency(const char *pTokName, JDefine *pDef, JDepFile *pFile);

};


#endif
