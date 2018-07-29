/**@Copyright All rights reserved.
 *@File no_copy.h
 *@Auth lily
 */
#ifndef __NO_COPY_H__
#define __NO_COPY_H__

namespace utils {

//! Base class for types that should not be assigned.
struct NoAssign {
    //! Deny assignment
    void operator = ( const NoAssign &) = delete;

    //! Define default constructor
    NoAssign() = default;
};

//! Base class for types that should not be copied or assigned.
struct NoCopy : NoAssign {
    //! Deny copy constructor
    NoCopy( const NoCopy& ) = delete;

    //! Allow default constructor
    NoCopy() = default;
    
    //! Allow default destructor
    ~NoCopy() = default;
};

} //! namespace utils

#endif //! __NO_COPY_H__