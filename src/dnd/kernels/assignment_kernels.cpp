//
// Copyright (C) 2012 Continuum Analytics
// All rights reserved.
//
#include <dnd/dtype.hpp>
#include <dnd/kernels/assignment_kernels.hpp>
#include <dnd/kernels/chained_unary_kernels.hpp>
#include "multiple_assigner_builtin.hpp"

using namespace std;
using namespace dnd;

static assignment_function_t single_assign_table[builtin_type_id_count][builtin_type_id_count][4] =
{
#define ERROR_MODE_LEVEL(dst_type, src_type) { \
        (assignment_function_t)&single_assigner_builtin<dst_type, src_type, assign_error_none>::assign, \
        (assignment_function_t)&single_assigner_builtin<dst_type, src_type, assign_error_overflow>::assign, \
        (assignment_function_t)&single_assigner_builtin<dst_type, src_type, assign_error_fractional>::assign, \
        (assignment_function_t)&single_assigner_builtin<dst_type, src_type, assign_error_inexact>::assign \
    }

#define SRC_TYPE_LEVEL(dst_type) { \
        ERROR_MODE_LEVEL(dst_type, dnd_bool), \
        ERROR_MODE_LEVEL(dst_type, int8_t), \
        ERROR_MODE_LEVEL(dst_type, int16_t), \
        ERROR_MODE_LEVEL(dst_type, int32_t), \
        ERROR_MODE_LEVEL(dst_type, int64_t), \
        ERROR_MODE_LEVEL(dst_type, uint8_t), \
        ERROR_MODE_LEVEL(dst_type, uint16_t), \
        ERROR_MODE_LEVEL(dst_type, uint32_t), \
        ERROR_MODE_LEVEL(dst_type, uint64_t), \
        ERROR_MODE_LEVEL(dst_type, float), \
        ERROR_MODE_LEVEL(dst_type, double), \
        ERROR_MODE_LEVEL(dst_type, complex<float>), \
        ERROR_MODE_LEVEL(dst_type, complex<double>) \
    }
    
    SRC_TYPE_LEVEL(dnd_bool),
    SRC_TYPE_LEVEL(int8_t),
    SRC_TYPE_LEVEL(int16_t),
    SRC_TYPE_LEVEL(int32_t),
    SRC_TYPE_LEVEL(int64_t),
    SRC_TYPE_LEVEL(uint8_t),
    SRC_TYPE_LEVEL(uint16_t),
    SRC_TYPE_LEVEL(uint32_t),
    SRC_TYPE_LEVEL(uint64_t),
    SRC_TYPE_LEVEL(float),
    SRC_TYPE_LEVEL(double),
    SRC_TYPE_LEVEL(complex<float>),
    SRC_TYPE_LEVEL(complex<double>)
#undef SRC_TYPE_LEVEL
#undef ERROR_MODE_LEVEL
};

assignment_function_t dnd::get_builtin_dtype_assignment_function(type_id_t dst_type_id, type_id_t src_type_id,
                                                                assign_error_mode errmode)
{
    // Do a table lookup for the built-in range of dtypes
    if (dst_type_id >= bool_type_id && dst_type_id <= complex_float64_type_id &&
            src_type_id >= bool_type_id && src_type_id <= complex_float64_type_id) {
        return single_assign_table[dst_type_id][src_type_id][errmode];
    } else {
        return NULL;
    }
}

void dnd::multiple_assignment_kernel(char *dst, intptr_t dst_stride, const char *src, intptr_t src_stride,
                                    intptr_t count, const AuxDataBase *auxdata)
{
    assignment_function_t asn = get_auxiliary_data<assignment_function_t>(auxdata);


    char *dst_cached = reinterpret_cast<char *>(dst);
    const char *src_cached = reinterpret_cast<const char *>(src);

    for (intptr_t i = 0; i < count; ++i) {
        asn(dst_cached, src_cached);
        dst_cached += dst_stride;
        src_cached += src_stride;
    }
}



#define DND_XSTRINGIFY(s) #s
#define DND_STRINGIFY(s) DND_XSTRINGIFY(s)

#define DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, src_type, BASE_FN, errmode) \
    case type_id_of<src_type>::value: \
        /*DEBUG_COUT << "returning " << DND_STRINGIFY(dst_type) << " " << DND_STRINGIFY(src_type) << " " << DND_STRINGIFY(ASSIGN_FN) << "\n";*/ \
        if (dst_fixedstride == sizeof(dst_type)) { \
            if (src_fixedstride == sizeof(src_type)) { \
                out_kernel.kernel = &dnd::multiple_assigner_builtin<dst_type, src_type, errmode>::BASE_FN##_contigstride_contigstride; \
            } else if (src_fixedstride == 0) { \
                out_kernel.kernel = &dnd::multiple_assigner_builtin<dst_type, src_type, errmode>::BASE_FN##_contigstride_zerostride; \
            } else { \
                out_kernel.kernel = &dnd::multiple_assigner_builtin<dst_type, src_type, errmode>::BASE_FN##_anystride_anystride; \
            } \
        } else { \
            if (src_fixedstride == 0) { \
                out_kernel.kernel = &dnd::multiple_assigner_builtin<dst_type, src_type, errmode>::BASE_FN##_anystride_zerostride; \
            } else { \
                out_kernel.kernel = &dnd::multiple_assigner_builtin<dst_type, src_type, errmode>::BASE_FN##_anystride_anystride; \
            } \
        } \
        return;

#define DTYPE_ASSIGN_ANY_TO_DST_SWITCH_STMT(dst_type, BASE_FN, errmode) \
        switch (src_type_id) { \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, dnd_bool, BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, int8_t,   BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, int16_t,  BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, int32_t,  BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, int64_t,  BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, uint8_t,  BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, uint16_t, BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, uint32_t, BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, uint64_t, BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, float,    BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, double,   BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, std::complex<float>,    BASE_FN, errmode); \
            DTYPE_ASSIGN_SRC_TO_DST_SINGLE_CASE(dst_type, std::complex<double>,   BASE_FN, errmode); \
        }


#define DTYPE_ASSIGN_ANY_TO_DST_CASE(dst_type, BASE_FN, errmode) \
    case type_id_of<dst_type>::value: \
        DTYPE_ASSIGN_ANY_TO_DST_SWITCH_STMT(dst_type, BASE_FN, errmode); \
        break

#define DTYPE_ASSIGN_ANY_TO_ANY_SWITCH(BASE_FN, errmode) \
    switch (dst_type_id) { \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(dnd_bool, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(int8_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(int16_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(int32_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(int64_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(uint8_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(uint16_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(uint32_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(uint64_t, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(float, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(double, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(std::complex<float>, BASE_FN, errmode); \
        DTYPE_ASSIGN_ANY_TO_DST_CASE(std::complex<double>, BASE_FN, errmode); \
    }

void dnd::get_builtin_dtype_assignment_kernel(
                    type_id_t dst_type_id, intptr_t dst_fixedstride,
                    type_id_t src_type_id, intptr_t src_fixedstride,
                    assign_error_mode errmode,
                    kernel_instance<unary_operation_t>& out_kernel)
{
    if (errmode == assign_error_fractional) {
        // The default error mode is fractional, so do specializations for it.

        // Make sure there's no stray auxiliary data
        out_kernel.auxdata.free();

        DTYPE_ASSIGN_ANY_TO_ANY_SWITCH(assign, assign_error_fractional);
    } else if (errmode == assign_error_none) {
        // The no-checking error mode also gets specializations, as it's intended for speed

        // Make sure there's no stray auxiliary data
        out_kernel.auxdata.free();

        DTYPE_ASSIGN_ANY_TO_ANY_SWITCH(assign, assign_error_none);
    } else {
        // Use a multiple assignment kernel with a assignment function for all the other cases.
        assignment_function_t asn = get_builtin_dtype_assignment_function(dst_type_id, src_type_id, errmode);
        if (asn != NULL) {
            out_kernel.kernel = &multiple_assignment_kernel;
            make_auxiliary_data<assignment_function_t>(out_kernel.auxdata, asn);
            return;
        }
    }
}


void dnd::get_dtype_assignment_kernel(
                    const dtype& dst_dt, intptr_t dst_fixedstride,
                    const dtype& src_dt, intptr_t src_fixedstride,
                    assign_error_mode errmode,
                    kernel_instance<unary_operation_t>& out_kernel)
{
    // If the casting can be done losslessly, disable the error check to find faster code paths
    if (errmode != assign_error_none && is_lossless_assignment(dst_dt, src_dt)) {
        errmode = assign_error_none;
    }

    // Assignment of built-in types
    if (dst_dt.extended() == NULL && src_dt.extended() == NULL) {
        get_builtin_dtype_assignment_kernel(dst_dt.type_id(), dst_fixedstride,
                            src_dt.type_id(), src_fixedstride, errmode, out_kernel);
        return;
    }

    // Assignment of expression dtypes
    if (src_dt.kind() == expression_kind) {
        if (src_dt.value_dtype() == dst_dt) {
            // If the source dtype's value_dtype matches the destination, it's easy
            src_dt.get_storage_to_value_operation(dst_fixedstride, src_fixedstride, out_kernel);
            return;
        } else if (dst_dt.kind() != expression_kind) {
            // Otherwise we have to chain a dtype casting function from src to the dst value_dtype
            deque<kernel_instance<unary_operation_t> > kernels;
            deque<intptr_t> element_sizes;

            // One link is a cast operation from src_dt.value_dtype() to dst_dt
            element_sizes.push_back(src_dt.value_dtype().itemsize());
            kernels.push_back(kernel_instance<unary_operation_t>());
            get_dtype_assignment_kernel(dst_dt, dst_fixedstride,
                                src_dt.value_dtype(), src_dt.value_dtype().itemsize(),
                                errmode, kernels.back());
            
            push_front_dtype_storage_to_value_kernels(src_dt, dst_fixedstride, src_dt.value_dtype().itemsize(),
                                kernels, element_sizes);

            make_chained_unary_kernel(kernels, element_sizes, out_kernel);
            return;
        } else {
            // Now we need a chain from src's storage to src's value,
            // a casting function to dst's value, then a chain from
            // dst's value to dst's storage
            std::deque<kernel_instance<unary_operation_t> > kernels;
            std::deque<intptr_t> element_sizes;

            push_front_dtype_storage_to_value_kernels(src_dt, src_dt.value_dtype().itemsize(), src_fixedstride,
                                kernels, element_sizes);

            element_sizes.push_back(src_dt.value_dtype().itemsize());
            // If needed, a casting from src's value to dst's value
            if (src_dt.value_dtype() != dst_dt.value_dtype()) {
                element_sizes.push_back(dst_dt.value_dtype().itemsize());
                kernels.push_back(kernel_instance<unary_operation_t>());
                get_dtype_assignment_kernel(dst_dt.value_dtype(), dst_dt.value_dtype().itemsize(),
                                    src_dt.value_dtype(), src_dt.value_dtype().itemsize(),
                                    errmode, kernels.back());
            }

            push_back_dtype_value_to_storage_kernels(dst_dt, dst_fixedstride, dst_dt.value_dtype().itemsize(),
                                kernels, element_sizes);

            make_chained_unary_kernel(kernels, element_sizes, out_kernel);
            return;
        }
    } else if (dst_dt.kind() == expression_kind) {
        if (src_dt == dst_dt.value_dtype()) {
            // If the destination dtype's storage matches the source, it's easy
            dst_dt.get_value_to_storage_operation(dst_fixedstride, src_fixedstride, out_kernel);
            return;
        } else {
            // Otherwise we have to chain a dtype casting function from src to the dst value_dtype
            std::deque<kernel_instance<unary_operation_t> > kernels;
            std::deque<intptr_t> element_sizes;

            // One link is a cast operation from src_dt to dst_dt.value_dtype()
            kernels.push_back(kernel_instance<unary_operation_t>());
            element_sizes.push_back(dst_dt.value_dtype().itemsize());
            get_dtype_assignment_kernel(dst_dt.value_dtype(), dst_dt.value_dtype().itemsize(),
                                src_dt, src_fixedstride, errmode, kernels.back());
            
            push_back_dtype_value_to_storage_kernels(dst_dt, dst_fixedstride, dst_dt.value_dtype().itemsize(),
                                kernels, element_sizes);

            make_chained_unary_kernel(kernels, element_sizes, out_kernel);
            return;
        }
    }

    stringstream ss;
    ss << "strided assignment from " << src_dt << " to " << dst_dt << " isn't yet supported";
    throw std::runtime_error(ss.str());
}