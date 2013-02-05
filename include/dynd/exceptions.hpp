//
// Copyright (C) 2011-13, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DYND__EXCEPTIONS_HPP_
#define _DYND__EXCEPTIONS_HPP_

#include <string>
#include <stdexcept>
#include <vector>

#include <dynd/irange.hpp>

namespace dynd {

// Forward declaration of object class, for broadcast_error
class dtype;
class ndobject;

class dynd_exception : public std::exception {
protected:
    std::string m_message, m_what;
public:
    dynd_exception(const char *exception_name, const std::string& msg)
        : m_message(msg), m_what(std::string() + exception_name + ": " + msg)
    {
    }

    virtual const char* message() const throw() {
        return m_message.c_str();
    }
    virtual const char* what() const throw() {
        return m_what.c_str();
    }

    virtual ~dynd_exception() throw() {
    }
};

/**
 * An exception for various kinds of broadcast errors.
 */
class broadcast_error : public dynd_exception {
public:

    /**
     * An exception for when 'src' doesn't broadcast to 'dst'
     */
    broadcast_error(int dst_ndim, const intptr_t *dst_shape,
                        int src_ndim, const intptr_t *src_shape);

    /**
     * An exception for when 'src' doesn't broadcast to 'dst'
     */
    broadcast_error(const ndobject& dst, const ndobject& src);

    /**
     * An exception for when a number of input operands can't be broadcast
     * together.
     */
    broadcast_error(size_t ninputs, const ndobject *inputs);

    broadcast_error(const dtype& dst_dt, const char *dst_metadata,
                    const dtype& src_dt, const char *src_metadata);

    virtual ~broadcast_error() throw() {
    }
};

/**
 * An exception for an index out of bounds
 */
class too_many_indices : public dynd_exception {
public:
    /**
     * An exception for when too many indices are provided in
     * an indexing operation (nindex > ndim).
     */
    too_many_indices(const dtype& dt, size_t nindices, size_t ndim);

    virtual ~too_many_indices() throw() {
    }
};

class index_out_of_bounds : public dynd_exception {
public:
    /**
     * An exception for when 'i' isn't within bounds for
     * the specified axis of the given shape
     */
    index_out_of_bounds(intptr_t i, size_t axis, size_t ndim, const intptr_t *shape);
    index_out_of_bounds(intptr_t i, size_t axis, const std::vector<intptr_t>& shape);
    index_out_of_bounds(intptr_t i, intptr_t dimension_size);

    virtual ~index_out_of_bounds() throw() {
    }
};

class axis_out_of_bounds : public dynd_exception {
public:
    /**
     * An exception for when 'i' isn't a valid axis
     * for the number of dimensions.
     */
    axis_out_of_bounds(size_t i, size_t ndim);

    virtual ~axis_out_of_bounds() throw() {
    }
};

/**
 * An exception for a range out of bounds.
 */
class irange_out_of_bounds : public dynd_exception {
public:
    /**
     * An exception for when 'i' isn't within bounds for
     * the specified axis of the given shape
     */
    irange_out_of_bounds(const irange& i, size_t axis, size_t ndim, const intptr_t *shape);
    irange_out_of_bounds(const irange& i, size_t axis, const std::vector<intptr_t>& shape);
    irange_out_of_bounds(const irange& i, intptr_t dimension_size);

    virtual ~irange_out_of_bounds() throw() {
    }
};

/**
 * An exception for an invalid type ID.
 */
class invalid_type_id : public dynd_exception {
public:
    invalid_type_id(int type_id);

    virtual ~invalid_type_id() throw() {
    }
};

} // namespace dynd

#endif // _DYND__EXCEPTIONS_HPP_
