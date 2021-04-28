/*

	JExpression.cpp - Node type for tracking expression states

*/

#include "StdAfx.h"
#include "JExpression.h"
#include "JDepFile.h"
#include <CType.h>
//#include <Assert.h>
//#include <stdarg.h>

// http://wwwcsif.cs.ucdavis.edu/~fletcher/classes/110S98/lecture_notes/stack.html

extern OpInfoStruct OpInfo [OP_OpCount];

JExpression::JExpression() : bError(false), bCantParse(false)
{
	Stack.SetOwner(this);
	Output.SetOwner(this);

	for(int i=0; i<JExpr_Prealloc; i++)
		NodeList.AddTail(NodePool + i);
}

JExpression::~JExpression()
{
	while(Stack.PopNode());
	while(Output.PopNode());
	while(NodeList.RemHead());	// remove all nodes from the node list
}


int JExpression::Evaluate(ccNode *pToken, JDepFile *pFile)
{
	InfixToPostfix(pToken, pFile);
	if(bError == true)
		return 0;

	if(bCantParse == true)
		return 1;

	int v1, v2, Result = 0;

	// Now actually evaluate the expression
	while(Output.IsEmpty() == false)
	{
		JExprNode *pNode = Output.PopNode();
		if(pNode->Op == OP_Value)
			Stack.PushNode(pNode);
		else
		{
			switch(pNode->Op)
			{
			// Binary operators pop two operands
			default:
				v2 = Stack.GetValue();
				Stack.Pop();
				// break left out intentionally

			// Unary operators pop only a single operand
			case OP_BitNot:
			case OP_LogicNot:
				v1 = Stack.GetValue();
				Stack.Pop();
				break;
			}

			switch(pNode->Op)
			{
			case OP_BitNot:		Result = ~v1;			break;
			case OP_LogicNot:	Result = !v1;			break;

			case OP_Multiply:	Result = v1 * v2;		break;
			case OP_Divide:
				if(v2 == 0)
				{
					bCantParse = true;
					return 1;
				}
				Result = v1 / v2;
				break;

			case OP_Modulo:
				if(v2 == 0)
				{
					bCantParse = true;
					return 1;
				}
				Result = v1 % v2;
				break;

			case OP_Add:		Result = v1 + v2;		break;
			case OP_Negate:
			case OP_Subtract:	Result = v1 - v2;		break;

			case OP_ShiftLeft:	Result = v1 << v2;		break;
			case OP_ShiftRight:	Result = v1 >> v2;		break;

			case OP_Greater:	Result = v1 > v2;		break;
			case OP_GreaterEqual:Result = v1 >= v2;		break;
			case OP_Less:		Result = v1 < v2;		break;
			case OP_LessEqual:	Result = v1 <= v2;		break;

			case OP_Equals:		Result = v1 == v2;		break;
			case OP_NotEquals:	Result = v1 != v2;		break;

			case OP_BitAnd:		Result = v1 & v2;		break;
			case OP_BitOr:		Result = v1 | v2;		break;
			case OP_BitXOR:		Result = v1 ^ v2;		break;

			case OP_LogicOr:	Result = v1 || v2;		break;
			case OP_LogicAnd:	Result = v1 && v2;		break;
			}

			Stack.Push(Result, OP_Value);
			FreeNode(pNode);
		}
	}
	Result = Stack.GetValue();
	Stack.Pop();
	return Result;
}

static bool IsHex(char c)
{
	return (c >= '0' && c <= '9') ||
		(c >= 'a' && c <= 'f') ||
		(c >= 'A' && c <= 'F');
}

int AtoX(const char *pString)
{
	int Result = 0;
	assert(pString[0] == '0' && toupper(pString[1]) == 'X');
	pString += 2;	// Skip the "0x" prefix
	while( *pString && IsHex(*pString) )
	{
		Result <<= 4;
		if(isdigit(*pString))
			Result += *pString++ - '0';
		else
			Result += toupper(*pString++) - 'A' + 10;
	}
	return Result;
}

int AtoI(const char *pString)
{
	int Result = 0;
	while( *pString && isdigit(*pString) )
	{
		Result *= 10;
		Result += *pString++ - '0';
	}
	return Result;
}


void JExpression::InfixToPostfix(ccNode *pToken, JDepFile *pFile)
{
	bool bLastWasValue = true;		// Used to detect unary negation

	// If the token is an operand, do not stack it. Pass it to the output
	char *pTokName = (char *)pToken->GetName();

	while(pTokName[0])
	{
		JOpType Type = GetOpType(pTokName);
		assert(OpInfo[Type].Opcode == Type);

		if(Type == OP_Value)
		{
			int Value;

			// Attempt to convert the value into a literal number, or find the define'd value
			if(isdigit(pTokName[0]))
			{
				// Check to see if the character is prefixed with 0 (octal), 0x(hex), or contains a decimal
				if(pTokName[0] == '0' && (pTokName[1] == 'x' || pTokName[1] == 'X'))
					Value = AtoX(pTokName);
				else
					Value = AtoI(pTokName);
			}
			else
			{
				// Try to find the defined value of this symbol
				JDefine *pDef = pFile->FindDefine(pTokName);

				if(pDef == 0)
					Value = 0;		// Not found == 0
				else
				{
					if(pDef->bIsValue)
					{
						if(pDef->bEvaluated == false)
						{
							if(HasCircularDependency(pTokName, pDef, pFile))
							{
								Value = 0;
							}
							else
							{
								Value = pFile->EvaluateDefine(pDef);
								if(pFile->bError)
								{
									bError = true;
									return;
								}
							}
						}
						else
						{
							Value = pDef->Value;
						}
					}
					else
						Value = 1;	// Simply defined == 1
				}
			}

			// Push the value onto the output list
			Output.Queue(Value, OP_Value);
			bLastWasValue = true;
		}
		else if(Type == OP_Defined)
		{
			pToken = pToken->GetNext();
			bool bParen = pToken->GetName()[0] == '(';
			if(bParen)
				pToken = pToken->GetNext();

			JDefine *pDef = pFile->FindDefine(pToken->GetName());
			int Result = pDef != 0;
			Output.Queue(Result, OP_Value);

			if(bParen)
			{
				pToken = pToken->GetNext();
				if(pToken->GetName()[0] != ')')
				{
					// printf("Syntax error in \"defined\" keyword\n");
					bError = true;
					return;
				}
			}
			bLastWasValue = true;
		}
		else
		{
			if(Type == OP_Decimal || Type == OP_Pointer)
			{
				bCantParse = true;
				return;
			}

			// Is this a binary subtract or unary negation?
			if(Type == OP_Negate)
			{
				if(bLastWasValue == true)
					Type = OP_Subtract;
				else
					Output.Queue(0, OP_Value);
			}

			// Pop all operators of higher priority and send them to output
			int OpPri = OpInfo[Type].Pri;
			while(	Stack.IsEmpty() == false &&
					OpPri < OpInfo[Stack.GetOpType()].Pri &&
					Stack.GetOpType() != OP_LParen )
			{
				Output.QueueNode(Stack.PopNode());
			}

			if(Type == OP_RParen)
			{
				// If the current operator is RParen, pop until LParen and don't
				// send either of them to the output

				while(Stack.IsEmpty() == false && Stack.GetOpType() != OP_LParen)
					Output.QueueNode(Stack.PopNode());

				assert(Stack.GetOpType() == OP_LParen);
			}
			else
			{
				// Push the current operator
				Stack.Push(0, Type);
			}

			bLastWasValue = false;
		}

		pToken = pToken->GetNext();
		pTokName = (char *)pToken->GetName();
	}

	// Pop any remaining nodes off the stack
	while(Stack.IsEmpty() == false)
	{
		if(Stack.GetOpType() == OP_LParen)
			Stack.Pop();
		else
			Output.QueueNode(Stack.PopNode());
	}
}

void JExpression::OutputWarning(const char *pFmt)
{        
    System::Console::Error->Write(gcnew System::String(pFmt, 0, (int)strlen(pFmt), System::Text::Encoding::ASCII));
}
void JExpression::OutputWarning(const char *pFmt, const char* arg1)
{
    System::Console::Error->Write(gcnew System::String(pFmt, 0, (int)strlen(pFmt), System::Text::Encoding::ASCII), 
                                  gcnew System::String(arg1, 0, (int)strlen(arg1), System::Text::Encoding::ASCII));
}

void JExpression::OutputWarning(const char *pFmt, const char* arg1, const char* arg2)
{
    System::Console::Error->Write(gcnew System::String(pFmt, 0, (int)strlen(pFmt), System::Text::Encoding::ASCII), 
                                  gcnew System::String(arg1, 0, (int)strlen(arg1), System::Text::Encoding::ASCII), 
                                  gcnew System::String(arg2, 0, (int)strlen(arg2), System::Text::Encoding::ASCII));
}


bool  JExpression::HasCircularDependency(const char *pTokName, JDefine *pDef, JDepFile *pFile)
{
	bool foundDependency = false;
	// Try to find immediate circular dependency. Bail out if found.
	if(!(pTokName && *pTokName && pDef))
	{
		return foundDependency;
	}
	ccNode *pChildToken = pDef->Tokens.GetHead();
	ccNode *pChildTailToken = pDef->Tokens.GetTail();

	while( !foundDependency && (pChildToken != pChildTailToken) )
	{
		char *pChildTokName = (char *)pChildToken->GetName();
		if(pChildTokName&& *pChildTokName)
		{
			if(0 == strcmp(pTokName , pChildTokName))
			{				
				const char* fname = "unknown";
				if(pFile)
					fname = pFile->GetName();
                OutputWarning("WARNING. Found circular dependency in preprocessor definitions.\n***** file: '{0}'\n***** token: {1}\n", fname, pTokName);
				foundDependency = true;
			}
			else
			{
				// See if points back to parent:
				JDefine *pChildDef = pFile->FindDefine(pChildTokName);
				foundDependency = HasCircularDependency(pTokName, pChildDef, pFile);
			}
		}
		if(foundDependency)
		{
			break;
		}

		pChildToken = pChildToken->GetNext();
	}
	return foundDependency;
}


JOpType JExpression::GetOpType(const char *pString)
{
	if(isdigit(pString[0]))
		return OP_Value;

	for(int i=0; i<OP_OpCount; i++)
	{
		if(strcmp(pString, OpInfo[i].pOpString) == 0)
			return (JOpType)i;
	}

	// Unrecognized - assume it's a value
	return OP_Value;
}


void JExpressionStack::Push(int NewValue, JOpType NewOp)
{
	JExprNode *pExpr = pOwner->AllocNode();
    if(pExpr == 0)
    {
        printf("Internal error. Fixed size node pool [size=%d]is too small to parse expression\n",JExpr_Prealloc);
        assert(pExpr);
    }

	pExpr->Value = NewValue;
	pExpr->Op = NewOp;

	Stack.AddHead(pExpr);
	Value = NewValue;
	OpType = NewOp;
}

void JExpressionStack::Queue(int NewValue, JOpType NewOp)
{
	JExprNode *pExpr = pOwner->AllocNode();
	assert(pExpr);

	pExpr->Value = NewValue;
	pExpr->Op = NewOp;

	Stack.AddTail(pExpr);
}

void JExpressionStack::Pop(void)
{
	JExprNode *pExpr = (JExprNode *)Stack.RemHead();
	JExprNode *pNode = (JExprNode *)Stack.GetHead();
	if(pNode)
	{
		Value = pNode->Value;
		OpType = pNode->Op;
	}
	else
	{
		Value = 0;
		OpType = OP_Value;
	}

	if(pExpr)
		pOwner->FreeNode(pExpr);
}

JExprNode *JExpressionStack::PopNode(void)
{
	JExprNode *pExpr = (JExprNode *)Stack.RemHead();
	JExprNode *pNode = (JExprNode *)Stack.GetHead();
	if(pNode)
	{
		Value = pNode->Value;
		OpType = pNode->Op;
	}
	else
	{
		Value = 0;
		OpType = OP_Value;
	}
	return pExpr;
}

void JExpressionStack::PushNode(JExprNode *pNode)
{
	Stack.AddHead(pNode);
	Value = pNode->Value;
	OpType = pNode->Op;
}

void JExpressionStack::QueueNode(JExprNode *pNode)
{
	Stack.AddTail(pNode);
}


OpInfoStruct OpInfo[OP_OpCount] = 
{
	{	-1,		OP_Value,		""	},
	{	-1,		OP_Defined,		"defined"},

	// Precedence
	{	100,	OP_LParen,		"("	},
	{	100,	OP_RParen,		")"	},

	// Unary
	{	95,		OP_Negate,		"-"	},
	{	95,		OP_BitNot,		"~"	},
	{	95,		OP_LogicNot,	"!"	},

	// Multiplicative
	{	90,		OP_Multiply,	"*"	},
	{	90,		OP_Divide,		"/"	},
	{	90,		OP_Modulo,		"%"	},

	// Additive
	{	85,		OP_Add,			"+"	},
	{	85,		OP_Subtract,	"-"	},

	// Shift
	{	80,		OP_ShiftLeft,	"<<"},
	{	80,		OP_ShiftRight,	">>"},

	// Relational
	{	75,		OP_Greater,		">"	},
	{	75,		OP_GreaterEqual,">="},
	{	75,		OP_Less,		"<"	},
	{	75,		OP_LessEqual,	"<="},

	// Equality
	{	70,		OP_Equals,		"=="},
	{	70,		OP_NotEquals,	"!="},

	// Binary Bitwise
	{	55,		OP_BitAnd,		"&"	},
	{	50,		OP_BitXOR,		"^"	},
	{	45,		OP_BitOr,		"|"	},

	// Binary Logical
	{	40,		OP_LogicAnd,	"&&"},
	{	35,		OP_LogicOr,		"||"},

	// Unhandled - Part of a class
	{	0,		OP_Decimal,		"."	},
	{	0,		OP_Pointer,		"->"},
};
