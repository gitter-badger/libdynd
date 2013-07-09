//
// Copyright (C) 2011-13 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DYND__DATETIME_ASSIGNMENT_KERNELS_HPP_
#define _DYND__DATETIME_ASSIGNMENT_KERNELS_HPP_

#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/dtypes/date_dtype.hpp>

namespace dynd {

/**
 * Makes a kernel which converts strings to datetimes.
 */
size_t make_string_to_datetime_assignment_kernel(
                hierarchical_kernel *out, size_t offset_out,
                const ndt::type& dst_datetime_dt, const char *dst_metadata,
                const ndt::type& src_string_dt, const char *src_metadata,
                kernel_request_t kernreq, assign_error_mode errmode,
                const eval::eval_context *ectx);

/**
 * Makes a kernel which converts datetimes to strings.
 */
size_t make_datetime_to_string_assignment_kernel(
                hierarchical_kernel *out, size_t offset_out,
                const ndt::type& dst_string_dt, const char *dst_metadata,
                const ndt::type& src_datetime_dt, const char *src_metadata,
                kernel_request_t kernreq, assign_error_mode errmode,
                const eval::eval_context *ectx);

} // namespace dynd

#endif // _DYND__DATETIME_ASSIGNMENT_KERNELS_HPP_

