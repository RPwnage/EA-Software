/*! ************************************************************************************************/
/*!
\file fittable.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_FITTABLE_H
#define BLAZE_GAMEMANAGER_FITTABLE_H

#include "gamemanager/tdf/matchmaker_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    typedef eastl::vector<float> FitTableContainer; // ToDo: Remove

    class FitTable
    {
        NON_COPYABLE(FitTable);
    public:
        static const char8_t* GENERICRULE_FIT_TABLE_KEY; //!< config map key for rule definition's fit table list ("fitTable")
        static const char8_t* GENERICRULE_SPARSE_FIT_TABLE_KEY; //!< config map key for rule definition's fit table list ("sparseFitTable")

        static const char8_t* SPARSE_FIT_TABLE_DIAGONAL_VALUE;
        static const char8_t* SPARSE_FIT_TABLE_OFFDIAGONAL_VALUE;
        // this value will be used to fill in any random marked row and column
        static const char8_t* SPARSE_FIT_TABLE_RANDOM_VALUE;
        static const char8_t* SPARSE_FIT_TABLE_SPARSE_VALUES;
        static const char8_t* SPARSE_FIT_TABLE_FILL_COLUMN;
        static const char8_t* SPARSE_FIT_TABLE_FILL_ROW;

        static const char8_t* ABSTAIN_LITERAL_CONFIG_VALUE;
        static const char8_t* RANDOM_LITERAL_CONFIG_VALUE;

    public:
        FitTable();
        virtual ~FitTable();

        bool initialize(const FitTableList& fitTable, const SparseFitTable& sparseFitTable, const PossibleValuesList& possibleValues);

        float getMaxFitPercent() const { return mMaxFitPercent; }

        float getFitPercent(size_t myValueIndex, size_t otherEntityValueIndex) const;

    private:
        /*! ************************************************************************************************/
        /*!
            \brief Helper: build a vector<float> from a fitTableSequence.  Returns false if fitTableSequence
                is nullptr or empty.

            \param[in]  fitTableList - values for the fit table
            \param[in,out]  fitTable - the destination vector to fill in.
            \return true on success, false if the possibleValues sequence is missing/invalid
        ****************************************************************************************************/
        bool parseExplicitFitTable(const FitTableList& fitTableList, FitTableContainer &fitTable) const;

        /*! ************************************************************************************************/
        /*!
            \brief Helper: build a vector<float> from sparseFitTable values.  Returns false if the values
                are invalid.

            \param[in]  sparseFitTable - map of the value tags for the sparse fit table
            \param[in]  possibleValues - sequence of possible values for the rule
            \param[in,out]  fitTable - the destination vector to fill in.
            \return true on success, false if the possibleValues sequence is missing/invalid
        ****************************************************************************************************/
        bool parseSparseFitTable(const SparseFitTable& sparseFitTable, const PossibleValuesList& possibleValues, FitTableContainer &fitTable);

        /*! ************************************************************************************************/
        /*! \brief Helper: if this rule uses 'abstain' or 'random', init the fitTable's abstain or random rows and columns.

            \param[in]  randomValue - the value to put in the random row and column.
            \param[in]  possibleValues - sequence of possible values for the rule
            \param[in,out]  fitTable - the destination vector to fill in.
            \return true on success, false if the possibleValues sequence is missing/invalid
        ****************************************************************************************************/
        void handleRandomAbstainForSparseFitTable(float randomValue, const PossibleValuesList& possibleValues,
            FitTableContainer &fitTable);

        /*! ************************************************************************************************/
        /*!
            \brief Helper: add sparse values to the fitTable.

            \param[in]  sparseFitValues - sequence containing sparse value sequences in [x,y,value] format
            \param[in]  possibleValues - sequence of possible values for the rule
            \param[in,out]  fitTable - the destination vector to fill in.
            \return true on success, false if the possibleValues sequence is missing/invalid
        ****************************************************************************************************/
        bool parseSparseValues(const SparseFitValueList& sparseFitValues,
            const PossibleValuesList& possibleValues, FitTableContainer &fitTable);

        /*! ***********************************************************************************/
        /*!
            \brief Helper: finds the string index in the sequence or returns -1 if not found

            \param[in] valueToFind - string to match against
            \param[in] possibleValues - sequence of strings to match against
            \return index where valueToFind was found in the sequence or -1 if not found
        ***************************************************************************************/
        int16_t findIndex( const char8_t* valueToFind, const PossibleValuesList& possibleValues ) const;

        /*! ***********************************************************************************/
        /*!
            \brief Helper: fill an entire row in the fit matrix with a value

            \param[in] rowIndex - the index of the row to fill
            \param[in] fitMatrixWidth - how wide is the matrix
            \param[in] value - what value do we put there
            \param[in,out] fitTable - the container for the fit table values
        ***************************************************************************************/
        void setRow(size_t rowIndex, size_t fitMatrixWidth, float value, FitTableContainer &fitTable)
        {
            for (size_t index = 0; index < fitMatrixWidth; ++index )
                fitTable[rowIndex * fitMatrixWidth + index] = value;
        }

        /*! ***********************************************************************************/
        /*!
            \brief Helper: fill an entire column in the fit matrix with a value

            \param[in] colIndex - the index of the column to fill
            \param[in] fitMatrixWidth - how wide is the matrix
            \param[in] value - what value do we put there
            \param[in,out] fitTable - the container for the fit table values
        ***************************************************************************************/
        void setCol(size_t colIndex, size_t fitMatrixWidth, float value, FitTableContainer &fitTable)
        {
            for (size_t index = 0; index < fitMatrixWidth; ++index)
                fitTable[colIndex + index * fitMatrixWidth] = value;
        }

        /*! ************************************************************************************************/
        /*!
            \brief Helper: initialize & validate the rule's fitTable. Returns false on error.

                Note: the fit table depends on the possibleValues, so we must init the possibleValues first.

            \param[in]  fitTable - a representation of the fitTable (as a row-major array).
            \return true on success, false otherwise
        *************************************************************************************************/
        bool initFitTable(const FitTableContainer& fitTable, size_t numPossibleValues);

    // Members
    private:
        size_t mNumberPossibleValues;
        size_t mFitTableSize;
        float *mFitTable; //< row-major fit table array (containing the fitPercents).
        float mMaxFitPercent;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_FITTABLE_H
