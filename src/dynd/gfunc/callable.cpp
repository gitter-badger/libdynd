//
// Copyright (C) 2011-13, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/gfunc/callable.hpp>
#include <dynd/dtypes/strided_dim_dtype.hpp>

using namespace std;
using namespace dynd;

void gfunc::callable::debug_print(std::ostream& o, const std::string& indent) const
{
    o << indent << "------ gfunc::callable\n";
    o << indent << " parameters_dtype: " << m_parameters_dtype << "\n";
    o << indent << " extra: " << m_extra << "\n";
    o << indent << " function: " << (void *)m_function << "\n";
    o << indent << "------" << endl;
}
