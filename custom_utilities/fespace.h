//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 15 Nov 2017 $
//   Revision:            $Revision: 1.0 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_FESPACE_H_INCLUDED )
#define  KRATOS_ISOGEOMETRIC_APPLICATION_FESPACE_H_INCLUDED

// System includes
#include <vector>

// External includes
#include <boost/enable_shared_from_this.hpp>

// Project includes
#include "includes/define.h"
#include "includes/serializer.h"
#include "custom_utilities/nurbs/cell.h"
#include "custom_utilities/nurbs/cell_manager.h"


namespace Kratos
{

enum BoundarySide
{
    _LEFT_   = 0,
    _RIGHT_  = 1,
    _TOP_    = 3,
    _BOTTOM_ = 2,
    _FRONT_  = 4,
    _BACK_   = 5,
    _NUMBER_OF_BOUNDARY_SIDE = 6
};

/**
An FESpace is a collection of shape function defined over the parametric domain. An isogeometric FESpace can be a NURBS FESpace, a hierarchical NURBS FESpace, or a T-Splines FESpace.
 */
template<int TDim>
class FESpace
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(FESpace);

    /// Type definition
    typedef CellManager<Cell> cell_container_t;

    /// Default constructor
    FESpace() {}

    /// Destructor
    virtual ~FESpace()
    {
    }

    /// Helper to create new BSplinesFESpace pointer
    static FESpace<TDim>::Pointer Create()
    {
        return FESpace<TDim>::Pointer(new FESpace());
    }

    /// Get the number of basis functions defined over the FESpace
    virtual const std::size_t TotalNumber() const
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /// Get the order of the FESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /// Get the string representing the type of the FESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string representing the type of the FESpace
    static std::string StaticType()
    {
        std::stringstream ss;
        ss << "FESpace" << TDim << "D";
        return ss.str();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Get the values of the basis function i at point xi
    virtual double GetValue(const std::size_t& i, const std::vector<double>& xi) const
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /// Get the values of the basis functions at point xi
    virtual std::vector<double> GetValue(const std::vector<double>& xi) const
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Reset all the dof numbers for each grid function to -1.
    void ResetFunctionIndices()
    {
        if (mFunctionsIds.size() != this->TotalNumber())
            mFunctionsIds.resize(this->TotalNumber());
        std::fill(mFunctionsIds.begin(), mFunctionsIds.end(), -1);
    }

    /// Reset the function indices to a given values.
    /// This is useful when assigning the id for the boundary patch.
    void ResetFunctionIndices(const std::vector<std::size_t>& func_indices)
    {
        assert(func_indices.size() == this->TotalNumber());
        if (mFunctionsIds.size() != this->TotalNumber())
            mFunctionsIds.resize(this->TotalNumber());
        std::copy(func_indices.begin(), func_indices.end(), mFunctionsIds.begin());
    }

    /// Enumerate the dofs of each grid function. The enumeration algorithm is pretty straightforward.
    /// If the dof does not have pre-existing value, which assume it is -1, it will be assigned the incremental value.
    virtual std::size_t& Enumerate(std::size_t& start)
    {
        mGlobalToLocal.clear();
        for (std::size_t i = 0; i < mFunctionsIds.size(); ++i)
        {
            if (mFunctionsIds[i] == -1) mFunctionsIds[i] = start++;
            mGlobalToLocal[mFunctionsIds[i]] = i;
        }

        return start;
    }

    /// Access the function indices (aka global ids)
    const std::vector<std::size_t>& FunctionIndices() const {return mFunctionsIds;}

    /// Update the function indices using a map. The map shall be the mapping from old index to new index.
    void UpdateFunctionIndices(const std::map<std::size_t, std::size_t>& indices_map)
    {
        for (std::size_t i = 0; i < mFunctionsIds.size(); ++i)
        {
            std::map<std::size_t, std::size_t>::const_iterator it = indices_map.find(mFunctionsIds[i]);

            if (it == indices_map.end())
            {
                std::cout << "WARNING!!! the indices_map does not contain " << mFunctionsIds[i] << std::endl;
                continue;
            }

            mFunctionsIds[i] = it->second;
        }

        mGlobalToLocal.clear();
        for (std::size_t i = 0; i < mFunctionsIds.size(); ++i)
        {
            mGlobalToLocal[mFunctionsIds[i]] = i;
        }
    }

    /// Return the local id of a given global id
    std::size_t LocalId(const std::size_t& global_id) const
    {
        std::map<std::size_t, std::size_t>::const_iterator it = mGlobalToLocal.find(global_id);

        if (it == mGlobalToLocal.end())
            KRATOS_THROW_ERROR(std::logic_error, "The global id does not exist in global_to_local map", "")

        return it->second;
    }

    /// Return the local ids of given global ids
    std::vector<std::size_t> LocalId(const std::vector<std::size_t>& global_ids) const
    {
        std::vector<std::size_t> local_ids(global_ids.size());
        for (std::size_t i = 0; i < global_ids.size(); ++i)
            local_ids[i] = this->LocalId(global_ids[i]);
        return local_ids;
    }

    /// Check the compatibility between boundaries of two FESpacees
    virtual bool CheckBoundaryCompatibility(const FESpace<TDim>& rFESpace1, const BoundarySide& side1,
            const FESpace<TDim>& rFESpace2, const BoundarySide& side2) const
    {
        typename FESpace<TDim-1>::Pointer BFESpace1 = rFESpace1.ConstructBoundaryFESpace(side1);
        typename FESpace<TDim-1>::Pointer BFESpace2 = rFESpace1.ConstructBoundaryFESpace(side2);

        return (*BFESpace1) == (*BFESpace2);
    }

    /// Validate the FESpace before using
    virtual bool Validate() const
    {
        return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Extract the index of the functions on the boundary
    virtual std::vector<std::size_t> ExtractBoundaryFunctionIndices(const BoundarySide& side) const
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /// Assign the index for the functions on the boundary
    virtual void AssignBoundaryFunctionIndices(const BoundarySide& side, const std::vector<std::size_t>& func_indices)
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /// Construct the boundary FESpace based on side
    virtual typename FESpace<TDim-1>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Compare the two FESpacees in terms of its parametric information.
    virtual bool IsCompatible(const FESpace<TDim>& rOtherFESpace) const
    {
        return false;
    }

    /// Overload comparison operator
    virtual bool operator==(const FESpace<TDim>& rOther) const
    {
        return this->IsCompatible(rOther);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Fast function to get the opposite boundary side
    static BoundarySide OppositeBoundarySide(const BoundarySide& side)
    {
        if (side == _LEFT_) return _RIGHT_;
        else if (side == _RIGHT_) return _LEFT_;
        else if (side == _TOP_) return _BOTTOM_;
        else if (side == _BOTTOM_) return _TOP_;
        else if (side == _FRONT_) return _BACK_;
        else if (side == _BACK_) return _FRONT_;
        else
            KRATOS_THROW_ERROR(std::logic_error, "Invalid boundary side", side)
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Create the cell manager for all the cells in the support domain of the FESpace
    virtual typename cell_container_t::Pointer ConstructCellManager() const
    {
        KRATOS_THROW_ERROR(std::logic_error, "Calling base class function", __FUNCTION__)
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Overload assignment operator
    FESpace<TDim>& operator=(const FESpace<TDim>& rOther)
    {
        this->mFunctionsIds = rOther.mFunctionsIds;
        return *this;
    }

    /// Clone this FESpace, this is a deep copy operation
    virtual typename FESpace<TDim>::Pointer Clone() const
    {
        typename FESpace<TDim>::Pointer pNewFESpace = typename FESpace<TDim>::Pointer(new FESpace<TDim>());
        *pNewFESpace = *this;
        return pNewFESpace;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << Type() << ", Add = " << this;
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
        rOStream << " Function Indices:";
        for (std::size_t i = 0; i < mFunctionsIds.size(); ++i)
            rOStream << " " << mFunctionsIds[i];
    }

protected:

    /**
     * data for grid function interpolation
     */
    std::vector<std::size_t> mFunctionsIds; // this is to store a unique number of the shape function over the forest of FESpace(s).

    std::map<std::size_t, std::size_t> mGlobalToLocal;

private:

    /// Serializer
    friend class Serializer;

    virtual void save(Serializer& rSerializer) const
    {
    }

    virtual void load(Serializer& rSerializer)
    {
    }
};



/**
 * Template specific instantiation for null-D FESpace to terminate the compilation
 */
template<>
class FESpace<0>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(FESpace);

    /// Default constructor
    FESpace() : mFunctionId(-1) {}

    /// Destructor
    virtual ~FESpace() {}

    /// Get the number of basis functions defined over the FESpace
    virtual const std::size_t TotalNumber() const
    {
        return 0;
    }

    /// Get the order of the FESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        return 0;
    }

    /// Get the string describing the type of the FESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string describing the type of the FESpace
    static std::string StaticType()
    {
        return "FESpace0D";
    }

    /// Overload comparison operator
    virtual bool operator==(const FESpace<0>& rOther)
    {
        return true;
    }

    /// Reset all the dof numbers for each grid function to -1
    void ResetFunctionIndices()
    {
        mFunctionId = -1;
    }

    /// Reset the function indices to a given values
    void ResetFunctionIndices(const std::vector<std::size_t>& func_indices)
    {
        assert(func_indices.size() == 1);
        mFunctionId = func_indices[0];
    }

    /// Get the vector of function indices
    std::vector<std::size_t> FunctionIndices() const {return std::vector<std::size_t>{mFunctionId};}

    /// Check the compatibility between boundaries of two FESpacees
    virtual bool CheckBoundaryCompatibility(const FESpace<0>& rFESpace1, const BoundarySide& side1,
            const FESpace<0>& rFESpace2, const BoundarySide& side2) const
    {
        return true;
    }

    /// Validate the FESpace before using
    virtual bool Validate() const
    {
        return true;
    }

    // /// Construct the boundary FESpace based on side
    // virtual typename FESpace<-1>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    // {
    //     return NULL;
    // }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "FESpace<0>";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
    }

protected:

    std::size_t mFunctionId;

};


/**
 * Template specific instantiation for -1-D FESpace to terminate the compilation
 */
template<>
class FESpace<-1>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(FESpace);

    /// Default constructor
    FESpace() {}

    /// Destructor
    virtual ~FESpace() {}

    /// Get the number of basis functions defined over the FESpace
    virtual const std::size_t TotalNumber() const
    {
        return 0;
    }

    /// Get the order of the FESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        return 0;
    }

    /// Get the string describing the type of the FESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string describing the type of the FESpace
    static std::string StaticType()
    {
        return "FESpace<-1>D";
    }

    /// Overload comparison operator
    virtual bool operator==(const FESpace<-1>& rOther)
    {
        return true;
    }

    /// Check the compatibility between boundaries of two FESpacees
    virtual bool CheckBoundaryCompatibility(const FESpace<-1>& rFESpace1, const BoundarySide& side1,
            const FESpace<-1>& rFESpace2, const BoundarySide& side2) const
    {
        return true;
    }

    /// Validate the FESpace before using
    virtual bool Validate() const
    {
        return true;
    }

    // /// Construct the boundary FESpace based on side
    // virtual typename FESpace<-2>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    // {
    //     return NULL;
    // }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "FESpace<-1>";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
    }
};


/**
 * Template specific instantiation for -2-D FESpace to terminate the compilation
 */
template<>
class FESpace<-2>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(FESpace);

    /// Default constructor
    FESpace() {}

    /// Destructor
    virtual ~FESpace() {}

    /// Get the number of basis functions defined over the FESpace
    virtual const std::size_t TotalNumber() const
    {
        return 0;
    }

    /// Get the order of the FESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        return 0;
    }

    /// Get the string describing the type of the FESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string describing the type of the FESpace
    static std::string StaticType()
    {
        return "FESpace<-2>D";
    }

    /// Overload comparison operator
    virtual bool operator==(const FESpace<-2>& rOther)
    {
        return true;
    }

    /// Check the compatibility between boundaries of two FESpacees
    virtual bool CheckBoundaryCompatibility(const FESpace<-2>& rFESpace1, const BoundarySide& side1,
            const FESpace<-2>& rFESpace2, const BoundarySide& side2) const
    {
        return true;
    }

    /// Validate the FESpace before using
    virtual bool Validate() const
    {
        return true;
    }

    // /// Construct the boundary FESpace based on side
    // virtual typename FESpace<-3>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    // {
    //     return NULL;
    // }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "FESpace<-2>";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
    }
};


/// output stream function
template<int TDim>
inline std::ostream& operator <<(std::ostream& rOStream, const FESpace<TDim>& rThis)
{
    rOStream << "-------------Begin FESpaceInfo-------------" << std::endl;
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);
    rOStream << std::endl;
    rOStream << "-------------End FESpaceInfo-------------";
    return rOStream;
}

} // namespace Kratos.

#undef DEBUG_DESTROY

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_FESPACE_H_INCLUDED defined

