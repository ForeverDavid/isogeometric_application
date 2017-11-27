//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 8 Nov 2017 $
//   Revision:            $Revision: 1.0 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_CONTROL_GRID_UTILITY_H_INCLUDED)
#define  KRATOS_ISOGEOMETRIC_APPLICATION_CONTROL_GRID_UTILITY_H_INCLUDED

// System includes
#include <vector>

// External includes

// Project includes
#include "includes/define.h"
#include "custom_utilities/control_point.h"
#include "custom_utilities/control_grid.h"
#include "custom_utilities/point_based_control_grid.h"

namespace Kratos
{


template<int TDim, typename TDataType>
struct ControlGridUtility_Helper
{
    /// Generate regular control grid with a specific data type
    static typename ControlGrid<TDataType>::Pointer CreateRegularZeroControlGrid(const std::string& Name, const std::vector<std::size_t>& ngrid)
    {
        KRATOS_THROW_ERROR(std::logic_error, __FUNCTION__, "is not implemented")
    }
};


/**
Utility class to manipulate the control grid and Helpers to generate control grid for isogeometric analysis.
 */
class ControlGridUtility
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(ControlGridUtility);

    /// Type definition
    typedef ControlPoint<double> ControlPointType;

    /// Default constructor
    ControlGridUtility() {}

    /// Destructor
    virtual ~ControlGridUtility() {}


    /// Transform a control grid to new control grid by a matrix multiplication.
    template<typename TDataType, typename TMatrixType>
    static void Transform(const TMatrixType& TformMat,
            const ControlGrid<TDataType>& rControlGrid,
            ControlGrid<TDataType>& rNewControlGrid)
    {
        // ensure the transformation matrix size is compatible
        if (TformMat.size1() != rControlGrid.Size())
            KRATOS_THROW_ERROR(std::logic_error, "The first size of the transformation matrix is not compatible with old grid function size", "")

        if (TformMat.size2() != rNewControlGrid.Size())
            KRATOS_THROW_ERROR(std::logic_error, "The second size of the transformation matrix is not compatible with new grid function size", "")

        // compute new data and store
        for (std::size_t i = 0; i < TformMat.size2(); ++i)
        {
            TDataType NewData(0.0);
            for (std::size_t j = 0; j < TformMat.size1(); ++j)
            {
                if (TformMat(j, i) != 0.0)
                    NewData += TformMat(j, i) * rControlGrid.GetData(j);
            }
            rNewControlGrid.SetData(i, NewData);
        }
    }


    /// Apply the homogeneous transformation to a grid of control points
    template<typename TDataType>
    static void ApplyTransformation(ControlGrid<ControlPointType>& rControlPointGrid, const Transformation<TDataType>& trans)
    {
        for (std::size_t i = 0; i < rControlPointGrid.size(); ++i)
        {
            ControlPointType point = rControlPointGrid.GetData(i);
            point.ApplyTransformation(trans);
            rControlPointGrid.SetData(i, point);
        }
    }



    /// Apply the homogeneous transformation to a grid of points
    template<typename TDataType>
    static void ApplyTransformation(ControlGrid<array_1d<TDataType, 3> >& rControlGrid, const Transformation<TDataType>& trans)
    {
        for (std::size_t i = 0; i < rControlGrid.size(); ++i)
        {
            array_1d<TDataType, 3> point = rControlGrid.GetData(i);
            trans.ApplyTransformation(point);
            rControlGrid.SetData(i, point);
        }
    }



    /// Helper function to create the point-based control grid based on Variable and FESpace
    template<typename TDataType, class TFESpaceType>
    static typename ControlGrid<TDataType>::Pointer CreatePointBasedControlGrid(
            const Variable<TDataType>& rVariable, typename TFESpaceType::Pointer pFESpace)
    {
        return PointBasedControlGrid<Variable<TDataType>, TFESpaceType>::Create(rVariable, pFESpace);
    }


    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "ControlGridUtility";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
    }
};

/// output stream function
inline std::ostream& operator <<(std::ostream& rOStream, const ControlGridUtility& rThis)
{
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);
    return rOStream;
}

} // namespace Kratos.

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_CONTROL_GRID_UTILITY_H_INCLUDED defined

