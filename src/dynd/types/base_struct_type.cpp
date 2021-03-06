//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/type.hpp>
#include <dynd/types/base_struct_type.hpp>
#include <dynd/types/struct_type.hpp>
#include <dynd/types/string_type.hpp>
#include <dynd/types/type_type.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/shortvector.hpp>
#include <dynd/shape_tools.hpp>
#include <dynd/ensure_immutable_contig.hpp>

using namespace std;
using namespace dynd;

ndt::base_struct_type::base_struct_type(type_id_t type_id, const nd::array &field_names, const nd::array &field_types,
                                        flags_type flags, bool layout_in_arrmeta, bool variadic)
    : base_tuple_type(type_id, field_types, flags, layout_in_arrmeta, variadic), m_field_names(field_names)
{
  /*
    if (!nd::ensure_immutable_contig<std::string>(m_field_names)) {
      stringstream ss;
      ss << "dynd struct field names requires an array of strings, got an "
            "array with type " << m_field_names.get_type();
      throw invalid_argument(ss.str());
    }
  */

  // Make sure that the number of names matches
  intptr_t name_count = reinterpret_cast<const fixed_dim_type_arrmeta *>(m_field_names.get()->metadata())->dim_size;
  if (name_count != m_field_count) {
    stringstream ss;
    ss << "dynd struct type requires that the number of names, " << name_count << " matches the number of types, "
       << m_field_count;
    throw invalid_argument(ss.str());
  }

  m_members.kind = variadic ? kind_kind : struct_kind;
}

ndt::base_struct_type::~base_struct_type()
{
}

intptr_t ndt::base_struct_type::get_field_index(const char *field_name_begin, const char *field_name_end) const
{
  size_t size = field_name_end - field_name_begin;
  if (size > 0) {
    char firstchar = *field_name_begin;
    intptr_t field_count = get_field_count();
    const char *fn_ptr = m_field_names.cdata();
    intptr_t fn_stride = reinterpret_cast<const fixed_dim_type_arrmeta *>(m_field_names.get()->metadata())->stride;
    for (intptr_t i = 0; i != field_count; ++i, fn_ptr += fn_stride) {
      const string *fn = reinterpret_cast<const string *>(fn_ptr);
      const char *begin = fn->begin(), *end = fn->end();
      if ((size_t)(end - begin) == size && *begin == firstchar) {
        if (memcmp(fn->begin(), field_name_begin, size) == 0) {
          return i;
        }
      }
    }
  }

  return -1;
}

ndt::type ndt::base_struct_type::apply_linear_index(intptr_t nindices, const irange *indices, size_t current_i,
                                                    const type &root_tp, bool leading_dimension) const
{
  if (nindices == 0) {
    return type(this, true);
  } else {
    bool remove_dimension;
    intptr_t start_index, index_stride, dimension_size;
    apply_single_linear_index(*indices, m_field_count, current_i, &root_tp, remove_dimension, start_index, index_stride,
                              dimension_size);
    if (remove_dimension) {
      return get_field_type(start_index)
          .apply_linear_index(nindices - 1, indices + 1, current_i + 1, root_tp, leading_dimension);
    } else if (nindices == 1 && start_index == 0 && index_stride == 1 && dimension_size == m_field_count) {
      // This is a do-nothing index, keep the same type
      return type(this, true);
    } else {
      // Take the subset of the fields in-place
      nd::array tmp_field_types(nd::empty(dimension_size, make_type()));
      type *tmp_field_types_raw = reinterpret_cast<type *>(tmp_field_types.data());

      // Make an "N * string" array without copying the actual
      // string text data. TODO: encapsulate this into a function.
      string *string_arr_ptr;
      type stp = string_type::make();
      type tp = make_fixed_dim(dimension_size, stp);
      nd::array tmp_field_names = nd::empty(tp);
      string_arr_ptr = reinterpret_cast<string *>(tmp_field_names.data());

      for (intptr_t i = 0; i < dimension_size; ++i) {
        intptr_t idx = start_index + i * index_stride;
        tmp_field_types_raw[i] =
            get_field_type(idx).apply_linear_index(nindices - 1, indices + 1, current_i + 1, root_tp, false);
        string_arr_ptr[i] = get_field_name_raw(idx);
      }

      tmp_field_types.flag_as_immutable();
      return struct_type::make(tmp_field_names, tmp_field_types);
    }
  }
}

intptr_t ndt::base_struct_type::apply_linear_index(intptr_t nindices, const irange *indices, const char *arrmeta,
                                                   const type &result_tp, char *out_arrmeta,
                                                   const intrusive_ptr<memory_block_data> &embedded_reference,
                                                   size_t current_i, const type &root_tp, bool leading_dimension,
                                                   char **inout_data,
                                                   intrusive_ptr<memory_block_data> &inout_dataref) const
{
  if (nindices == 0) {
    // If there are no more indices, copy the arrmeta verbatim
    arrmeta_copy_construct(out_arrmeta, arrmeta, embedded_reference);
    return 0;
  } else {
    const uintptr_t *offsets = get_data_offsets(arrmeta);
    const uintptr_t *arrmeta_offsets = get_arrmeta_offsets_raw();
    bool remove_dimension;
    intptr_t start_index, index_stride, dimension_size;
    apply_single_linear_index(*indices, m_field_count, current_i, &root_tp, remove_dimension, start_index, index_stride,
                              dimension_size);
    if (remove_dimension) {
      const type &dt = get_field_type(start_index);
      intptr_t offset = offsets[start_index];
      if (!dt.is_builtin()) {
        if (leading_dimension) {
          // In the case of a leading dimension, first bake the offset into
          // the data pointer, so that it's pointing at the right element
          // for the collapsing of leading dimensions to work correctly.
          *inout_data += offset;
          offset = dt.extended()->apply_linear_index(nindices - 1, indices + 1, arrmeta + arrmeta_offsets[start_index],
                                                     result_tp, out_arrmeta, embedded_reference, current_i + 1, root_tp,
                                                     true, inout_data, inout_dataref);
        } else {
          intrusive_ptr<memory_block_data> tmp;
          offset += dt.extended()->apply_linear_index(nindices - 1, indices + 1, arrmeta + arrmeta_offsets[start_index],
                                                      result_tp, out_arrmeta, embedded_reference, current_i + 1,
                                                      root_tp, false, NULL, tmp);
        }
      }
      return offset;
    } else {
      intrusive_ptr<memory_block_data> tmp;
      intptr_t *out_offsets = reinterpret_cast<intptr_t *>(out_arrmeta);
      const struct_type *result_e_dt = result_tp.extended<struct_type>();
      for (intptr_t i = 0; i < dimension_size; ++i) {
        intptr_t idx = start_index + i * index_stride;
        out_offsets[i] = offsets[idx];
        const type &dt = result_e_dt->get_field_type(i);
        if (!dt.is_builtin()) {
          out_offsets[i] +=
              dt.extended()->apply_linear_index(nindices - 1, indices + 1, arrmeta + arrmeta_offsets[idx], dt,
                                                out_arrmeta + result_e_dt->get_arrmeta_offset(i), embedded_reference,
                                                current_i + 1, root_tp, false, NULL, tmp);
        }
      }
      return 0;
    }
  }
}

bool ndt::base_struct_type::match(const char *arrmeta, const type &candidate_tp, const char *candidate_arrmeta,
                                  std::map<std::string, type> &tp_vars) const
{
  intptr_t candidate_field_count = candidate_tp.extended<base_struct_type>()->get_field_count();
  bool candidate_variadic = candidate_tp.extended<base_tuple_type>()->is_variadic();

  if ((m_field_count == candidate_field_count && !candidate_variadic) ||
      ((candidate_field_count >= m_field_count) && m_variadic)) {
    // Compare the field names
    if (m_field_count == candidate_field_count) {
      if (!get_field_names().equals_exact(candidate_tp.extended<base_struct_type>()->get_field_names())) {
        return false;
      }
    } else {
      nd::array leading_field_names = get_field_names();
      if (!leading_field_names.equals_exact(
               candidate_tp.extended<base_struct_type>()->get_field_names()(irange() < m_field_count))) {
        return false;
      }
    }

    const type *fields = get_field_types_raw();
    const type *candidate_fields = candidate_tp.extended<base_struct_type>()->get_field_types_raw();
    for (intptr_t i = 0; i < m_field_count; ++i) {
      if (!fields[i].match(arrmeta, candidate_fields[i], candidate_arrmeta, tp_vars)) {
        return false;
      }
    }
    return true;
  }

  return false;
}

size_t ndt::base_struct_type::get_elwise_property_index(const std::string &property_name) const
{
  intptr_t i = get_field_index(property_name);
  if (i >= 0) {
    return i;
  } else {
    stringstream ss;
    ss << "dynd type " << type(this, true) << " does not have a kernel for property " << property_name;
    throw runtime_error(ss.str());
  }
}

ndt::type ndt::base_struct_type::get_elwise_property_type(size_t elwise_property_index, bool &out_readable,
                                                          bool &out_writable) const
{
  size_t field_count = get_field_count();
  if (elwise_property_index < field_count) {
    out_readable = true;
    out_writable = false;
    return get_field_type(elwise_property_index).value_type();
  } else {
    return type::make<void>();
  }
}

namespace {
struct struct_property_getter_ck : nd::base_kernel<struct_property_getter_ck, 1> {
  size_t m_field_offset;

  ~struct_property_getter_ck()
  {
    get_child()->destroy();
  }

  void single(char *dst, char *const *src)
  {
    ckernel_prefix *child = get_child();
    expr_single_t child_fn = child->get_function<expr_single_t>();
    char *src_copy = src[0] + m_field_offset;
    child_fn(child, dst, &src_copy);
  }

  void strided(char *dst, intptr_t dst_stride, char *const *src, const intptr_t *src_stride, size_t count)
  {
    ckernel_prefix *child = get_child();
    expr_strided_t child_fn = child->get_function<expr_strided_t>();
    char *src_copy = src[0] + m_field_offset;
    child_fn(child, dst, dst_stride, &src_copy, src_stride, count);
  }
};
} // anonymous namespace

size_t ndt::base_struct_type::make_elwise_property_getter_kernel(void *ckb, intptr_t ckb_offset,
                                                                 const char *dst_arrmeta, const char *src_arrmeta,
                                                                 size_t src_elwise_property_index,
                                                                 kernel_request_t kernreq,
                                                                 const eval::eval_context *ectx) const
{
  typedef struct_property_getter_ck self_type;
  size_t field_count = get_field_count();
  if (src_elwise_property_index < field_count) {
    const uintptr_t *arrmeta_offsets = get_arrmeta_offsets_raw();
    const type &field_type = get_field_type(src_elwise_property_index);
    self_type *self = self_type::make(ckb, kernreq, ckb_offset);
    self->m_field_offset = get_data_offsets(src_arrmeta)[src_elwise_property_index];
    return ::make_assignment_kernel(ckb, ckb_offset, field_type.value_type(), dst_arrmeta, field_type,
                                    src_arrmeta + arrmeta_offsets[src_elwise_property_index], kernreq, ectx);
  } else {
    stringstream ss;
    ss << "dynd type " << type(this, true);
    ss << " given an invalid property index" << src_elwise_property_index;
    throw runtime_error(ss.str());
  }
}

size_t ndt::base_struct_type::make_elwise_property_setter_kernel(
    void *DYND_UNUSED(ckb), intptr_t DYND_UNUSED(ckb_offset), const char *DYND_UNUSED(dst_arrmeta),
    size_t dst_elwise_property_index, const char *DYND_UNUSED(src_arrmeta), kernel_request_t DYND_UNUSED(kernreq),
    const eval::eval_context *DYND_UNUSED(ectx)) const
{
  // No writable properties
  stringstream ss;
  ss << "dynd type " << type(this, true);
  ss << " given an invalid property index" << dst_elwise_property_index;
  throw runtime_error(ss.str());
}
