//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 21 Nov 2017 $
//   Revision:            $Revision: 1.0 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_TRANSFORMATION_H_INCLUDED )
#define  KRATOS_ISOGEOMETRIC_APPLICATION_TRANSFORMATION_H_INCLUDED

// System includes
#include <string>
#include <iostream>

// External includes
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/operation.hpp>

// Project includes
#include "includes/define.h"
#include "containers/array_1d.h"

namespace Kratos
{

/**
Represent a transformation in homogeneous coordinates
 */
template<typename TDataType>
class Transformation
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(Transformation);

    /// Type definitions
    typedef boost::numeric::ublas::matrix<TDataType> MatrixType;
    typedef boost::numeric::ublas::zero_matrix<TDataType> ZeroMatrixType;
    typedef boost::numeric::ublas::identity_matrix<TDataType> IdentityMatrixType;

    /// Default constructor
    Transformation()
    {
        mTransMat.resize(4, 4);
        noalias(mTransMat) = IdentityMatrixType(4, 4);
    }

    /// Destructor
    virtual ~Transformation() {}

    /// Get the homogeneous transformation matrix
    const MatrixType& Mat() const {return mTransMat;}

    /// Append the transformation
    void AppendTransformation(const Transformation& rOther)
    {
        MatrixType temp = prod(rOther.Mat(), this->mTransMat);
        noalias(this->mTransMat) = temp;
    }

    /// Apply the transformation for a point in 3D
    template<typename TVectorType>
    void ApplyTransformation(TVectorType& value) const
    {
        double new_value[3];
        for (std::size_t i = 0; i < 3; ++i)
        {
            new_value[i] = 0.0;
            for (std::size_t j = 0; j < 3; ++j)
                new_value[i] += mTransMat(i, j) * value[j];
            new_value[i] += mTransMat(i, 3);
        }
        for (std::size_t i = 0; i < 3; ++i)
            value[i] = new_value[i];
    }

    /// overload operator ()
    TDataType& operator() (const int& i, const int& j)
    {
        return mTransMat(i, j);
    }

    /// overload operator ()
    const TDataType& operator() (const int& i, const int& j) const
    {
        return mTransMat(i, j);
    }

    /// Assignment operator
    Transformation& operator=(const Transformation& rOther)
    {
        this->mTransMat = rOther.mTransMat;
        return *this;
    }

    /// Multiplication operator
    Transformation& operator*=(const Transformation& rOther)
    {
        noalias(this->mTransMat) = prod(rOther.mTransMat, this->mTransMat);
        return *this;
    }

    /// Multiplication operator
    friend Transformation operator*(const Transformation& t1, const Transformation& t2)
    {
        Transformation t = t1;
        t *= t2;
        return t;
    }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "Homogeneous Transformation";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
        // print the control point in homogeneous coordinates
        rOStream << mTransMat;
    }

protected:

    MatrixType mTransMat;

};

/// output stream function
template<class TDataType>
inline std::ostream& operator <<(std::ostream& rOStream, const Transformation<TDataType>& rThis)
{
    rThis.PrintInfo(rOStream);
    rOStream << ": ";
    rThis.PrintData(rOStream);
    return rOStream;
}

}// namespace Kratos.

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_TRANSFORMATION_H_INCLUDED

