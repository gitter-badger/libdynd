//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _NDOBJECT_ITER_HPP_
#define _NDOBJECT_ITER_HPP_

#include <algorithm>

#include <dynd/ndobject.hpp>
#include <dynd/shape_tools.hpp>

namespace dynd {

namespace detail {
    /** A simple metaprogram to indicate whether a value is within the bounds or not */
    template<int V, int V_START, int V_END>
    struct is_value_within_bounds {
        enum {value = (V >= V_START) && V < V_END};
    };
}

template<int Nwrite, int Nread>
class ndobject_iter;

template<>
class ndobject_iter<1, 0> {
    intptr_t m_itersize;
    int m_iter_ndim;
    dimvector m_iterindex;
    dimvector m_itershape;
    char *m_data;
    const char *m_metadata;
    iterdata_common *m_iterdata;
    dtype m_array_dtype, m_uniform_dtype;
public:
    ndobject_iter(const ndobject& op0) {
        m_array_dtype = op0.get_dtype();
        m_iter_ndim = m_array_dtype.get_uniform_ndim();
        m_itersize = 1;
        if (m_iter_ndim != 0) {
            m_iterindex.init(m_iter_ndim);
            memset(m_iterindex.get(), 0, sizeof(intptr_t) * m_iter_ndim);
            m_itershape.init(m_iter_ndim);
            m_array_dtype.extended()->get_shape(0, m_itershape.get(), op0.get_ndo()->m_data_pointer, op0.get_ndo_meta());

            size_t iterdata_size = m_array_dtype.extended()->get_iterdata_size(m_iter_ndim);
            m_iterdata = reinterpret_cast<iterdata_common *>(malloc(iterdata_size));
            if (!m_iterdata) {
                throw std::bad_alloc("memory allocation error creating dynd ndobject iterator");
            }
            m_metadata = op0.get_ndo_meta();
            m_array_dtype.iterdata_construct(m_iterdata,
                            &m_metadata, m_iter_ndim, m_itershape.get(), m_uniform_dtype);
            m_data = m_iterdata->reset(m_iterdata, op0.get_ndo()->m_data_pointer, m_iter_ndim);

            for (size_t i = 0, i_end = m_iter_ndim; i != i_end; ++i) {
                m_itersize *= m_itershape[i];
            }
        } else {
            m_iterdata = NULL;
            m_uniform_dtype = m_array_dtype;
            m_data = op0.get_ndo()->m_data_pointer;
            m_metadata = op0.get_ndo_meta();
        }
    }

    ~ndobject_iter() {
        if (m_iterdata) {
            m_array_dtype.extended()->iterdata_destruct(m_iterdata, m_iter_ndim);
            free(m_iterdata);
        }
    }

    size_t itersize() const {
        return m_itersize;
    }

    bool empty() const {
        return m_itersize == 0;
    }

    bool next() {
        size_t i = m_iter_ndim;
        if (i != 0) {
            do {
                --i;
                if (++m_iterindex[i] != m_itershape[i]) {
                    m_data = m_iterdata->incr(m_iterdata, m_iter_ndim - i - 1);
                    return true;
                } else {
                    m_iterindex[i] = 0;
                }
            } while (i != 0);
        }

        return false;
    }

    char *data() {
        return m_data;
    }

    const char *metadata() {
        return m_metadata;
    }

    const dtype& get_uniform_dtype() const {
        return m_uniform_dtype;
    }
};

template<>
class ndobject_iter<1, 1> {
    intptr_t m_itersize;
    int m_iter_ndim[2];
    dimvector m_iterindex;
    dimvector m_itershape;
    char *m_data[2];
    const char *m_metadata[2];
    iterdata_common *m_iterdata[2];
    dtype m_array_dtype[2], m_uniform_dtype[2];
public:
    ndobject_iter(const ndobject& op0, const ndobject& op1) {
        m_array_dtype[0] = op0.get_dtype();
        m_array_dtype[1] = op1.get_dtype();
        m_itersize = 1;
        // The destination shape
        m_iter_ndim[0] = m_array_dtype[0].get_uniform_ndim();
        m_itershape.init(m_iter_ndim[0]);
        op0.get_shape(m_itershape.get());
        // The source shape
        dimvector src_shape;
        m_iter_ndim[1] = m_array_dtype[1].get_uniform_ndim();
        src_shape.init(m_iter_ndim[1]);
        op1.get_shape(src_shape.get());
        // Check that the source shape broadcasts ok
        if (!shape_can_broadcast(m_iter_ndim[0], m_itershape.get(), m_iter_ndim[1], src_shape.get())) {
            throw broadcast_error(op0, op1);
        }
        // Allocate and initialize the iterdata
        if (m_iter_ndim[0] != 0) {
            m_iterindex.init(m_iter_ndim[0]);
            memset(m_iterindex.get(), 0, sizeof(intptr_t) * m_iter_ndim[0]);
            // The destination iterdata
            size_t iterdata_size = m_array_dtype[0].get_iterdata_size(m_iter_ndim[0]);
            m_iterdata[0] = reinterpret_cast<iterdata_common *>(malloc(iterdata_size));
            if (!m_iterdata[0]) {
                throw std::bad_alloc("memory allocation error creating dynd ndobject iterator");
            }
            m_metadata[0] = op0.get_ndo_meta();
            m_array_dtype[0].iterdata_construct(m_iterdata[0],
                            &m_metadata[0], m_iter_ndim[0], m_itershape.get(), m_uniform_dtype[0]);
            m_data[0] = m_iterdata[0]->reset(m_iterdata[0], op0.get_readwrite_originptr(), m_iter_ndim[0]);
            // The source iterdata
            iterdata_size = m_array_dtype[1].get_broadcasted_iterdata_size(m_iter_ndim[1]);
            m_iterdata[1] = reinterpret_cast<iterdata_common *>(malloc(iterdata_size));
            if (!m_iterdata[1]) {
                throw std::bad_alloc("memory allocation error creating dynd ndobject iterator");
            }
            m_metadata[1] = op1.get_ndo_meta();
            m_array_dtype[1].broadcasted_iterdata_construct(m_iterdata[1],
                            &m_metadata[1], m_iter_ndim[1],
                            m_itershape.get() + (m_iter_ndim[0] - m_iter_ndim[1]), m_uniform_dtype[1]);
            m_data[1] = m_iterdata[1]->reset(m_iterdata[1], op1.get_readwrite_originptr(), m_iter_ndim[0]);

            for (size_t i = 0, i_end = m_iter_ndim[0]; i != i_end; ++i) {
                m_itersize *= m_itershape[i];
            }
        } else {
            m_iterdata[0] = NULL;
            m_iterdata[1] = NULL;
            m_uniform_dtype[0] = m_array_dtype[0];
            m_uniform_dtype[1] = m_array_dtype[1];
            m_data[0] = op0.get_readwrite_originptr();
            m_data[1] = op1.get_ndo()->m_data_pointer;
            m_metadata[0] = op0.get_ndo_meta();
            m_metadata[1] = op1.get_ndo_meta();
        }
    }

    ~ndobject_iter() {
        if (m_iterdata[0]) {
            m_array_dtype[0].iterdata_destruct(m_iterdata[0], m_iter_ndim[0]);
            free(m_iterdata[0]);
        }
        if (m_iterdata[1]) {
            m_array_dtype[1].iterdata_destruct(m_iterdata[1], m_iter_ndim[1]);
            free(m_iterdata[1]);
        }
    }

    size_t itersize() const {
        return m_itersize;
    }

    bool empty() const {
        return m_itersize == 0;
    }

    bool next() {
        size_t i = m_iter_ndim[0];
        if (i != 0) {
            do {
                --i;
                if (++m_iterindex[i] != m_itershape[i]) {
                    m_data[0] = m_iterdata[0]->incr(m_iterdata[0], m_iter_ndim[0] - i - 1);
                    m_data[1] = m_iterdata[1]->incr(m_iterdata[1], m_iter_ndim[0] - i - 1);
                    return true;
                } else {
                    m_iterindex[i] = 0;
                }
            } while (i != 0);
        }

        return false;
    }

    /**
     * Provide non-const access to the 'write' operands.
     */
    template<int K>
    inline typename enable_if<detail::is_value_within_bounds<K, 0, 1>::value, char *>::type data() {
        return m_data[K];
    }

    /**
     * Provide const access to all the operands.
     */
    template<int K>
    inline typename enable_if<detail::is_value_within_bounds<K, 0, 2>::value, const char *>::type data() const {
        return m_data[K];
    }

    template<int K>
    inline typename enable_if<detail::is_value_within_bounds<K, 0, 2>::value, const char *>::type metadata() const {
        return m_metadata[K];
    }

    template<int K>
    inline typename enable_if<detail::is_value_within_bounds<K, 0, 2>::value, const dtype&>::type get_uniform_dtype() const {
        return m_uniform_dtype[K];
    }
};

} // namespace dynd

#endif // _NDOBJECT_ITER_HPP_