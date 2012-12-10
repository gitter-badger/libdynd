//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <algorithm>

#include <dynd/dtypes/date_dtype.hpp>
#include <dynd/dtypes/date_property_dtype.hpp>
#include <dynd/dtypes/fixedstruct_dtype.hpp>
#include <dynd/kernels/single_compare_kernel_instance.hpp>
#include <dynd/kernels/string_assignment_kernels.hpp>
#include <dynd/exceptions.hpp>
#include <dynd/gfunc/make_callable.hpp>
#include <dynd/kernels/date_assignment_kernels.hpp>

#include <datetime_strings.h>

using namespace std;
using namespace dynd;

date_dtype::date_dtype(date_unit_t unit)
    : m_unit(unit)
{
    switch (unit) {
        case date_unit_year:
        case date_unit_month:
        case date_unit_day:
            break;
        default:
            throw runtime_error("Unrecognized date unit in date dtype constructor");
    }
}

void date_dtype::print_element(std::ostream& o, const char *DYND_UNUSED(metadata), const char *data) const
{
    int32_t value = *reinterpret_cast<const int32_t *>(data);
    switch (m_unit) {
        case date_unit_year:
            o << datetime::make_iso_8601_date(value, datetime::datetime_unit_year);
            break;
        case date_unit_month:
            o << datetime::make_iso_8601_date(value, datetime::datetime_unit_month);
            break;
        case date_unit_day:
            o << datetime::make_iso_8601_date(value, datetime::datetime_unit_day);
            break;
        default:
            o << "<corrupt date dtype>";
            break;
    }
}

void date_dtype::print_dtype(std::ostream& o) const
{
    if (m_unit == date_unit_day) {
        o << "date";
    } else {
        o << "date<" << m_unit << ">";
    }
}

bool date_dtype::is_lossless_assignment(const dtype& dst_dt, const dtype& src_dt) const
{
    if (dst_dt.extended() == this) {
        if (src_dt.extended() == this) {
            return true;
        } else if (src_dt.get_type_id() == date_type_id) {
            const date_dtype *src_fs = static_cast<const date_dtype*>(src_dt.extended());
            return src_fs->m_unit == m_unit;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void date_dtype::get_single_compare_kernel(single_compare_kernel_instance& /*out_kernel*/) const {
    throw runtime_error("get_single_compare_kernel for date are not implemented");
}

void date_dtype::get_dtype_assignment_kernel(const dtype& dst_dt, const dtype& src_dt,
                assign_error_mode errmode,
                kernel_instance<unary_operation_pair_t>& out_kernel) const
{
    if (this == dst_dt.extended()) {
        if (src_dt.get_kind() == string_kind) {
            // Assignment from strings
            get_string_to_date_assignment_kernel(m_unit, src_dt, errmode, out_kernel);
            return;
        }
        if (src_dt.extended()) {
            src_dt.extended()->get_dtype_assignment_kernel(dst_dt, src_dt, errmode, out_kernel);
        }
        // TODO
    } else {
        if (dst_dt.get_kind() == string_kind) {
            // Assignment to strings
            get_date_to_string_assignment_kernel(dst_dt, m_unit, errmode, out_kernel);
            return;
        } else if (dst_dt.get_kind() == struct_kind) {
            get_date_to_struct_assignment_kernel(dst_dt, m_unit, errmode, out_kernel);
            return;
        }
        // TODO
    }

    stringstream ss;
    ss << "assignment from " << src_dt << " to " << dst_dt << " is not implemented yet";
    throw runtime_error(ss.str());
}


bool date_dtype::operator==(const extended_dtype& rhs) const
{
    if (this == &rhs) {
        return true;
    } else if (rhs.get_type_id() != date_type_id) {
        return false;
    } else {
        const date_dtype *dt = static_cast<const date_dtype*>(&rhs);
        return m_unit == dt->m_unit;
    }
}

///////// properties on the dtype

static string property_get_encoding(const dtype& dt) {
    const date_dtype *d = static_cast<const date_dtype *>(dt.extended());
    stringstream ss;
    ss << d->get_unit();
    return ss.str();
}

static pair<string, gfunc::callable> date_dtype_properties[] = {
    pair<string, gfunc::callable>("unit", gfunc::make_callable(&property_get_encoding, "self"))
};

void date_dtype::get_dynamic_dtype_properties(const std::pair<std::string, gfunc::callable> **out_properties, int *out_count) const
{
    *out_properties = date_dtype_properties;
    *out_count = sizeof(date_dtype_properties) / sizeof(date_dtype_properties[0]);
}

///////// properties on the ndobject

static ndobject property_ndo_get_year(const ndobject& n) {
    dtype array_dt = n.get_dtype();
    dtype dt = array_dt.get_dtype_at_dimension(NULL, array_dt.get_uniform_ndim());
    return n.view_scalars(make_date_property_dtype(dt, "year"));
}

static ndobject property_ndo_get_month(const ndobject& n) {
    dtype array_dt = n.get_dtype();
    dtype dt = array_dt.get_dtype_at_dimension(NULL, array_dt.get_uniform_ndim());
    return n.view_scalars(make_date_property_dtype(dt, "month"));
}

static ndobject property_ndo_get_day(const ndobject& n) {
    dtype array_dt = n.get_dtype();
    dtype dt = array_dt.get_dtype_at_dimension(NULL, array_dt.get_uniform_ndim());
    return n.view_scalars(make_date_property_dtype(dt, "day"));
}

static pair<string, gfunc::callable> date_ndobject_properties[] = {
    pair<string, gfunc::callable>("year", gfunc::make_callable(&property_ndo_get_year, "self")),
    pair<string, gfunc::callable>("month", gfunc::make_callable(&property_ndo_get_month, "self")),
    pair<string, gfunc::callable>("day", gfunc::make_callable(&property_ndo_get_day, "self"))
};

void date_dtype::get_dynamic_ndobject_properties(const std::pair<std::string, gfunc::callable> **out_properties, int *out_count) const
{
    *out_properties = date_ndobject_properties;
    *out_count = sizeof(date_ndobject_properties) / sizeof(date_ndobject_properties[0]);
    // Exclude the day and/or month properties for the larger units
    if (m_unit == date_unit_month) {
        *out_count -= 1;
    } else if (m_unit == date_unit_year) {
        *out_count -= 2;
    }
}

///////// property accessor kernels (used by date_property_dtype)

namespace {
    void property_kernel_year_single(char *dst, const char *src, unary_kernel_static_data *extra)
    {
        datetime::datetime_unit_t unit = static_cast<datetime::datetime_unit_t>(get_raw_auxiliary_data(extra->auxdata)>>1);
        datetime::datetime_fields fld;
        fld.set_from_date_val(*reinterpret_cast<const int32_t *>(src), unit);
        *reinterpret_cast<int32_t *>(dst) = fld.year;
    }
    void property_kernel_year_contig(char *dst, const char *src, size_t count, unary_kernel_static_data *extra)
    {
        datetime::datetime_unit_t unit = static_cast<datetime::datetime_unit_t>(get_raw_auxiliary_data(extra->auxdata)>>1);
        datetime::datetime_fields fld;
        int32_t *dst_array = reinterpret_cast<int32_t *>(dst);
        const int32_t *src_array = reinterpret_cast<const int32_t *>(src);
        for (size_t i = 0; i != count; ++i, ++src_array, ++dst_array) {
            fld.set_from_date_val(*src_array, unit);
            *dst_array = fld.year;
        }
    }

    void property_kernel_month_single(char *dst, const char *src, unary_kernel_static_data *extra)
    {
        datetime::datetime_unit_t unit = static_cast<datetime::datetime_unit_t>(get_raw_auxiliary_data(extra->auxdata)>>1);
        datetime::datetime_fields fld;
        fld.set_from_date_val(*reinterpret_cast<const int32_t *>(src), unit);
        *reinterpret_cast<int32_t *>(dst) = fld.month;
    }
    void property_kernel_month_contig(char *dst, const char *src, size_t count, unary_kernel_static_data *extra)
    {
        datetime::datetime_unit_t unit = static_cast<datetime::datetime_unit_t>(get_raw_auxiliary_data(extra->auxdata)>>1);
        datetime::datetime_fields fld;
        int32_t *dst_array = reinterpret_cast<int32_t *>(dst);
        const int32_t *src_array = reinterpret_cast<const int32_t *>(src);
        for (size_t i = 0; i != count; ++i, ++src_array, ++dst_array) {
            fld.set_from_date_val(*src_array, unit);
            *dst_array = fld.month;
        }
    }

    void property_kernel_day_single(char *dst, const char *src, unary_kernel_static_data *extra)
    {
        datetime::datetime_unit_t unit = static_cast<datetime::datetime_unit_t>(get_raw_auxiliary_data(extra->auxdata)>>1);
        datetime::datetime_fields fld;
        fld.set_from_date_val(*reinterpret_cast<const int32_t *>(src), unit);
        *reinterpret_cast<int32_t *>(dst) = fld.day;
    }
    void property_kernel_day_contig(char *dst, const char *src, size_t count, unary_kernel_static_data *extra)
    {
        datetime::datetime_unit_t unit = static_cast<datetime::datetime_unit_t>(get_raw_auxiliary_data(extra->auxdata)>>1);
        datetime::datetime_fields fld;
        int32_t *dst_array = reinterpret_cast<int32_t *>(dst);
        const int32_t *src_array = reinterpret_cast<const int32_t *>(src);
        for (size_t i = 0; i != count; ++i, ++src_array, ++dst_array) {
            fld.set_from_date_val(*src_array, unit);
            *dst_array = fld.day;
        }
    }
} // anonymous namespace

void date_dtype::get_property_getter_kernel(const std::string& property_name,
                dtype& out_value_dtype, kernel_instance<unary_operation_pair_t>& out_to_value_kernel) const
{
    datetime::datetime_unit_t unit = m_unit == date_unit_day ? datetime::datetime_unit_day :
                                    m_unit == date_unit_month ? datetime::datetime_unit_month :
                                    datetime::datetime_unit_year;

    out_value_dtype = make_dtype<int32_t>();
    make_raw_auxiliary_data(out_to_value_kernel.auxdata, static_cast<uintptr_t>(unit)<<1);
    if (property_name == "year") {
        out_to_value_kernel.kernel.single = &property_kernel_year_single;
        out_to_value_kernel.kernel.contig = &property_kernel_year_contig;
    } else if (m_unit <= date_unit_month && property_name == "month") {
        out_to_value_kernel.kernel.single = &property_kernel_month_single;
        out_to_value_kernel.kernel.contig = &property_kernel_month_contig;
    } else if (m_unit <= date_unit_day && property_name == "day") {
        out_to_value_kernel.kernel.single = &property_kernel_day_single;
        out_to_value_kernel.kernel.contig = &property_kernel_day_contig;
    } else {
        stringstream ss;
        ss << "dynd date dtype does not have a kernel for property " << property_name;
        throw runtime_error(ss.str());
    }
}