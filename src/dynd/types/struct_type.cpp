//
// Copyright (C) 2011-14 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/types/struct_type.hpp>
#include <dynd/types/type_alignment.hpp>
#include <dynd/types/property_type.hpp>
#include <dynd/shape_tools.hpp>
#include <dynd/exceptions.hpp>
#include <dynd/func/make_callable.hpp>
#include <dynd/kernels/tuple_assignment_kernels.hpp>
#include <dynd/kernels/struct_assignment_kernels.hpp>
#include <dynd/kernels/tuple_comparison_kernels.hpp>

using namespace std;
using namespace dynd;

struct_type::~struct_type() {}

static bool is_simple_identifier_name(const char *begin, const char *end)
{
  if (begin == end) {
    return false;
  }
  else {
    char c = *begin++;
    if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_')) {
      return false;
    }
    while (begin < end) {
      c = *begin++;
      if (!(('0' <= c && c <= '9') || ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') || c == '_')) {
        return false;
      }
    }
    return true;
  }
}

void struct_type::print_type(std::ostream &o) const
{
  // Use the record datashape syntax
  o << "{";
  for (intptr_t i = 0, i_end = m_field_count; i != i_end; ++i) {
    if (i != 0) {
      o << ", ";
    }
    const string_type_data &fn = get_field_name_raw(i);
    if (is_simple_identifier_name(fn.begin, fn.end)) {
      o.write(fn.begin, fn.end - fn.begin);
    }
    else {
      print_escaped_utf8_string(o, fn.begin, fn.end, true);
    }
    o << " : " << get_field_type(i);
  }
  o << "}";
}

void struct_type::transform_child_types(type_transform_fn_t transform_fn,
                                        intptr_t arrmeta_offset, void *extra,
                                        ndt::type &out_transformed_tp,
                                        bool &out_was_transformed) const
{
  nd::array tmp_field_types(nd::empty(m_field_count, ndt::make_type()));
  ndt::type *tmp_field_types_raw =
      reinterpret_cast<ndt::type *>(tmp_field_types.get_readwrite_originptr());

  bool was_transformed = false;
  for (intptr_t i = 0, i_end = m_field_count; i != i_end; ++i) {
    transform_fn(get_field_type(i), arrmeta_offset + get_arrmeta_offset(i),
                 extra, tmp_field_types_raw[i], was_transformed);
  }
  if (was_transformed) {
    tmp_field_types.flag_as_immutable();
    out_transformed_tp = ndt::make_struct(m_field_names, tmp_field_types);
    out_was_transformed = true;
  }
  else {
    out_transformed_tp = ndt::type(this, true);
  }
}

ndt::type struct_type::get_canonical_type() const
{
  nd::array tmp_field_types(nd::empty(m_field_count, ndt::make_type()));
  ndt::type *tmp_field_types_raw =
      reinterpret_cast<ndt::type *>(tmp_field_types.get_readwrite_originptr());

  for (intptr_t i = 0, i_end = m_field_count; i != i_end; ++i) {
    tmp_field_types_raw[i] = get_field_type(i).get_canonical_type();
  }

  tmp_field_types.flag_as_immutable();
  return ndt::make_struct(m_field_names, tmp_field_types);
}

bool struct_type::is_lossless_assignment(const ndt::type &dst_tp,
                                         const ndt::type &src_tp) const
{
  if (dst_tp.extended() == this) {
    if (src_tp.extended() == this) {
      return true;
    }
    else if (src_tp.get_type_id() == struct_type_id) {
      return *dst_tp.extended() == *src_tp.extended();
    }
  }

  return false;
}

intptr_t struct_type::make_assignment_kernel(
    const arrfunc_type_data *self, const arrfunc_type *af_tp, void *ckb,
    intptr_t ckb_offset, const ndt::type &dst_tp, const char *dst_arrmeta,
    const ndt::type &src_tp, const char *src_arrmeta, kernel_request_t kernreq,
    const eval::eval_context *ectx, const nd::array &kwds) const
{
  if (this == dst_tp.extended()) {
    if (this == src_tp.extended()) {
      return make_tuple_identical_assignment_kernel(
          ckb, ckb_offset, dst_tp, dst_arrmeta, src_arrmeta, kernreq, ectx);
    }
    else if (src_tp.get_kind() == struct_kind) {
      return make_struct_assignment_kernel(ckb, ckb_offset, dst_tp, dst_arrmeta,
                                           src_tp, src_arrmeta, kernreq, ectx);
    }
    else if (src_tp.is_builtin()) {
      return make_broadcast_to_tuple_assignment_kernel(
          ckb, ckb_offset, dst_tp, dst_arrmeta, src_tp, src_arrmeta, kernreq,
          ectx);
    }
    else {
      return src_tp.extended()->make_assignment_kernel(
          self, af_tp, ckb, ckb_offset, dst_tp, dst_arrmeta, src_tp,
          src_arrmeta, kernreq, ectx, kwds);
    }
  }

  stringstream ss;
  ss << "Cannot assign from " << src_tp << " to " << dst_tp;
  throw dynd::type_error(ss.str());
}

size_t struct_type::make_comparison_kernel(void *ckb, intptr_t ckb_offset,
                                           const ndt::type &src0_dt,
                                           const char *src0_arrmeta,
                                           const ndt::type &src1_dt,
                                           const char *src1_arrmeta,
                                           comparison_type_t comptype,
                                           const eval::eval_context *ectx) const
{
  if (this == src0_dt.extended()) {
    if (*this == *src1_dt.extended()) {
      return make_tuple_comparison_kernel(
          ckb, ckb_offset, src0_dt, src0_arrmeta, src1_arrmeta, comptype, ectx);
    }
    else if (src1_dt.get_kind() == struct_kind) {
      // TODO
    }
  }

  throw not_comparable_error(src0_dt, src1_dt, comptype);
}

bool struct_type::operator==(const base_type &rhs) const
{
  if (this == &rhs) {
    return true;
  }
  else if (rhs.get_type_id() != struct_type_id) {
    return false;
  }
  else {
    const struct_type *dt = static_cast<const struct_type *>(&rhs);
    return get_data_alignment() == dt->get_data_alignment() &&
           m_field_types.equals_exact(dt->m_field_types) &&
           m_field_names.equals_exact(dt->m_field_names);
  }
}

void struct_type::arrmeta_debug_print(const char *arrmeta, std::ostream &o,
                                      const std::string &indent) const
{
  const size_t *offsets = reinterpret_cast<const size_t *>(arrmeta);
  o << indent << "struct arrmeta\n";
  o << indent << " field offsets: ";
  for (intptr_t i = 0, i_end = m_field_count; i != i_end; ++i) {
    o << offsets[i];
    if (i != i_end - 1) {
      o << ", ";
    }
  }
  o << "\n";
  const uintptr_t *arrmeta_offsets = get_arrmeta_offsets_raw();
  for (intptr_t i = 0; i < m_field_count; ++i) {
    const ndt::type &field_dt = get_field_type(i);
    if (!field_dt.is_builtin() && field_dt.extended()->get_arrmeta_size() > 0) {
      o << indent << " field " << i << " (name ";
      const string_type_data &fnr = get_field_name_raw(i);
      o.write(fnr.begin, fnr.end - fnr.begin);
      o << ") arrmeta:\n";
      field_dt.extended()->arrmeta_debug_print(arrmeta + arrmeta_offsets[i], o,
                                               indent + "  ");
    }
  }
}

static nd::array property_get_field_names(const ndt::type &tp)
{
  return tp.extended<struct_type>()->get_field_names();
}

static nd::array property_get_field_types(const ndt::type &tp)
{
  return tp.extended<struct_type>()->get_field_types();
}

static nd::array property_get_arrmeta_offsets(const ndt::type &tp)
{
  return tp.extended<struct_type>()->get_arrmeta_offsets();
}

void struct_type::get_dynamic_type_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  static pair<string, gfunc::callable> type_properties[] = {
      pair<string, gfunc::callable>(
          "field_names",
          gfunc::make_callable(&property_get_field_names, "self")),
      pair<string, gfunc::callable>(
          "field_types",
          gfunc::make_callable(&property_get_field_types, "self")),
      pair<string, gfunc::callable>(
          "arrmeta_offsets",
          gfunc::make_callable(&property_get_arrmeta_offsets, "self"))};

  *out_properties = type_properties;
  *out_count = sizeof(type_properties) / sizeof(type_properties[0]);
}

static array_preamble *property_get_array_field(const array_preamble *params,
                                                void *extra)
{
  // Get the nd::array 'self' parameter
  nd::array n = nd::array(*(array_preamble **)params->m_data_pointer, true);
  intptr_t i = reinterpret_cast<intptr_t>(extra);
  intptr_t undim = n.get_ndim();
  ndt::type udt = n.get_dtype();
  if (udt.get_kind() == expr_kind) {
    string field_name =
        udt.value_type().extended<struct_type>()->get_field_name(i);
    return n.replace_dtype(ndt::make_property(udt, field_name, i)).release();
  }
  else {
    if (undim == 0) {
      return n(i).release();
    }
    else {
      shortvector<irange> idx(undim + 1);
      idx[undim] = irange(i);
      return n.at_array(undim + 1, idx.get()).release();
    }
  }
}

void struct_type::create_array_properties()
{
  ndt::type array_parameters_type =
      ndt::make_cstruct(ndt::make_ndarrayarg(), "self");

  m_array_properties.resize(m_field_count);
  for (intptr_t i = 0, i_end = m_field_count; i != i_end; ++i) {
    // TODO: Transform the name into a valid Python symbol?
    m_array_properties[i].first = get_field_name(i);
    m_array_properties[i].second.set(array_parameters_type,
                                     &property_get_array_field, (void *)i);
  }
}

void struct_type::get_dynamic_array_properties(
    const std::pair<std::string, gfunc::callable> **out_properties,
    size_t *out_count) const
{
  *out_properties = m_array_properties.empty() ? NULL : &m_array_properties[0];
  *out_count = (int)m_array_properties.size();
}

nd::array dynd::struct_concat(nd::array lhs, nd::array rhs)
{
  nd::array res;
  if (lhs.is_null()) {
    res = rhs;
    return res;
  }
  if (rhs.is_null()) {
    res = lhs;
    return res;
  }
  const ndt::type &lhs_tp = lhs.get_type(), &rhs_tp = rhs.get_type();
  if (lhs_tp.get_kind() != struct_kind) {
    stringstream ss;
    ss << "Cannot concatenate array with type " << lhs_tp << " as a struct";
    throw invalid_argument(ss.str());
  }
  if (rhs_tp.get_kind() != struct_kind) {
    stringstream ss;
    ss << "Cannot concatenate array with type " << rhs_tp << " as a struct";
    throw invalid_argument(ss.str());
  }

  // Make an empty shell struct by concatenating the fields together
  intptr_t lhs_n = lhs_tp.extended<base_struct_type>()->get_field_count();
  intptr_t rhs_n = rhs_tp.extended<base_struct_type>()->get_field_count();
  intptr_t res_n = lhs_n + rhs_n;
  nd::array res_field_names = nd::empty(res_n, ndt::make_string());
  nd::array res_field_types = nd::empty(res_n, ndt::make_type());
  res_field_names(irange(0, lhs_n)).vals() =
      lhs_tp.extended<base_struct_type>()->get_field_names();
  res_field_names(irange(lhs_n, res_n)).vals() =
      rhs_tp.extended<base_struct_type>()->get_field_names();
  res_field_types(irange(0, lhs_n)).vals() =
      lhs_tp.extended<base_struct_type>()->get_field_types();
  res_field_types(irange(lhs_n, res_n)).vals() =
      rhs_tp.extended<base_struct_type>()->get_field_types();
  ndt::type res_tp = ndt::make_struct(res_field_names, res_field_types);
  const ndt::type *res_field_tps =
      res_tp.extended<base_struct_type>()->get_field_types_raw();
  res = nd::empty_shell(res_tp);

  // Initialize the default data offsets for the struct arrmeta
  struct_type::fill_default_data_offsets(
      res_n, res_tp.extended<base_struct_type>()->get_field_types_raw(),
      reinterpret_cast<uintptr_t *>(res.get_arrmeta()));
  // Get information about the arrmeta layout of the input and res
  const uintptr_t *lhs_arrmeta_offsets =
      lhs_tp.extended<base_struct_type>()->get_arrmeta_offsets_raw();
  const uintptr_t *rhs_arrmeta_offsets =
      rhs_tp.extended<base_struct_type>()->get_arrmeta_offsets_raw();
  const uintptr_t *res_arrmeta_offsets =
      res_tp.extended<base_struct_type>()->get_arrmeta_offsets_raw();
  const char *lhs_arrmeta = lhs.get_arrmeta();
  const char *rhs_arrmeta = rhs.get_arrmeta();
  char *res_arrmeta = res.get_arrmeta();
  // Copy the arrmeta from the input arrays
  for (intptr_t i = 0; i < lhs_n; ++i) {
    const ndt::type &tp = res_field_tps[i];
    if (!tp.is_builtin()) {
      tp.extended()->arrmeta_copy_construct(
          res_arrmeta + res_arrmeta_offsets[i],
          lhs_arrmeta + lhs_arrmeta_offsets[i], lhs.get_data_memblock().get());
    }
  }
  for (intptr_t i = 0; i < rhs_n; ++i) {
    const ndt::type &tp = res_field_tps[i + lhs_n];
    if (!tp.is_builtin()) {
      tp.extended()->arrmeta_copy_construct(
          res_arrmeta + res_arrmeta_offsets[i + lhs_n],
          rhs_arrmeta + rhs_arrmeta_offsets[i], rhs.get_data_memblock().get());
    }
  }

  // Get information about the data layout of the input and res
  const uintptr_t *lhs_data_offsets =
      lhs_tp.extended<base_struct_type>()->get_data_offsets(lhs.get_arrmeta());
  const uintptr_t *rhs_data_offsets =
      rhs_tp.extended<base_struct_type>()->get_data_offsets(rhs.get_arrmeta());
  const uintptr_t *res_data_offsets =
      res_tp.extended<base_struct_type>()->get_data_offsets(res.get_arrmeta());
  const char *lhs_data = lhs.get_readonly_originptr();
  const char *rhs_data = rhs.get_readonly_originptr();
  char *res_data = res.get_readwrite_originptr();
  // Copy the data from the input arrays
  for (intptr_t i = 0; i < lhs_n; ++i) {
    const ndt::type &tp = res_field_tps[i];
    typed_data_copy(tp, res_arrmeta + res_arrmeta_offsets[i],
                    res_data + res_data_offsets[i],
                    lhs_arrmeta + lhs_arrmeta_offsets[i],
                    lhs_data + lhs_data_offsets[i]);
  }

  for (intptr_t i = 0; i < rhs_n; ++i) {
    const ndt::type &tp = res_field_tps[i + lhs_n];
    typed_data_copy(tp, res_arrmeta + res_arrmeta_offsets[i + lhs_n],
                    res_data + res_data_offsets[i + lhs_n],
                    rhs_arrmeta + rhs_arrmeta_offsets[i],
                    rhs_data + rhs_data_offsets[i]);
  }

  return res;
}
