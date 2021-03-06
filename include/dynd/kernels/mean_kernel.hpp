//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/func/arithmetic.hpp>
#include <dynd/func/sum.hpp>
#include <dynd/kernels/base_kernel.hpp>
#include <dynd/types/any_kind_type.hpp>

namespace dynd {
namespace nd {

  // All methods are inlined, so this does not need to be declared DYND_API.
  struct mean_kernel : base_kernel<mean_kernel, 1> {
    std::intptr_t compound_div_offset;
    int64 count;

    mean_kernel(int64 count) : count(count)
    {
    }

    void single(char *dst, char *const *src)
    {
      ckernel_prefix *sum_kernel = get_child();
      expr_single_t sum = sum_kernel->get_function<expr_single_t>();
      sum(sum_kernel, dst, src);

      ckernel_prefix *compound_div_kernel = get_child(compound_div_offset);
      expr_single_t compound_div =
          compound_div_kernel->get_function<expr_single_t>();
      char *child_src[1] = {reinterpret_cast<char *>(&count)};
      compound_div(compound_div_kernel, dst, child_src);
    }

    static void data_init(char *static_data, size_t DYND_UNUSED(data_size),
                          char *data, const ndt::type &dst_tp, intptr_t nsrc,
                          const ndt::type *src_tp, intptr_t nkwd,
                          const nd::array *kwds,
                          const std::map<std::string, ndt::type> &tp_vars)
    {
      std::size_t sum_data_size = nd::sum::get().get()->data_size;
      if (sum_data_size > 0) {
        nd::sum::get().get()->data_init(nd::sum::get().get()->static_data,
                                        sum_data_size, data, dst_tp, nsrc,
                                        src_tp, nkwd, kwds, tp_vars);
      }

      std::size_t compound_div_data_size =
          nd::compound_div::get().get()->data_size;
      if (compound_div_data_size > 0) {
        nd::compound_div::get().get()->data_init(
            nd::compound_div::get().get()->static_data, compound_div_data_size,
            data + compound_div_data_size, dst_tp, 1,
            reinterpret_cast<ndt::type *>(static_data), 0, NULL, tp_vars);
      }
    }

    static void
    resolve_dst_type(char *DYND_UNUSED(static_data),
                     std::size_t DYND_UNUSED(data_size), char *data,
                     ndt::type &dst_tp, intptr_t nsrc, const ndt::type *src_tp,
                     intptr_t nkwd, const array *kwds,
                     const std::map<std::string, ndt::type> &tp_vars)
    {
      nd::sum::get().get()->resolve_dst_type(
          nd::sum::get().get()->static_data, nd::sum::get().get()->data_size,
          data, dst_tp, nsrc, src_tp, nkwd, kwds, tp_vars);
    }

    static intptr_t instantiate(
        char *static_data, size_t data_size, char *data, void *ckb,
        intptr_t ckb_offset, const ndt::type &dst_tp, const char *dst_arrmeta,
        intptr_t nsrc, const ndt::type *src_tp, const char *const *src_arrmeta,
        kernel_request_t kernreq, const eval::eval_context *ectx, intptr_t nkwd,
        const array *kwds, const std::map<std::string, ndt::type> &tp_vars)
    {
      intptr_t mean_offset = ckb_offset;
      make(ckb, kernreq, ckb_offset, src_tp[0].get_size(src_arrmeta[0]));

      ckb_offset = nd::sum::get().get()->instantiate(
          nd::sum::get().get()->static_data, data_size, data, ckb, ckb_offset,
          dst_tp, dst_arrmeta, nsrc, src_tp, src_arrmeta, kernreq, ectx, nkwd,
          kwds, tp_vars);

      mean_kernel *self = get_self(
          reinterpret_cast<ckernel_builder<kernel_request_host> *>(ckb),
          mean_offset);
      self->compound_div_offset = ckb_offset;
      ckb_offset = nd::compound_div::get().get()->instantiate(
          nd::compound_div::get().get()->static_data, data_size, data, ckb,
          ckb_offset, dst_tp, dst_arrmeta, 1,
          reinterpret_cast<ndt::type *>(static_data), NULL, kernreq, ectx, nkwd,
          kwds, tp_vars);

      return ckb_offset;
    }
  };

} // namespace dynd::nd

namespace ndt {

  template <>
  struct type::equivalent<nd::mean_kernel> {
    static type make()
    {
      return nd::sum::get().get_array_type();
    }
  };

} // namespace dynd::ndt
} // namespace dynd
