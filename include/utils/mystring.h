#ifndef __MYSTRING_H__
#define __MYSTRING_H__

#include <string>
#include "tbb/tbb_allocator.h"

namespace utils {

//! String type with scalable allocator.
using MyString = std::basic_string<char, std::char_traits<char>, tbb::tbb_allocator<char>>;

} //! utils

#endif //! #ifndef __MYSTRING_H__