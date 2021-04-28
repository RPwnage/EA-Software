//
// profan.h
//

/*
 Simple profanity filter to scan incoming text for prohibited works and replace them
 with strings of symbols. In order to keep performance from lagging, a deterministic
 finite automata search engine is used with a precalculated state table. The state
 table only needs to be build when the prohibited work list changes. The actual scanning
 with the prebuild table is very fast O(n).
 */

#ifndef __profan_h
#define __profan_h

typedef struct ProfanRef ProfanRef;

// create new profanity filter
ProfanRef *ProfanCreate();

// destroy the profanity filter
void ProfanDestroy(ProfanRef *ref);

// clear the profanity table
void ProfanClear(ProfanRef *ref);

// let user set their own substiution table
void ProfanSubst(ProfanRef *ref, const char *sublst);

// import a profanity filter
int32_t ProfanImport(ProfanRef *ref, const char *list);

// convert a line of text
int32_t ProfanConv(ProfanRef *ref, char *line, bool justCheckIsProfane = false);

// chop leading/trailing/extra spaces from line
int32_t ProfanChop(ProfanRef *ref, char *line);

// check the filter file to see what rules are overshadowed
void ProfanCheck(ProfanRef *ref);

// Return the rule that was last used on a ProfanConv() call
const char *ProfanLastMatchedRule(ProfanRef *pRef);

// Write the state table to the given file descriptor
int32_t ProfanSave(ProfanRef *pRef, int32_t iFd);

// Read the state table from the given file descriptor
int32_t ProfanLoad(ProfanRef *pRef, int32_t iFd);

#endif // __profan_h

