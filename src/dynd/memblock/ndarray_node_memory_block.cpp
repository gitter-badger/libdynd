//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

// DEPRECATED 2012-11-02

#include <dynd/memblock/ndarray_node_memory_block.hpp>

using namespace std;
using namespace dynd;


// this one
ndarray_node_ptr dynd::make_uninitialized_ndarray_node_memory_block(intptr_t sizeof_node, char **out_node_memory)
{
    //cout << "allocating ndarray node size " << sizeof_node << endl;
    //cout << "sizeof memory_block_data " << sizeof(memory_block_data) << endl;
    // Allocate the memory_block
    char *result = reinterpret_cast<char *>(malloc(sizeof(memory_block_data) + sizeof_node));
    if (result == NULL) {
        throw bad_alloc();
    }
    *out_node_memory = result + sizeof(memory_block_data);
    return ndarray_node_ptr(new (result) memory_block_data(1, deprecated_ndarray_node_memory_block_type), false);
}

namespace dynd { namespace detail {

void free_ndarray_node_memory_block(memory_block_data *memblock)
{
    ndarray_node *node = reinterpret_cast<ndarray_node *>(reinterpret_cast<char *>(memblock) + sizeof(memory_block_data));
    node->~ndarray_node();
    free(reinterpret_cast<void *>(memblock));
}

}} // namespace dynd::detail