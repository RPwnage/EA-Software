/*! ************************************************************************************************/
/*!
\file fittable.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "fittable.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    const char8_t* FitTable::GENERICRULE_FIT_TABLE_KEY = "fitTable";
    const char8_t* FitTable::GENERICRULE_SPARSE_FIT_TABLE_KEY = "sparseFitTable";

    const char8_t* FitTable::SPARSE_FIT_TABLE_DIAGONAL_VALUE = "diagonalValue";
    const char8_t* FitTable::SPARSE_FIT_TABLE_OFFDIAGONAL_VALUE = "offDiagonalValue";
    const char8_t* FitTable::SPARSE_FIT_TABLE_RANDOM_VALUE = "randomValue";
    const char8_t* FitTable::SPARSE_FIT_TABLE_SPARSE_VALUES = "sparseValues";

    const char8_t* FitTable::SPARSE_FIT_TABLE_FILL_COLUMN = "sparseFitTableFillColumn";
    const char8_t* FitTable::SPARSE_FIT_TABLE_FILL_ROW = "sparseFitTableFillRow";

    const char8_t* FitTable::ABSTAIN_LITERAL_CONFIG_VALUE = "abstain";
    const char8_t* FitTable::RANDOM_LITERAL_CONFIG_VALUE = "random";

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    FitTable::FitTable() : mFitTable(nullptr), mMaxFitPercent(0)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    FitTable::~FitTable()
    {
        // free fit table
        if (mFitTable != nullptr)
        {
            delete[] mFitTable;
            mFitTable = nullptr;
        }
    }

    // Config map is for the rule, this method determines if it is sparse or not.
    bool FitTable::initialize(const FitTableList& fitTableList, const SparseFitTable& sparseFitTable, const PossibleValuesList& possibleValues)
    {
        FitTableContainer fitTable;

        bool hasFitTable = !(fitTableList.empty());

        // if we have a normal fit table, parse that...
        if(hasFitTable)
        {
            if (!parseExplicitFitTable(fitTableList, fitTable))
            {
                return false;
            }
        }
        else
        {
            // if we have a sparse fit table, parse that...
            if (!parseSparseFitTable(sparseFitTable, possibleValues, fitTable))
            {
                return false;
            }
        }

        // TODO: setup the fit table.
        return initFitTable(fitTable, possibleValues.size());
    }

    /*! ************************************************************************************************/
    /*!
    \brief Helper: build a vector<float> from a fitTableSequence.  Returns false if fitTableSequence
    is nullptr or empty.

    \param[in]  fitTableList - values for the fit table
    \param[in,out]  fitTable - the destination vector to fill in.
    \return true on success, false if the possibleValues sequence is missing/invalid
    ****************************************************************************************************/
    bool FitTable::parseExplicitFitTable(const FitTableList& fitTableList, FitTableContainer &fitTable) const
    {
        fitTable.reserve(fitTableList.size());
        FitTableList::const_iterator iter = fitTableList.begin();
        for(; iter != fitTableList.end(); ++iter)
        {
            float fit = *iter;
            if ( (fit < 0) || (fit > 1) )
            {
                ERR_LOG("[FitTable].parseExplicitFitTable 'fitTable' contains invalid value (must be between 0 and 1)");
                return false;
            }
            fitTable.push_back(fit);
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*!
    \brief Helper: build a vector<float> from sparseFitTable values.  Returns false if the values
    are invalid.

    \param[in]  sparseFitTable -  value tags for the sparse fit table
    \param[in]  possibleValues -  possible values for the rule
    \param[in,out]  fitTable - the destination vector to fill in.
    \return true on success, false if the possibleValues sequence is missing/invalid
    ****************************************************************************************************/
    bool FitTable::parseSparseFitTable(const SparseFitTable& sparseFitTable, const PossibleValuesList& possibleValues, FitTableContainer &fitTable)
    {

        // get the diagonalValue, offDiagonalValue, and randomValue
        float diagonalValue = sparseFitTable.getDiagonalValue();
        TRACE_LOG("[FitTable].parseSparseFitTable - diagonalValue = " << diagonalValue);
        float offDiagonalValue = sparseFitTable.getOffDiagonalValue();
        TRACE_LOG("[FitTable].parseSparseFitTable - offDiagonalValue = " << offDiagonalValue);
        float randomValue = sparseFitTable.getRandomValue();
        TRACE_LOG("[FitTable].parseSparseFitTable - randomValue = " << randomValue);

        // verify that the values are valid
        const char8_t* errItem = nullptr;
        if ( (diagonalValue < 0.0) || (diagonalValue > 1.0) )
            errItem = SPARSE_FIT_TABLE_DIAGONAL_VALUE;
        else if ( (offDiagonalValue < 0.0) || (offDiagonalValue > 1.0) )
            errItem = SPARSE_FIT_TABLE_OFFDIAGONAL_VALUE;
        else if ( (randomValue < 0.0) || (randomValue > 1.0) )
            errItem = SPARSE_FIT_TABLE_RANDOM_VALUE;

        if ( errItem != nullptr )
        {
            ERR_LOG("[FitTable].parseSparseFitTable '" << errItem << "' contains invalid value (must be between 0 and 1)");
            return false;
        }

        // figure the size of the matrix, the width is the same as the height
        size_t fitTableWidth = possibleValues.size();
        size_t fitTableSize = fitTableWidth * fitTableWidth;
        fitTable.reserve(fitTableSize);

        // if x == y, put in the diagonalValue, otherwise put in the offDiagonalValue
        for(size_t yi=0; yi<fitTableWidth; ++yi)
            for(size_t xi=0; xi<fitTableWidth; ++xi)
                fitTable.push_back(xi == yi ? diagonalValue : offDiagonalValue);

        // deal with the special columns
        handleRandomAbstainForSparseFitTable(randomValue, possibleValues, fitTable);

        return parseSparseValues(sparseFitTable.getSparseValues(), possibleValues, fitTable);
    }

    /*! ************************************************************************************************/
    /*! \brief Helper: if this rule uses 'abstain' or 'random', init the fitTable's abstain or random rows and columns.

    \param[in]  randomValue - the value to put in the random row and column.
    \param[in]  possibleValueSequence - sequence of possible values for the rule
    \param[in,out]  fitTable - the destination vector to fill in.
    \return true on success, false if the possibleValues sequence is missing/invalid
    ****************************************************************************************************/
    void FitTable::handleRandomAbstainForSparseFitTable(float randomValue, const PossibleValuesList& possibleValues,
        FitTableContainer &fitTable)
    {
        // get the width of the matrix
        int32_t matrixWidth = int32_t(possibleValues.size());

        // look for abstain and random in the possible value sequence
        const int32_t INVALID_INDEX = -1;
        int32_t randomIndex = INVALID_INDEX;
        int32_t abstainIndex = INVALID_INDEX;

        PossibleValuesList::const_iterator iter = possibleValues.begin();
        for (int32_t index=0; iter != possibleValues.end(); ++iter)
        {
            const char8_t *currentPossibleValue = *iter;
            if (blaze_stricmp(currentPossibleValue, ABSTAIN_LITERAL_CONFIG_VALUE) == 0)
            {
                abstainIndex = index;
            }
            else if (blaze_stricmp(currentPossibleValue, RANDOM_LITERAL_CONFIG_VALUE) == 0)
            {
                randomIndex = index;
            }

            // early out if we've already found both
            if ( (abstainIndex != INVALID_INDEX) && (randomIndex != INVALID_INDEX) )
            {
                break;
            }
            ++index;
        }

        // if random exists, set row and column values to the specified 'randomValue'
        if (randomIndex != INVALID_INDEX)
        {
            setCol(randomIndex, matrixWidth, randomValue, fitTable);
            setRow(randomIndex, matrixWidth, randomValue, fitTable);
        }

        // if abstain exists, set row and column values to 1.0
        if (abstainIndex != INVALID_INDEX)
        {
            setCol(abstainIndex, matrixWidth, 1.0, fitTable);
            setRow(abstainIndex, matrixWidth, 1.0, fitTable);
        }
    }

    /*! ************************************************************************************************/
    /*!
        \brief Helper: initialize & validate the rule's fitTable. Returns false on error.

            Note: the fit table depends on the possibleValues, so we must init the possibleValues first.

        \param[in]  fitTable - a representation of the fitTable (as a row-major array).
        \return true on success, false otherwise
    *************************************************************************************************/
    bool FitTable::initFitTable(const FitTableContainer &fitTable, size_t numPossibleValues)
    {
        if (numPossibleValues == 0)
        {
            ERR_LOG("[FitTable].initFitTable: numPossible values passed in with 0.");
            return false;
        }

        size_t expectedFitTableSize = numPossibleValues * numPossibleValues;

        if (fitTable.size() != expectedFitTableSize)
        {
            ERR_LOG("[FitTable].initFitTable ERROR fitTable has incorrect length; expected " << expectedFitTableSize << " elements. (ie: numPossibleValues^2).");
            return false;
        }

        // alloc fit Table
        mFitTable = BLAZE_NEW_ARRAY(float, expectedFitTableSize);
        mMaxFitPercent = 0;

        for(size_t i=0; i<expectedFitTableSize; ++i)
        {
            mFitTable[i] = fitTable[i];
            if (mMaxFitPercent < fitTable[i])
            {
                mMaxFitPercent = fitTable[i];
            }
        }

        mNumberPossibleValues = numPossibleValues;
        mFitTableSize = expectedFitTableSize;

        return true;
    }
    

    /*! ************************************************************************************************/
    /*!
        \brief Helper: add sparse values to the fitTable.

        \param[in]  sparseFitValues - sequence containing sparse value sequences in [x,y,value] format
        \param[in]  possibleValues - sequence of possible values for the rule
        \param[in,out]  fitTable - the destination vector to fill in.
        \return true on success, false if the possibleValues sequence is missing/invalid
    ****************************************************************************************************/
    bool FitTable::parseSparseValues(const SparseFitValueList& sparseFitValues, const PossibleValuesList& possibleValues, FitTableContainer &fitTable)
    {
        SparseFitValueList::const_iterator iter = sparseFitValues.begin();
        for (; iter != sparseFitValues.end(); ++iter)
        {
            const FitThresholdList *sparseValueList = *iter;
            uint16_t valueCount = (uint16_t)possibleValues.size();

            if(sparseValueList->size() != 3)
            {
                ERR_LOG("[FitTable].parseSparseValues '" << (FitTable::SPARSE_FIT_TABLE_SPARSE_VALUES) << "' contains invalid entry");
                return false;
            }

            int16_t xc = valueCount;
            int16_t yc = valueCount;

            const char8_t* n1 = sparseValueList->at(0);
            const char8_t* n2 = sparseValueList->at(1);
            const char8_t* n3 = sparseValueList->at(2);

            float val;
            blaze_str2flt(n3, val);

            if(blaze_stricmp(n1, FitTable::SPARSE_FIT_TABLE_FILL_COLUMN) == 0)
            {
                xc = findIndex(n2, possibleValues);
            }
            else if (blaze_stricmp(n1, FitTable::SPARSE_FIT_TABLE_FILL_ROW) == 0)
            {
                yc = findIndex(n2, possibleValues);
            }
            else
            {
                xc = findIndex(n1, possibleValues);
                yc = findIndex(n2, possibleValues);
            }

            if ((val < 0.0 || val > 1.0) || (xc >= valueCount && yc >= valueCount) || (xc < 0 || yc < 0))
            {
                ERR_LOG("[FitTable].parseSparseValues '" << (FitTable::SPARSE_FIT_TABLE_SPARSE_VALUES) << "' contains invalid values");
                return false;
            }

            if(xc >= valueCount)
            {
                setRow(yc, valueCount, val, fitTable);
            }
            else if (yc >= valueCount)
            {
                setCol(xc, valueCount, val, fitTable);
            }
            else
            {
                fitTable[xc + yc * valueCount] = val;
            }
        }

        return true;
    }

    /*! ***********************************************************************************/
        /*!
        \brief Local Helper: finds the string index in the sequence or returns -1 if not found

        \param[in] findStr - string to match against
        \param[in] possibleValues - sequence of strings to match against
        \return index where findStr was found in the sequence or -1 if not found
    ***************************************************************************************/
    int16_t FitTable::findIndex( const char8_t* findStr, const PossibleValuesList& possibleValues ) const
    {
        uint16_t index = 0;
        uint16_t numVals = (uint16_t)possibleValues.size();

        while( index < numVals )
        {
            if (blaze_stricmp(findStr, possibleValues.at(index)) == 0)
                return index;

            ++index;
        }

        // if we didn't find the string in the list, see if it's a number
        blaze_str2int(findStr, &index);
        if( index == 0 && (*findStr < '0' || *findStr > '9') )
            return -1;

        return index;
    }

    float FitTable::getFitPercent(size_t myValueIndex, size_t otherEntityValueIndex) const
    {
        size_t index = myValueIndex * mNumberPossibleValues + otherEntityValueIndex;

        if (myValueIndex > mNumberPossibleValues)
        {
            ERR_LOG("[FitTable].getFitPercent ERROR my index outside of acceptable range '" << myValueIndex << "'");
        }

        if (otherEntityValueIndex > mNumberPossibleValues)
        {
            ERR_LOG("[FitTable].getFitPercent ERROR other index outside of acceptable range '" << otherEntityValueIndex << "'");
        }

        if (mFitTable != nullptr)
            return mFitTable[index];
        else
            return 0.0f;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
