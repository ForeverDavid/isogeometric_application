//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 23 Apr 2015 $
//   Revision:            $Revision: 1.0 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_CELL_H_INCLUDED )
#define  KRATOS_ISOGEOMETRIC_APPLICATION_CELL_H_INCLUDED

// System includes
#include <string>
#include <vector>
#include <iostream>

// External includes

// Project includes
#include "includes/define.h"
#include "custom_utilities/nurbs/knot.h"

namespace Kratos
{

class Cell;

template<int TDim>
struct Cell_Helper
{
    static bool IsCovered(const Cell& this_cell, const Cell& other_cell);
};

/**
    Represent a cell in NURBS/hierarchical B-Splines/T-splines mesh topology.
    A cell is the smaller unit in the isogeometric topology mesh (e.g. it represents an element, or Bezier element of the T-splines basis function).
    A cell is determined by its topology index of its vertices.
 */
class Cell
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(Cell);

    /// Type definitions
    typedef Knot<double> KnotType;
    typedef KnotType::Pointer knot_t;

    /// Constructor with knots
    Cell(const std::size_t& Id, knot_t pLeft, knot_t pRight, knot_t pDown, knot_t pUp)
    : mId(Id), mpLeft(pLeft), mpRight(pRight), mpUp(pUp), mpDown(pDown), mpAbove(new KnotType(0.0)), mpBelow(new KnotType(0.0))
    {}

    /// Constructor with knots
    Cell(const std::size_t& Id, knot_t pLeft, knot_t pRight, knot_t pDown, knot_t pUp, knot_t pBelow, knot_t pAbove)
    : mId(Id), mpLeft(pLeft), mpRight(pRight), mpUp(pUp), mpDown(pDown), mpAbove(pAbove), mpBelow(pBelow)
    {}

    /// Destructor
    virtual ~Cell() {}

    /// Get the Id
    std::size_t Id() const {return mId;}

    /// Get the coordinates
    knot_t Left() const {return mpLeft;}
    int LeftIndex() const {return mpLeft->Index();}
    double LeftValue() const {return mpLeft->Value();}

    knot_t Right() const {return mpRight;}
    int RightIndex() const {return mpRight->Index();}
    double RightValue() const {return mpRight->Value();}

    knot_t Up() const {return mpUp;}
    int UpIndex() const {return mpUp->Index();}
    double UpValue() const {return mpUp->Value();}

    knot_t Down() const {return mpDown;}
    int DownIndex() const {return mpDown->Index();}
    double DownValue() const {return mpDown->Value();}

    knot_t Above() const {return mpAbove;}
    int AboveIndex() const {return mpAbove->Index();}
    double AboveValue() const {return mpAbove->Value();}

    knot_t Below() const {return mpBelow;}
    int BelowIndex() const {return mpBelow->Index();}
    double BelowValue() const {return mpBelow->Value();}

    /// Check if the cell is covered by knot spans; the comparison is based on indexing, so the knot vectors must be sorted a priori
    bool IsCovered(const std::vector<int>& rKnotsIndex1, const std::vector<int>& rKnotsIndex2) const
    {
        int anchor_cover_xi_min  = *std::min_element(rKnotsIndex1.begin(), rKnotsIndex1.end());
        int anchor_cover_xi_max  = *std::max_element(rKnotsIndex1.begin(), rKnotsIndex1.end());
        int anchor_cover_eta_min = *std::min_element(rKnotsIndex2.begin(), rKnotsIndex2.end());
        int anchor_cover_eta_max = *std::max_element(rKnotsIndex2.begin(), rKnotsIndex2.end());

        if(     LeftIndex()  >= anchor_cover_xi_min
            && RightIndex() <= anchor_cover_xi_max
            && DownIndex()  >= anchor_cover_eta_min
            && UpIndex()    <= anchor_cover_eta_max )
        {
            return true;
        }
        return false;
    }

    /// Check if this cell is covered by another cell
    template<int TDim>
    bool IsCovered(Cell::ConstPointer p_cell) const
    {
        return Cell_Helper<TDim>::IsCovered(*this, *p_cell);
    }

    /// Check if this cell cover a point in knot space
    bool IsCoverage(const double& rXi, const double& rEta) const
    {
        if(    LeftValue()  <= rXi  && RightValue() >= rXi
            && DownValue()  <= rEta && UpValue()    >= rEta )
            return true;
        return false;
    }

    /// Check if this cell cover a point in knot space
    bool IsCoverage(const double& rXi, const double& rEta, const double& rZeta) const
    {
        if(    LeftValue()  <= rXi   && RightValue() >= rXi
            && DownValue()  <= rEta  && UpValue()    >= rEta
            && BelowValue() <= rZeta && AboveValue() >= rZeta )
            return true;
        return false;
    }

    /// check if this cell is the same as the reference cell. Two cells are the same if it has the same bounding knot values.
    bool IsSame(const Cell::Pointer p_cell, int Dim, double tol) const
    {
        if(    fabs( LeftValue()  - p_cell->LeftValue() )  < tol
            && fabs( RightValue() - p_cell->RightValue() ) < tol
            && fabs( DownValue()  - p_cell->DownValue() )  < tol
            && fabs( UpValue()    - p_cell->UpValue() )    < tol
            && fabs( BelowValue() - p_cell->BelowValue() ) < tol
            && fabs( AboveValue() - p_cell->AboveValue() ) < tol )
                return true;
        return false;
    }

    /// Clear internal data of this cell
    void Reset()
    {
        mSupportedAnchors.clear();
        mAnchorWeights.clear();
        mCrows.clear();
    }

    /// Add supported anchor and the respective extraction operator of this cell to the anchor
    void AddAnchor(const unsigned int& Id, const double& W, const Vector& Crow)
    {
        mSupportedAnchors.push_back(Id);
        mAnchorWeights.push_back(W);
        mCrows.push_back(Crow);
    }

    /// Get the number of supported anchors of this cell. In the other language, it is the number of basis functions that the support domain includes this cell.
    std::size_t NumberOfAnchors() const {return mSupportedAnchors.size();}

    /// Get the supported anchors of this cell
    const std::vector<std::size_t>& GetSupportedAnchors() const {return mSupportedAnchors;}

    /// Get the weights of all the supported anchors
    const std::vector<double>& GetAnchorWeights() const {return mAnchorWeights;}
    void GetAnchorWeights(Vector& rWeights) const
    {
        if(rWeights.size() != mAnchorWeights.size())
            rWeights.resize(mAnchorWeights.size());
        std::copy(mAnchorWeights.begin(), mAnchorWeights.end(), rWeights.begin());
    }

    /// Get the extraction operator matrix
    Matrix GetExtractionOperator() const
    {
        Matrix M;
        M.resize(mCrows.size(), mCrows[0].size());
        for(std::size_t i = 0; i < mCrows.size(); ++i)
            noalias(row(M, i)) = mCrows[i];
        return M;
    }

    /// Get the extraction operator as CSR triplet
    void GetExtractionOperator(std::vector<int>& rowPtr, std::vector<int>& colInd, std::vector<double>& values) const
    {
        int cnt = 0;
        rowPtr.push_back(cnt);
        for(std::size_t i = 0; i < mCrows.size(); ++i)
        {
            for(std::size_t j = 0; j < mCrows[i].size(); ++j)
            {
                if(mCrows[i](j) != 0)
                {
                    colInd.push_back(j);
                    values.push_back(mCrows[i](j));
                    ++cnt;
                }
            }
            rowPtr.push_back(cnt);
        }
    }

    /// Implement relational operator for automatic arrangement in container
    inline bool operator==(const Cell& rA) const
    {
        return this->Id() == rA.Id();
    }

    inline bool operator<(const Cell& rA) const
    {
        return this->Id() < rA.Id();
    }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "{Id:" << Id() << ",range:([" << LeftIndex() << " " << RightIndex() << "];[" << DownIndex() << " " << UpIndex() << "];[" << BelowIndex() << " " << AboveIndex() << "])";
        rOStream << "<=>([" << LeftValue() << " " << RightValue() << "];[" << DownValue() << " " << UpValue() << "];[" << BelowValue() << " " << AboveValue() << "])}";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
        rOStream << ", supporting anchors: ";
        rOStream << "(";
        for(std::vector<std::size_t>::const_iterator it = mSupportedAnchors.begin(); it != mSupportedAnchors.end(); ++it)
            rOStream << " " << (*it);
        rOStream << ")";
    }

private:

    std::size_t mId;
    knot_t mpLeft;
    knot_t mpRight;
    knot_t mpUp;
    knot_t mpDown;
    knot_t mpAbove;
    knot_t mpBelow;
    std::vector<std::size_t> mSupportedAnchors;
    std::vector<double> mAnchorWeights; // weight of the anchor
    std::vector<Vector> mCrows; // bezier extraction operator row to each anchor
};

template<>
inline bool Cell_Helper<1>::IsCovered(const Cell& this_cell, const Cell& other_cell)
{
    if(    this_cell.LeftValue()  >= other_cell.LeftValue() && this_cell.RightValue() <= other_cell.RightValue() )
        return true;
    return false;
}

template<>
inline bool Cell_Helper<2>::IsCovered(const Cell& this_cell, const Cell& other_cell)
{
    if(    this_cell.LeftValue()  >= other_cell.LeftValue() && this_cell.RightValue() <= other_cell.RightValue()
        && this_cell.DownValue()  >= other_cell.DownValue() && this_cell.UpValue()    <= other_cell.UpValue() )
        return true;
    return false;
}

template<>
inline bool Cell_Helper<3>::IsCovered(const Cell& this_cell, const Cell& other_cell)
{
    if(    this_cell.LeftValue()  >= other_cell.LeftValue() && this_cell.RightValue() <= other_cell.RightValue()
        && this_cell.DownValue()  >= other_cell.DownValue() && this_cell.UpValue()    <= other_cell.UpValue()
        && this_cell.BelowValue() >= other_cell.BelowValue() && this_cell.AboveValue() <= other_cell.AboveValue() )
        return true;
    return false;
}

/// output stream function
inline std::ostream& operator <<(std::ostream& rOStream, const Cell& rThis)
{
    rOStream << "cell ";
    rThis.PrintInfo(rOStream);
    rThis.PrintData(rOStream);
    return rOStream;
}

}// namespace Kratos.

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_CELL_H_INCLUDED

