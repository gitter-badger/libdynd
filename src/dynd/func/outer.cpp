//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/arrmeta_holder.hpp>
#include <dynd/func/elwise.hpp>
#include <dynd/func/outer.hpp>

using namespace std;
using namespace dynd;

nd::callable nd::functional::outer(const callable &child)
{
  return callable::make<outer_ck>(outer_make_type(child.get_type()), child, 0);
}

ndt::type nd::functional::outer_make_type(const ndt::callable_type *child_tp)
{
  const ndt::type *param_types = child_tp->get_pos_types_raw();
  intptr_t param_count = child_tp->get_npos();
  dynd::nd::array out_param_types = dynd::nd::empty(param_count, ndt::make_type());
  ndt::type *pt = reinterpret_cast<ndt::type *>(out_param_types.data());

  for (intptr_t i = 0, i_end = child_tp->get_npos(); i != i_end; ++i) {
    std::string dimsname("Dims" + std::to_string(i));
    if (param_types[i].get_kind() == memory_kind) {
      pt[i] = pt[i].extended<ndt::base_memory_type>()->with_replaced_storage_type(
          ndt::make_ellipsis_dim(dimsname, param_types[i].without_memory_type()));
    } else if (param_types[i].get_type_id() == typevar_constructed_type_id) {
      pt[i] = ndt::typevar_constructed_type::make(
          param_types[i].extended<ndt::typevar_constructed_type>()->get_name(),
          ndt::make_ellipsis_dim(dimsname, param_types[i].extended<ndt::typevar_constructed_type>()->get_arg()));
    } else {
      pt[i] = ndt::make_ellipsis_dim(dimsname, param_types[i]);
    }
  }

  ndt::type kwd_tp = child_tp->get_kwd_struct();

  ndt::type ret_tp = child_tp->get_return_type();
  if (ret_tp.get_kind() == memory_kind) {
    throw std::runtime_error("outer -- need to fix this");
  } else if (ret_tp.get_type_id() == typevar_constructed_type_id) {
    ret_tp = ndt::typevar_constructed_type::make(
        ret_tp.extended<ndt::typevar_constructed_type>()->get_name(),
        ndt::make_ellipsis_dim("Dims", ret_tp.extended<ndt::typevar_constructed_type>()->get_arg()));
  } else {
    ret_tp = ndt::make_ellipsis_dim("Dims", child_tp->get_return_type());
  }

  return ndt::callable_type::make(ret_tp, ndt::tuple_type::make(out_param_types), kwd_tp);
}
