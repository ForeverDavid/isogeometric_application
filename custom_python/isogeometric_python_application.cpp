/*
LICENSE: see isogeometric_application/LICENSE.txt
*/

//
//   Project Name:        Kratos
//   Last modified by:    $Author: hbui $
//   Date:                $Date: 2013-08-18 $
//   Revision:            $Revision: 1.1 $
//
//

// System includes

#if defined(KRATOS_PYTHON)
// External includes
#include <boost/python.hpp>

#include "custom_python/add_utilities_to_python.h"
#include "custom_python/add_processes_to_python.h"
#include "custom_python/add_io_to_python.h"

////utilities

// Project includes
#include "includes/define.h"
#include "isogeometric_application.h"


namespace Kratos
{

namespace Python
{


BOOST_PYTHON_MODULE(KratosIsogeometricApplication)
{

    using namespace boost::python;

    class_<KratosIsogeometricApplication,
           KratosIsogeometricApplication::Pointer,
           bases<KratosApplication>, boost::noncopyable > ("KratosIsogeometricApplication")
           ;

    IsogeometricApplication_AddCustomUtilities1ToPython();
    IsogeometricApplication_AddCustomUtilities2ToPython();
    IsogeometricApplication_AddCustomUtilities4ToPython();
    IsogeometricApplication_AddProcessesToPython();
    IsogeometricApplication_AddIOToPython();

    KRATOS_REGISTER_IN_PYTHON_VARIABLE( NUM_DIVISION_1 )
    KRATOS_REGISTER_IN_PYTHON_VARIABLE( NUM_DIVISION_2 )
    KRATOS_REGISTER_IN_PYTHON_VARIABLE( NUM_DIVISION_3 )
    KRATOS_REGISTER_IN_PYTHON_3D_VARIABLE_WITH_COMPONENTS( LOCAL_COORDINATES )

}


} // namespace Python.

} // namespace Kratos.

#endif // KRATOS_PYTHON defined
