/*
 * Copyright (c) 1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

# include <stdio.h>     /* XXX should use <cstdio> */
# include <iostream.h>  /* XXX should use <iostream> */

__STL_BEGIN_NAMESPACE

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma set woff 1174
#endif

// Set buf_start, buf_end, and buf_ptr appropriately, filling tmp_buf
// if necessary.  Assumes _M_path_end[leaf_index] and leaf_pos are correct.
// Results in a valid buf_ptr if the iterator can be legitimately
// dereferenced.
template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_S_setbuf( 
  _Rope_iterator_base<_CharT,_Alloc>& __x)
{
    const _RopeRep* __leaf = __x._M_path_end[__x._M_leaf_index];
    size_t __leaf_pos = __x._M_leaf_pos;
    size_t __pos = __x._M_current_pos;

    switch(__leaf->_M_tag) {
	case _RopeRep::_S_leaf:
	    __x._M_buf_start = 
	      ((_Rope_RopeLeaf<_CharT,_Alloc>*)__leaf)->_M_data;
	    __x._M_buf_ptr = __x._M_buf_start + (__pos - __leaf_pos);
	    __x._M_buf_end = __x._M_buf_start + __leaf->_M_size;
	    break;
	case _RopeRep::_S_function:
	case _RopeRep::_S_substringfn:
	    {
		size_t __len = _S_iterator_buf_len;
		size_t __buf_start_pos = __leaf_pos;
		size_t __leaf_end = __leaf_pos + __leaf->_M_size;
		char_producer<_CharT>* __fn =
			((_Rope_RopeFunction<_CharT,_Alloc>*)__leaf)->_M_fn;

		if (__buf_start_pos + __len <= __pos) {
		    __buf_start_pos = __pos - __len/4;
		    if (__buf_start_pos + __len > __leaf_end) {
			__buf_start_pos = __leaf_end - __len;
		    }
		}
		if (__buf_start_pos + __len > __leaf_end) {
		    __len = __leaf_end - __buf_start_pos;
		}
		(*__fn)(__buf_start_pos - __leaf_pos, __len, __x._M_tmp_buf);
		__x._M_buf_ptr = __x._M_tmp_buf + (__pos - __buf_start_pos);
		__x._M_buf_start = __x._M_tmp_buf;
		__x._M_buf_end = __x._M_tmp_buf + __len;
	    }
	    break;
	default:
	    __stl_assert(0);
    }
}

// Set path and buffer inside a rope iterator.  We assume that 
// pos and root are already set.
template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_S_setcache
(_Rope_iterator_base<_CharT,_Alloc>& __x)
{
    const _RopeRep* __path[_RopeRep::_S_max_rope_depth+1];
    const _RopeRep* __curr_rope;
    int __curr_depth = -1;  /* index into path    */
    size_t __curr_start_pos = 0;
    size_t __pos = __x._M_current_pos;
    unsigned char __dirns = 0; // Bit vector marking right turns in the path

    __stl_assert(__pos <= __x._M_root->_M_size);
    if (__pos >= __x._M_root->_M_size) {
	__x._M_buf_ptr = 0;
	return;
    }
    __curr_rope = __x._M_root;
    if (0 != __curr_rope->_M_c_string) {
	/* Treat the root as a leaf. */
	__x._M_buf_start = __curr_rope->_M_c_string;
	__x._M_buf_end = __curr_rope->_M_c_string + __curr_rope->_M_size;
	__x._M_buf_ptr = __curr_rope->_M_c_string + __pos;
	__x._M_path_end[0] = __curr_rope;
	__x._M_leaf_index = 0;
	__x._M_leaf_pos = 0;
	return;
    }
    for(;;) {
	++__curr_depth;
	__stl_assert(__curr_depth <= _RopeRep::_S_max_rope_depth);
	__path[__curr_depth] = __curr_rope;
	switch(__curr_rope->_M_tag) {
	  case _RopeRep::_S_leaf:
	  case _RopeRep::_S_function:
	  case _RopeRep::_S_substringfn:
	    __x._M_leaf_pos = __curr_start_pos;
	    goto done;
	  case _RopeRep::_S_concat:
	    {
		_Rope_RopeConcatenation<_CharT,_Alloc>* __c =
			(_Rope_RopeConcatenation<_CharT,_Alloc>*)__curr_rope;
		_RopeRep* __left = __c->_M_left;
		size_t __left_len = __left->_M_size;
		
		__dirns <<= 1;
		if (__pos >= __curr_start_pos + __left_len) {
		    __dirns |= 1;
		    __curr_rope = __c->_M_right;
		    __curr_start_pos += __left_len;
		} else {
		    __curr_rope = __left;
		}
	    }
	    break;
	}
    }
  done:
    // Copy last section of path into _M_path_end.
      {
	int __i = -1;
	int __j = __curr_depth + 1 - _S_path_cache_len;

	if (__j < 0) __j = 0;
	while (__j <= __curr_depth) {
	    __x._M_path_end[++__i] = __path[__j++];
	}
	__x._M_leaf_index = __i;
      }
      __x._M_path_directions = __dirns;
      _S_setbuf(__x);
}

// Specialized version of the above.  Assumes that
// the path cache is valid for the previous position.
template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_S_setcache_for_incr
(_Rope_iterator_base<_CharT,_Alloc>& __x)
{
    int __current_index = __x._M_leaf_index;
    const _RopeRep* __current_node = __x._M_path_end[__current_index];
    size_t __len = __current_node->_M_size;
    size_t __node_start_pos = __x._M_leaf_pos;
    unsigned char __dirns = __x._M_path_directions;
    _Rope_RopeConcatenation<_CharT,_Alloc>* __c;

    __stl_assert(__x._M_current_pos <= __x._M_root->_M_size);
    if (__x._M_current_pos - __node_start_pos < __len) {
	/* More stuff in this leaf, we just didn't cache it. */
	_S_setbuf(__x);
	return;
    }
    __stl_assert(__node_start_pos + __len == __x._M_current_pos);
    //  node_start_pos is starting position of last_node.
    while (--__current_index >= 0) {
	if (!(__dirns & 1) /* Path turned left */) 
	  break;
	__current_node = __x._M_path_end[__current_index];
	__c = (_Rope_RopeConcatenation<_CharT,_Alloc>*)__current_node;
	// Otherwise we were in the right child.  Thus we should pop
	// the concatenation node.
	__node_start_pos -= __c->_M_left->_M_size;
	__dirns >>= 1;
    }
    if (__current_index < 0) {
	// We underflowed the cache. Punt.
	_S_setcache(__x);
	return;
    }
    __current_node = __x._M_path_end[__current_index];
    __c = (_Rope_RopeConcatenation<_CharT,_Alloc>*)__current_node;
    // current_node is a concatenation node.  We are positioned on the first
    // character in its right child.
    // node_start_pos is starting position of current_node.
    __node_start_pos += __c->_M_left->_M_size;
    __current_node = __c->_M_right;
    __x._M_path_end[++__current_index] = __current_node;
    __dirns |= 1;
    while (_RopeRep::_S_concat == __current_node->_M_tag) {
	++__current_index;
	if (_S_path_cache_len == __current_index) {
	    int __i;
	    for (__i = 0; __i < _S_path_cache_len-1; __i++) {
		__x._M_path_end[__i] = __x._M_path_end[__i+1];
	    }
	    --__current_index;
	}
	__current_node =
	    ((_Rope_RopeConcatenation<_CharT,_Alloc>*)__current_node)->_M_left;
	__x._M_path_end[__current_index] = __current_node;
	__dirns <<= 1;
	// node_start_pos is unchanged.
    }
    __x._M_leaf_index = __current_index;
    __x._M_leaf_pos = __node_start_pos;
    __x._M_path_directions = __dirns;
    _S_setbuf(__x);
}

template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_M_incr(size_t __n) {
    _M_current_pos += __n;
    if (0 != _M_buf_ptr) {
        size_t __chars_left = _M_buf_end - _M_buf_ptr;
        if (__chars_left > __n) {
            _M_buf_ptr += __n;
        } else if (__chars_left == __n) {
            _M_buf_ptr += __n;
            _S_setcache_for_incr(*this);
        } else {
            _M_buf_ptr = 0;
        }
    }
}

template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_M_decr(size_t __n) {
    if (0 != _M_buf_ptr) {
        size_t __chars_left = _M_buf_ptr - _M_buf_start;
        if (__chars_left >= __n) {
            _M_buf_ptr -= __n;
        } else {
            _M_buf_ptr = 0;
        }
    }
    _M_current_pos -= __n;
}

template <class _CharT, class _Alloc>
void _Rope_iterator<_CharT,_Alloc>::_M_check() {
    if (_M_root_rope->_M_tree_ptr != _M_root) {
        // _Rope was modified.  Get things fixed up.
        _RopeRep::_S_unref(_M_root);
        _M_root = _M_root_rope->_M_tree_ptr;
        _RopeRep::_S_ref(_M_root);
        _M_buf_ptr = 0;
    }
}

template <class _CharT, class _Alloc>
inline 
_Rope_const_iterator<_CharT, _Alloc>::_Rope_const_iterator(
  const _Rope_iterator<_CharT,_Alloc>& __x)
: _Rope_iterator_base<_CharT,_Alloc>(__x) 
{ }

template <class _CharT, class _Alloc>
inline _Rope_iterator<_CharT,_Alloc>::_Rope_iterator(
  rope<_CharT,_Alloc>& __r, size_t __pos)
: _Rope_iterator_base<_CharT,_Alloc>(__r._M_tree_ptr, __pos), 
  _M_root_rope(&__r)
{
    _RopeRep::_S_ref(_M_root);
}

template <class _CharT, class _Alloc>
inline size_t 
rope<_CharT,_Alloc>::_S_char_ptr_len(const _CharT* __s)
{
    const _CharT* __p = __s;

    while (!_S_is0(*__p)) { ++__p; }
    return (__p - __s);
}


#ifndef __GC

template <class _CharT, class _Alloc>
inline void _Rope_RopeRep<_CharT,_Alloc>::_M_free_c_string()
{
    _CharT* __cstr = _M_c_string;
    if (0 != __cstr) {
	size_t __size = _M_size + 1;
	destroy(__cstr, __cstr + __size);
	_Data_deallocate(__cstr, __size);
    }
}


template <class _CharT, class _Alloc>
#ifdef __STL_USE_STD_ALLOCATORS
  inline void _Rope_RopeRep<_CharT,_Alloc>::_S_free_string(_CharT* __s,
							   size_t __n,
						           allocator_type __a)
#else
  inline void _Rope_RopeRep<_CharT,_Alloc>::_S_free_string(_CharT* __s,
							   size_t __n)
#endif
{
    if (!_S_is_basic_char_type((_CharT*)0)) {
	destroy(__s, __s + __n);
    }
//  This has to be a static member, so this gets a bit messy
#   ifdef __STL_USE_STD_ALLOCATORS
        __a.deallocate(
	    __s, _Rope_RopeLeaf<_CharT,_Alloc>::_S_rounded_up_size(__n));
#   else
	_Data_deallocate(
	    __s, _Rope_RopeLeaf<_CharT,_Alloc>::_S_rounded_up_size(__n));
#   endif
}


//  There are several reasons for not doing this with virtual destructors
//  and a class specific delete operator:
//  - A class specific delete operator can't easily get access to
//    allocator instances if we need them.
//  - Any virtual function would need a 4 or byte vtable pointer;
//    this only requires a one byte tag per object.
template <class _CharT, class _Alloc>
void _Rope_RopeRep<_CharT,_Alloc>::_M_free_tree()
{
    switch(_M_tag) {
	case _S_leaf:
	    {
	        _Rope_RopeLeaf<_CharT,_Alloc>* __l
			= (_Rope_RopeLeaf<_CharT,_Alloc>*)this;
	        __l->_Rope_RopeLeaf<_CharT,_Alloc>::~_Rope_RopeLeaf();
	        _L_deallocate(__l, 1);
	        break;
	    }
	case _S_concat:
	    {
	        _Rope_RopeConcatenation<_CharT,_Alloc>* __c
		    = (_Rope_RopeConcatenation<_CharT,_Alloc>*)this;
	        __c->_Rope_RopeConcatenation<_CharT,_Alloc>::
		       ~_Rope_RopeConcatenation();
	        _C_deallocate(__c, 1);
	        break;
	    }
	case _S_function:
	    {
	        _Rope_RopeFunction<_CharT,_Alloc>* __f
		    = (_Rope_RopeFunction<_CharT,_Alloc>*)this;
	        __f->_Rope_RopeFunction<_CharT,_Alloc>::~_Rope_RopeFunction();
	        _F_deallocate(__f, 1);
	        break;
	    }
	case _S_substringfn:
	    {
	        _Rope_RopeSubstring<_CharT,_Alloc>* __ss =
			(_Rope_RopeSubstring<_CharT,_Alloc>*)this;
		__ss->_Rope_RopeSubstring<_CharT,_Alloc>::
		        ~_Rope_RopeSubstring();
		_S_deallocate(__ss, 1);
		break;
	    }
    }
}
#else

template <class _CharT, class _Alloc>
#ifdef __STL_USE_STD_ALLOCATORS
  inline void _Rope_RopeRep<_CharT,_Alloc>::_S_free_string
		(const _CharT*, size_t, allocator_type)
#else
  inline void _Rope_RopeRep<_CharT,_Alloc>::_S_free_string
		(const _CharT*, size_t)
#endif
{}

#endif


// Concatenate a C string onto a leaf rope by copying the rope data.
// Used for short ropes.
template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeLeaf*
rope<_CharT,_Alloc>::_S_leaf_concat_char_iter
		(_RopeLeaf* __r, const _CharT* __iter, size_t __len)
{
    size_t __old_len = __r->_M_size;
    _CharT* __new_data = (_CharT*)
	_Data_allocate(_S_rounded_up_size(__old_len + __len));
    _RopeLeaf* __result;
    
    uninitialized_copy_n(__r->_M_data, __old_len, __new_data);
    uninitialized_copy_n(__iter, __len, __new_data + __old_len);
    _S_cond_store_eos(__new_data[__old_len + __len]);
    __STL_TRY {
	__result = _S_new_RopeLeaf(__new_data, __old_len + __len,
				   __r->get_allocator());
    }
    __STL_UNWIND(_RopeRep::__STL_FREE_STRING(__new_data, __old_len + __len,
					     __r->get_allocator()));
    return __result;
}

#ifndef __GC
// As above, but it's OK to clobber original if refcount is 1
template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeLeaf*
rope<_CharT,_Alloc>::_S_destr_leaf_concat_char_iter
		(_RopeLeaf* __r, const _CharT* __iter, size_t __len)
{
    __stl_assert(__r->_M_refcount >= 1);
    if (__r->_M_refcount > 1)
      return _S_leaf_concat_char_iter(__r, __iter, __len);
    size_t __old_len = __r->_M_size;
    if (_S_allocated_capacity(__old_len) >= __old_len + __len) {
	// The space has been partially initialized for the standard
	// character types.  But that doesn't matter for those types.
	uninitialized_copy_n(__iter, __len, __r->_M_data + __old_len);
	if (_S_is_basic_char_type((_CharT*)0)) {
	    _S_cond_store_eos(__r->_M_data[__old_len + __len]);
	    __stl_assert(__r->_M_c_string == __r->_M_data);
	} else if (__r->_M_c_string != __r->_M_data && 0 != __r->_M_c_string) {
	    __r->_M_free_c_string();
	    __r->_M_c_string = 0;
	}
	__r->_M_size = __old_len + __len;
	__stl_assert(__r->_M_refcount == 1);
	__r->_M_refcount = 2;
	return __r;
    } else {
	_RopeLeaf* __result = _S_leaf_concat_char_iter(__r, __iter, __len);
	__stl_assert(__result->_M_refcount == 1);
	return __result;
    }
}
#endif

// Assumes left and right are not 0.
// Does not increment (nor decrement on exception) child reference counts.
// Result has ref count 1.
template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeRep*
rope<_CharT,_Alloc>::_S_tree_concat (_RopeRep* __left, _RopeRep* __right)
{
    _RopeConcatenation* __result =
      _S_new_RopeConcatenation(__left, __right, __left->get_allocator());
    size_t __depth = __result->_M_depth;
    
#   ifdef __STL_USE_STD_ALLOCATORS
      __stl_assert(__left->get_allocator() == __right->get_allocator());
#   endif
    if (__depth > 20 && (__result->_M_size < 1000 ||
			 __depth > _RopeRep::_S_max_rope_depth)) {
        _RopeRep* __balanced;
      
	__STL_TRY {
	   __balanced = _S_balance(__result);
#          ifndef __GC
	     if (__result != __balanced) {
		__stl_assert(1 == __result->_M_refcount
			     && 1 == __balanced->_M_refcount);
	     }
#          endif
	   __result->_M_unref_nonnil();
        }
	__STL_UNWIND((_C_deallocate(__result,1)));
		// In case of exception, we need to deallocate
		// otherwise dangling result node.  But caller
		// still owns its children.  Thus unref is
		// inappropriate.
	return __balanced;
    } else {
	return __result;
    }
}

template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeRep* rope<_CharT,_Alloc>::_S_concat_char_iter
		(_RopeRep* __r, const _CharT*__s, size_t __slen)
{
    _RopeRep* __result;
    if (0 == __slen) {
	_S_ref(__r);
	return __r;
    }
    if (0 == __r)
      return __STL_ROPE_FROM_UNOWNED_CHAR_PTR(__s, __slen,
					      __r->get_allocator());
    if (_RopeRep::_S_leaf == __r->_M_tag && 
          __r->_M_size + __slen <= _S_copy_max) {
	__result = _S_leaf_concat_char_iter((_RopeLeaf*)__r, __s, __slen);
#       ifndef __GC
	  __stl_assert(1 == __result->_M_refcount);
#       endif
	return __result;
    }
    if (_RopeRep::_S_concat == __r->_M_tag
	&& _RopeRep::_S_leaf == ((_RopeConcatenation*)__r)->_M_right->_M_tag) {
	_RopeLeaf* __right = 
	  (_RopeLeaf* )(((_RopeConcatenation* )__r)->_M_right);
	if (__right->_M_size + __slen <= _S_copy_max) {
	  _RopeRep* __left = ((_RopeConcatenation*)__r)->_M_left;
	  _RopeRep* __nright = 
	    _S_leaf_concat_char_iter((_RopeLeaf*)__right, __s, __slen);
	  __left->_M_ref_nonnil();
	  __STL_TRY {
	    __result = _S_tree_concat(__left, __nright);
          }
	  __STL_UNWIND(_S_unref(__left); _S_unref(__nright));
#         ifndef __GC
	    __stl_assert(1 == __result->_M_refcount);
#         endif
	  return __result;
	}
    }
    _RopeRep* __nright =
      __STL_ROPE_FROM_UNOWNED_CHAR_PTR(__s, __slen, __r->get_allocator());
    __STL_TRY {
      __r->_M_ref_nonnil();
      __result = _S_tree_concat(__r, __nright);
    }
    __STL_UNWIND(_S_unref(__r); _S_unref(__nright));
#   ifndef __GC
      __stl_assert(1 == __result->_M_refcount);
#   endif
    return __result;
}

#ifndef __GC
template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeRep* 
rope<_CharT,_Alloc>::_S_destr_concat_char_iter(
  _RopeRep* __r, const _CharT* __s, size_t __slen)
{
    _RopeRep* __result;
    if (0 == __r)
      return __STL_ROPE_FROM_UNOWNED_CHAR_PTR(__s, __slen,
					      __r->get_allocator());
    size_t __count = __r->_M_refcount;
    size_t __orig_size = __r->_M_size;
    __stl_assert(__count >= 1);
    if (__count > 1) return _S_concat_char_iter(__r, __s, __slen);
    if (0 == __slen) {
	__r->_M_refcount = 2;      // One more than before
	return __r;
    }
    if (__orig_size + __slen <= _S_copy_max && 
          _RopeRep::_S_leaf == __r->_M_tag) {
	__result = _S_destr_leaf_concat_char_iter((_RopeLeaf*)__r, __s, __slen);
	return __result;
    }
    if (_RopeRep::_S_concat == __r->_M_tag) {
	_RopeLeaf* __right = (_RopeLeaf*)(((_RopeConcatenation*)__r)->_M_right);
	if (_RopeRep::_S_leaf == __right->_M_tag
	    && __right->_M_size + __slen <= _S_copy_max) {
	  _RopeRep* __new_right = 
	    _S_destr_leaf_concat_char_iter(__right, __s, __slen);
	  if (__right == __new_right) {
	      __stl_assert(__new_right->_M_refcount == 2);
	      __new_right->_M_refcount = 1;
	  } else {
	      __stl_assert(__new_right->_M_refcount >= 1);
	      __right->_M_unref_nonnil();
	  }
	  __stl_assert(__r->_M_refcount == 1);
	  __r->_M_refcount = 2;    // One more than before.
	  ((_RopeConcatenation*)__r)->_M_right = __new_right;
	  __r->_M_size = __orig_size + __slen;
	  if (0 != __r->_M_c_string) {
	      __r->_M_free_c_string();
	      __r->_M_c_string = 0;
	  }
	  return __r;
	}
    }
    _RopeRep* __right =
      __STL_ROPE_FROM_UNOWNED_CHAR_PTR(__s, __slen, __r->get_allocator());
    __r->_M_ref_nonnil();
    __STL_TRY {
      __result = _S_tree_concat(__r, __right);
    }
    __STL_UNWIND(_S_unref(__r); _S_unref(__right))
    __stl_assert(1 == __result->_M_refcount);
    return __result;
}
#endif /* !__GC */

template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeRep*
rope<_CharT,_Alloc>::_S_concat(_RopeRep* __left, _RopeRep* __right)
{
    if (0 == __left) {
	_S_ref(__right);
	return __right;
    }
    if (0 == __right) {
	__left->_M_ref_nonnil();
	return __left;
    }
    if (_RopeRep::_S_leaf == __right->_M_tag) {
	if (_RopeRep::_S_leaf == __left->_M_tag) {
	  if (__right->_M_size + __left->_M_size <= _S_copy_max) {
	    return _S_leaf_concat_char_iter((_RopeLeaf*)__left,
					 ((_RopeLeaf*)__right)->_M_data,
					 __right->_M_size);
	  }
	} else if (_RopeRep::_S_concat == __left->_M_tag
		   && _RopeRep::_S_leaf ==
		      ((_RopeConcatenation*)__left)->_M_right->_M_tag) {
	  _RopeLeaf* __leftright =
		    (_RopeLeaf*)(((_RopeConcatenation*)__left)->_M_right); 
	  if (__leftright->_M_size + __right->_M_size <= _S_copy_max) {
	    _RopeRep* __leftleft = ((_RopeConcatenation*)__left)->_M_left;
	    _RopeRep* __rest = _S_leaf_concat_char_iter(__leftright,
					   ((_RopeLeaf*)__right)->_M_data,
					   __right->_M_size);
	    __leftleft->_M_ref_nonnil();
	    __STL_TRY {
	      return(_S_tree_concat(__leftleft, __rest));
            }
	    __STL_UNWIND(_S_unref(__leftleft); _S_unref(__rest))
	  }
	}
    }
    __left->_M_ref_nonnil();
    __right->_M_ref_nonnil();
    __STL_TRY {
      return(_S_tree_concat(__left, __right));
    }
    __STL_UNWIND(_S_unref(__left); _S_unref(__right));
}

template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeRep*
rope<_CharT,_Alloc>::_S_substring(_RopeRep* __base, 
                               size_t __start, size_t __endp1)
{
    if (0 == __base) return 0;
    size_t __len = __base->_M_size;
    size_t __adj_endp1;
    const size_t __lazy_threshold = 128;
    
    if (__endp1 >= __len) {
	if (0 == __start) {
	    __base->_M_ref_nonnil();
	    return __base;
	} else {
	    __adj_endp1 = __len;
	}
    } else {
	__adj_endp1 = __endp1;
    }
    switch(__base->_M_tag) {
	case _RopeRep::_S_concat:
	    {
		_RopeConcatenation* __c = (_RopeConcatenation*)__base;
		_RopeRep* __left = __c->_M_left;
		_RopeRep* __right = __c->_M_right;
		size_t __left_len = __left->_M_size;
		_RopeRep* __result;

		if (__adj_endp1 <= __left_len) {
		    return _S_substring(__left, __start, __endp1);
		} else if (__start >= __left_len) {
		    return _S_substring(__right, __start - __left_len,
				  __adj_endp1 - __left_len);
		}
		_Self_destruct_ptr __left_result(
		  _S_substring(__left, __start, __left_len));
		_Self_destruct_ptr __right_result(
		  _S_substring(__right, 0, __endp1 - __left_len));
		__result = _S_concat(__left_result, __right_result);
#               ifndef __GC
		  __stl_assert(1 == __result->_M_refcount);
#               endif
		return __result;
	    }
	case _RopeRep::_S_leaf:
	    {
		_RopeLeaf* __l = (_RopeLeaf*)__base;
		_RopeLeaf* __result;
		size_t __result_len;
		if (__start >= __adj_endp1) return 0;
		__result_len = __adj_endp1 - __start;
		if (__result_len > __lazy_threshold) goto lazy;
#               ifdef __GC
		    const _CharT* __section = __l->_M_data + __start;
		    __result = _S_new_RopeLeaf(__section, __result_len,
					  __base->get_allocator());
		    __result->_M_c_string = 0;  // Not eos terminated.
#               else
		    // We should sometimes create substring node instead.
		    __result = __STL_ROPE_FROM_UNOWNED_CHAR_PTR(
					__l->_M_data + __start, __result_len,
					__base->get_allocator());
#               endif
		return __result;
	    }
	case _RopeRep::_S_substringfn:
	    // Avoid introducing multiple layers of substring nodes.
	    {
		_RopeSubstring* __old = (_RopeSubstring*)__base;
		size_t __result_len;
		if (__start >= __adj_endp1) return 0;
		__result_len = __adj_endp1 - __start;
		if (__result_len > __lazy_threshold) {
		    _RopeSubstring* __result =
			_S_new_RopeSubstring(__old->_M_base,
					  __start + __old->_M_start,
					  __adj_endp1 - __start,
					  __base->get_allocator());
		    return __result;

		} // *** else fall through: ***
	    }
	case _RopeRep::_S_function:
	    {
		_RopeFunction* __f = (_RopeFunction*)__base;
		_CharT* __section;
		size_t __result_len;
		if (__start >= __adj_endp1) return 0;
		__result_len = __adj_endp1 - __start;

		if (__result_len > __lazy_threshold) goto lazy;
		__section = (_CharT*)
			_Data_allocate(_S_rounded_up_size(__result_len));
		__STL_TRY {
		  (*(__f->_M_fn))(__start, __result_len, __section);
                }
		__STL_UNWIND(_RopeRep::__STL_FREE_STRING(
	               __section, __result_len, __base->get_allocator()));
		_S_cond_store_eos(__section[__result_len]);
		return _S_new_RopeLeaf(__section, __result_len,
				       __base->get_allocator());
	    }
    }
    /*NOTREACHED*/
    __stl_assert(false);
  lazy:
    {
	// Create substring node.
	return _S_new_RopeSubstring(__base, __start, __adj_endp1 - __start,
			       __base->get_allocator());
    }
}

template<class _CharT>
class _Rope_flatten_char_consumer : public _Rope_char_consumer<_CharT> {
    private:
	_CharT* _M_buf_ptr;
    public:
	//  _CharT* _M_buffer;  // XXX not used

	_Rope_flatten_char_consumer(_CharT* __buffer) {
	    _M_buf_ptr = __buffer;
	};
	~_Rope_flatten_char_consumer() {}
	bool operator() (const _CharT* __leaf, size_t __n) {
	    uninitialized_copy_n(__leaf, __n, _M_buf_ptr);
	    _M_buf_ptr += __n;
	    return true;
	}
};
	    
template<class _CharT>
class _Rope_find_char_char_consumer : public _Rope_char_consumer<_CharT> {
    private:
	_CharT _M_pattern;
    public:
	size_t _M_count;  // Number of nonmatching characters
	_Rope_find_char_char_consumer(_CharT __p) 
	  : _M_pattern(__p), _M_count(0) {}
	~_Rope_find_char_char_consumer() {}
	bool operator() (const _CharT* __leaf, size_t __n) {
	    size_t __i;
	    for (__i = 0; __i < __n; __i++) {
		if (__leaf[__i] == _M_pattern) {
		    _M_count += __i; return false;
		}
	    }
	    _M_count += __n; return true;
	}
};
	    
template<class _CharT>
class _Rope_insert_char_consumer : public _Rope_char_consumer<_CharT> {
    private:
	typedef ostream _Insert_ostream;
	_Insert_ostream& _M_o;
    public:
	// _CharT* buffer;    // XXX not used
	_Rope_insert_char_consumer(_Insert_ostream& __writer) 
	  : _M_o(__writer) {};
	~_Rope_insert_char_consumer() { };
		// Caller is presumed to own the ostream
	bool operator() (const _CharT* __leaf, size_t __n);
		// Returns true to continue traversal.
};
	    
template<class _CharT>
bool _Rope_insert_char_consumer<_CharT>::operator()
					(const _CharT* __leaf, size_t __n)
{
    size_t __i;
    //  We assume that formatting is set up correctly for each element.
    for (__i = 0; __i < __n; __i++) _M_o << __leaf[__i];
    return true;
}

inline bool _Rope_insert_char_consumer<char>::operator()
					(const char* __leaf, size_t __n)
{
    size_t __i;
    for (__i = 0; __i < __n; __i++) _M_o.put(__leaf[__i]);
    return true;
}

#if 0
// I couldn't get this to work work with the VC++ version of basic_ostream.
// It also doesn't really do the right thing unless o is a wide stream.
// Given that wchar_t is often 4 bytes, its not clear to me how useful
// this stuff is anyway.
inline bool _Rope_insert_char_consumer<wchar_t>::operator()
					(const wchar_t* __leaf, size_t __n)
{
    size_t __i;
    for (__i = 0; __i < __n; __i++) _M_o.put(__leaf[__i]);
    return true;
}
#endif /* !_MSC_VER  && !BORLAND */

template <class _CharT, class _Alloc>
bool rope<_CharT, _Alloc>::_S_apply_to_pieces(
				_Rope_char_consumer<_CharT>& __c,
				const _RopeRep* __r,
				size_t __begin, size_t __end)
{
    if (0 == __r) return true;
    switch(__r->_M_tag) {
	case _RopeRep::_S_concat:
	    {
		_RopeConcatenation* __conc = (_RopeConcatenation*)__r;
		_RopeRep* __left =  __conc->_M_left;
		size_t __left_len = __left->_M_size;
		if (__begin < __left_len) {
		    size_t __left_end = min(__left_len, __end);
		    if (!_S_apply_to_pieces(__c, __left, __begin, __left_end))
			return false;
		}
		if (__end > __left_len) {
		    _RopeRep* __right =  __conc->_M_right;
		    size_t __right_start = max(__left_len, __begin);
		    if (!_S_apply_to_pieces(__c, __right,
					 __right_start - __left_len,
					 __end - __left_len)) {
			return false;
		    }
		}
	    }
	    return true;
	case _RopeRep::_S_leaf:
	    {
		_RopeLeaf* __l = (_RopeLeaf*)__r;
		return __c(__l->_M_data + __begin, __end - __begin);
	    }
	case _RopeRep::_S_function:
	case _RopeRep::_S_substringfn:
	    {
		_RopeFunction* __f = (_RopeFunction*)__r;
		size_t __len = __end - __begin;
		bool __result;
		_CharT* __buffer =
		  (_CharT*)alloc::allocate(__len * sizeof(_CharT));
		__STL_TRY {
		  (*(__f->_M_fn))(__begin, __end, __buffer);
		  __result = __c(__buffer, __len);
                  alloc::deallocate(__buffer, __len * sizeof(_CharT));
                }
		__STL_UNWIND((alloc::deallocate(__buffer,
						__len * sizeof(_CharT))))
		return __result;
	    }
	default:
	    __stl_assert(false);
	    /*NOTREACHED*/
	    return false;
    }
}

inline void _Rope_fill(ostream& __o, size_t __n)
{
    char __f = __o.fill();
    size_t __i;

    for (__i = 0; __i < __n; __i++) __o.put(__f);
}
    

template <class _CharT> inline bool _Rope_is_simple(_CharT*) { return false; }
inline bool _Rope_is_simple(char*) { return true; }
inline bool _Rope_is_simple(wchar_t*) { return true; }


template<class _CharT, class _Alloc>
ostream& operator<< (ostream& __o, const rope<_CharT, _Alloc>& __r)
{
    size_t __w = __o.width();
    bool __left = bool(__o.flags() & ios::left);
    size_t __pad_len;
    size_t __rope_len = __r.size();
    _Rope_insert_char_consumer<_CharT> __c(__o);
    bool __is_simple = _Rope_is_simple((_CharT*)0);
    
    if (__rope_len < __w) {
	__pad_len = __w - __rope_len;
    } else {
	__pad_len = 0;
    }
    if (!__is_simple) __o.width(__w/__rope_len);
    __STL_TRY {
      if (__is_simple && !__left && __pad_len > 0) {
	_Rope_fill(__o, __pad_len);
      }
      __r.apply_to_pieces(0, __r.size(), __c);
      if (__is_simple && __left && __pad_len > 0) {
	_Rope_fill(__o, __pad_len);
      }
      if (!__is_simple)
        __o.width(__w);
    }
    __STL_UNWIND(if (!__is_simple) __o.width(__w))
    return __o;
}

template <class _CharT, class _Alloc>
_CharT*
rope<_CharT,_Alloc>::_S_flatten(_RopeRep* __r,
				 size_t __start, size_t __len,
				 _CharT* __buffer)
{
    _Rope_flatten_char_consumer<_CharT> __c(__buffer);
    _S_apply_to_pieces(__c, __r, __start, __start + __len);
    return(__buffer + __len);
}

template <class _CharT, class _Alloc>
size_t
rope<_CharT,_Alloc>::find(_CharT __pattern, size_t __start) const
{
    _Rope_find_char_char_consumer<_CharT> __c(__pattern);
    _S_apply_to_pieces(__c, _M_tree_ptr, __start, size());
    size_type __result_pos = __start + __c._M_count;
#   ifndef __STL_OLD_ROPE_SEMANTICS
	if (__result_pos == size()) __result_pos = npos;
#   endif
    return __result_pos;
}

template <class _CharT, class _Alloc>
_CharT*
rope<_CharT,_Alloc>::_S_flatten(_RopeRep* __r, _CharT* __buffer)
{
    if (0 == __r) return __buffer;
    switch(__r->_M_tag) {
	case _RopeRep::_S_concat:
	    {
		_RopeConcatenation* __c = (_RopeConcatenation*)__r;
		_RopeRep* __left = __c->_M_left;
		_RopeRep* __right = __c->_M_right;
		_CharT* __rest = _S_flatten(__left, __buffer);
		return _S_flatten(__right, __rest);
	    }
	case _RopeRep::_S_leaf:
	    {
		_RopeLeaf* __l = (_RopeLeaf*)__r;
		return copy_n(__l->_M_data, __l->_M_size, __buffer).second;
	    }
	case _RopeRep::_S_function:
	case _RopeRep::_S_substringfn:
	    // We dont yet do anything with substring nodes.
	    // This needs to be fixed before ropefiles will work well.
	    {
		_RopeFunction* __f = (_RopeFunction*)__r;
		(*(__f->_M_fn))(0, __f->_M_size, __buffer);
		return __buffer + __f->_M_size;
	    }
	default:
	    __stl_assert(false);
	    /*NOTREACHED*/
	    return 0;
    }
}


// This needs work for _CharT != char
template <class _CharT, class _Alloc>
void
rope<_CharT,_Alloc>::_S_dump(_RopeRep* __r, int __indent)
{
    for (int __i = 0; __i < __indent; __i++) putchar(' ');
    if (0 == __r) {
	printf("NULL\n"); return;
    }
    if (_RopeRep::_S_concat == __r->_M_tag) {
	_RopeConcatenation* __c = (_RopeConcatenation*)__r;
	_RopeRep* __left = __c->_M_left;
	_RopeRep* __right = __c->_M_right;

#       ifdef __GC
	  printf("Concatenation %p (depth = %d, len = %ld, %s balanced)\n",
	    __r, __r->_M_depth, __r->_M_size, __r->_M_is_balanced? "" : "not");
#       else
	  printf("Concatenation %p (rc = %ld, depth = %d, "
	           "len = %ld, %s balanced)\n",
		 __r, __r->_M_refcount, __r->_M_depth, __r->_M_size,
		 __r->_M_is_balanced? "" : "not");
#       endif
	_S_dump(__left, __indent + 2);
	_S_dump(__right, __indent + 2);
	return;
    } else {
	char* __kind;

	switch (__r->_M_tag) {
	    case _RopeRep::_S_leaf:
		__kind = "Leaf";
		break;
	    case _RopeRep::_S_function:
		__kind = "Function";
		break;
	    case _RopeRep::_S_substringfn:
		__kind = "Function representing substring";
		break;
	    default:
		__kind = "(corrupted kind field!)";
	}
#       ifdef __GC
	  printf("%s %p (depth = %d, len = %ld) ",
		 __kind, __r, __r->_M_depth, __r->_M_size);
#       else
	  printf("%s %p (rc = %ld, depth = %d, len = %ld) ",
		 __kind, __r, __r->_M_refcount, __r->_M_depth, __r->_M_size);
#       endif
	if (_S_is_one_byte_char_type((_CharT*)0)) {
	    const int __max_len = 40;
	    _Self_destruct_ptr __prefix(_S_substring(__r, 0, __max_len));
	    _CharT __buffer[__max_len + 1];
	    bool __too_big = __r->_M_size > __prefix->_M_size;

	    _S_flatten(__prefix, __buffer);
	    __buffer[__prefix->_M_size] = _S_eos((_CharT*)0); 
	    printf("%s%s\n", 
	           (char*)__buffer, __too_big? "...\n" : "\n");
	} else {
	    printf("\n");
	}
    }
}

template <class _CharT, class _Alloc>
const unsigned long
rope<_CharT,_Alloc>::_S_min_len[
  _Rope_RopeRep<_CharT,_Alloc>::_S_max_rope_depth + 1] = {
/* 0 */1, /* 1 */2, /* 2 */3, /* 3 */5, /* 4 */8, /* 5 */13, /* 6 */21,
/* 7 */34, /* 8 */55, /* 9 */89, /* 10 */144, /* 11 */233, /* 12 */377,
/* 13 */610, /* 14 */987, /* 15 */1597, /* 16 */2584, /* 17 */4181,
/* 18 */6765, /* 19 */10946, /* 20 */17711, /* 21 */28657, /* 22 */46368,
/* 23 */75025, /* 24 */121393, /* 25 */196418, /* 26 */317811,
/* 27 */514229, /* 28 */832040, /* 29 */1346269, /* 30 */2178309,
/* 31 */3524578, /* 32 */5702887, /* 33 */9227465, /* 34 */14930352,
/* 35 */24157817, /* 36 */39088169, /* 37 */63245986, /* 38 */102334155,
/* 39 */165580141, /* 40 */267914296, /* 41 */433494437,
/* 42 */701408733, /* 43 */1134903170, /* 44 */1836311903,
/* 45 */2971215073u };
// These are Fibonacci numbers < 2**32.

template <class _CharT, class _Alloc>
rope<_CharT,_Alloc>::_RopeRep*
rope<_CharT,_Alloc>::_S_balance(_RopeRep* __r)
{
    _RopeRep* __forest[_RopeRep::_S_max_rope_depth + 1];
    _RopeRep* __result = 0;
    int __i;
    // Invariant:
    // The concatenation of forest in descending order is equal to __r.
    // __forest[__i]._M_size >= _S_min_len[__i]
    // __forest[__i]._M_depth = __i
    // References from forest are included in refcount.

    for (__i = 0; __i <= _RopeRep::_S_max_rope_depth; ++__i) 
      __forest[__i] = 0;
    __STL_TRY {
      _S_add_to_forest(__r, __forest);
      for (__i = 0; __i <= _RopeRep::_S_max_rope_depth; ++__i) 
        if (0 != __forest[__i]) {
#	ifndef __GC
	  _Self_destruct_ptr __old(__result);
#	endif
	  __result = _S_concat(__forest[__i], __result);
	__forest[__i]->_M_unref_nonnil();
#	if !defined(__GC) && defined(__STL_USE_EXCEPTIONS)
	  __forest[__i] = 0;
#	endif
      }
    }
    __STL_UNWIND(for(__i = 0; __i <= _RopeRep::_S_max_rope_depth; __i++)
		 _S_unref(__forest[__i]))
    if (__result->_M_depth > _RopeRep::_S_max_rope_depth) abort();
    return(__result);
}


template <class _CharT, class _Alloc>
void
rope<_CharT,_Alloc>::_S_add_to_forest(_RopeRep* __r, _RopeRep** __forest)
{
    if (__r->_M_is_balanced) {
	_S_add_leaf_to_forest(__r, __forest);
	return;
    }
    __stl_assert(__r->_M_tag == _RopeRep::_S_concat);
    {
	_RopeConcatenation* __c = (_RopeConcatenation*)__r;

	_S_add_to_forest(__c->_M_left, __forest);
	_S_add_to_forest(__c->_M_right, __forest);
    }
}


template <class _CharT, class _Alloc>
void
rope<_CharT,_Alloc>::_S_add_leaf_to_forest(_RopeRep* __r, _RopeRep** __forest)
{
    _RopeRep* __insertee;   		// included in refcount
    _RopeRep* __too_tiny = 0;    	// included in refcount
    int __i;  				// forest[0..__i-1] is empty
    size_t __s = __r->_M_size;

    for (__i = 0; __s >= _S_min_len[__i+1]/* not this bucket */; ++__i) {
	if (0 != __forest[__i]) {
#	    ifndef __GC
	      _Self_destruct_ptr __old(__too_tiny);
#	    endif
	    __too_tiny = _S_concat_and_set_balanced(__forest[__i], __too_tiny);
	    __forest[__i]->_M_unref_nonnil();
	    __forest[__i] = 0;
	}
    }
    {
#	ifndef __GC
	  _Self_destruct_ptr __old(__too_tiny);
#	endif
	__insertee = _S_concat_and_set_balanced(__too_tiny, __r);
    }
    // Too_tiny dead, and no longer included in refcount.
    // Insertee is live and included.
    __stl_assert(_S_is_almost_balanced(__insertee));
    __stl_assert(__insertee->_M_depth <= __r->_M_depth + 1);
    for (;; ++__i) {
	if (0 != __forest[__i]) {
#	    ifndef __GC
	      _Self_destruct_ptr __old(__insertee);
#	    endif
	    __insertee = _S_concat_and_set_balanced(__forest[__i], __insertee);
	    __forest[__i]->_M_unref_nonnil();
	    __forest[__i] = 0;
	    __stl_assert(_S_is_almost_balanced(__insertee));
	}
	__stl_assert(_S_min_len[__i] <= __insertee->_M_size);
	__stl_assert(__forest[__i] == 0);
	if (__i == _RopeRep::_S_max_rope_depth || 
	      __insertee->_M_size < _S_min_len[__i+1]) {
	    __forest[__i] = __insertee;
	    // refcount is OK since __insertee is now dead.
	    return;
	}
    }
}

template <class _CharT, class _Alloc>
_CharT
rope<_CharT,_Alloc>::_S_fetch(_RopeRep* __r, size_type __i)
{
    __GC_CONST _CharT* __cstr = __r->_M_c_string;

    __stl_assert(__i < __r->_M_size);
    if (0 != __cstr) return __cstr[__i]; 
    for(;;) {
      switch(__r->_M_tag) {
	case _RopeRep::_S_concat:
	    {
		_RopeConcatenation* __c = (_RopeConcatenation*)__r;
		_RopeRep* __left = __c->_M_left;
		size_t __left_len = __left->_M_size;

		if (__i >= __left_len) {
		    __i -= __left_len;
		    __r = __c->_M_right;
		} else {
		    __r = __left;
		}
	    }
	    break;
	case _RopeRep::_S_leaf:
	    {
		_RopeLeaf* __l = (_RopeLeaf*)__r;
		return __l->_M_data[__i];
	    }
	case _RopeRep::_S_function:
	case _RopeRep::_S_substringfn:
	    {
		_RopeFunction* __f = (_RopeFunction*)__r;
		_CharT __result;

		(*(__f->_M_fn))(__i, 1, &__result);
		return __result;
	    }
      }
    }
}

# ifndef __GC
// Return a uniquely referenced character slot for the given
// position, or 0 if that's not possible.
template <class _CharT, class _Alloc>
_CharT*
rope<_CharT,_Alloc>::_S_fetch_ptr(_RopeRep* __r, size_type __i)
{
    _RopeRep* __clrstack[_RopeRep::_S_max_rope_depth];
    size_t __csptr = 0;

    for(;;) {
      if (__r->_M_refcount > 1) return 0;
      switch(__r->_M_tag) {
	case _RopeRep::_S_concat:
	    {
		_RopeConcatenation* __c = (_RopeConcatenation*)__r;
		_RopeRep* __left = __c->_M_left;
		size_t __left_len = __left->_M_size;

		if (__c->_M_c_string != 0) __clrstack[__csptr++] = __c;
		if (__i >= __left_len) {
		    __i -= __left_len;
		    __r = __c->_M_right;
		} else {
		    __r = __left;
		}
	    }
	    break;
	case _RopeRep::_S_leaf:
	    {
		_RopeLeaf* __l = (_RopeLeaf*)__r;
		if (__l->_M_c_string != __l->_M_data && __l->_M_c_string != 0)
		    __clrstack[__csptr++] = __l;
		while (__csptr > 0) {
		    -- __csptr;
		    _RopeRep* __d = __clrstack[__csptr];
		    __d->_M_free_c_string();
		    __d->_M_c_string = 0;
		}
		return __l->_M_data + __i;
	    }
	case _RopeRep::_S_function:
	case _RopeRep::_S_substringfn:
	    return 0;
      }
    }
}
# endif /* __GC */

// The following could be implemented trivially using
// lexicographical_compare_3way.
// We do a little more work to avoid dealing with rope iterators for
// flat strings.
template <class _CharT, class _Alloc>
int
rope<_CharT,_Alloc>::_S_compare (const _RopeRep* __left, 
                                 const _RopeRep* __right)
{
    size_t __left_len;
    size_t __right_len;

    if (0 == __right) return 0 != __left;
    if (0 == __left) return -1;
    __left_len = __left->_M_size;
    __right_len = __right->_M_size;
    if (_RopeRep::_S_leaf == __left->_M_tag) {
	_RopeLeaf* __l = (_RopeLeaf*) __left;
	if (_RopeRep::_S_leaf == __right->_M_tag) {
	    _RopeLeaf* __r = (_RopeLeaf*) __right;
	    return lexicographical_compare_3way(
			__l->_M_data, __l->_M_data + __left_len,
			__r->_M_data, __r->_M_data + __right_len);
	} else {
	    const_iterator __rstart(__right, 0);
	    const_iterator __rend(__right, __right_len);
	    return lexicographical_compare_3way(
			__l->_M_data, __l->_M_data + __left_len,
			__rstart, __rend);
	}
    } else {
	const_iterator __lstart(__left, 0);
	const_iterator __lend(__left, __left_len);
	if (_RopeRep::_S_leaf == __right->_M_tag) {
	    _RopeLeaf* __r = (_RopeLeaf*) __right;
	    return lexicographical_compare_3way(
				   __lstart, __lend,
				   __r->_M_data, __r->_M_data + __right_len);
	} else {
	    const_iterator __rstart(__right, 0);
	    const_iterator __rend(__right, __right_len);
	    return lexicographical_compare_3way(
				   __lstart, __lend,
				   __rstart, __rend);
	}
    }
}

// Assignment to reference proxies.
template <class _CharT, class _Alloc>
_Rope_char_ref_proxy<_CharT, _Alloc>&
_Rope_char_ref_proxy<_CharT, _Alloc>::operator= (_CharT __c) {
    _RopeRep* __old = _M_root->_M_tree_ptr;
#   ifndef __GC
	// First check for the case in which everything is uniquely
	// referenced.  In that case we can do this destructively.
	_CharT* __ptr = _My_rope::_S_fetch_ptr(__old, _M_pos);
	if (0 != __ptr) {
	    *__ptr = __c;
	    return *this;
	}
#   endif
    _Self_destruct_ptr __left(
      _My_rope::_S_substring(__old, 0, _M_pos));
    _Self_destruct_ptr __right(
      _My_rope::_S_substring(__old, _M_pos+1, __old->_M_size));
    _Self_destruct_ptr __result_left(
      _My_rope::_S_destr_concat_char_iter(__left, &__c, 1));

#   ifndef __GC
      __stl_assert(__left == __result_left || 1 == __result_left->_M_refcount);
#   endif
    _RopeRep* __result =
		_My_rope::_S_concat(__result_left, __right);
#   ifndef __GC
      __stl_assert(1 <= __result->_M_refcount);
      _RopeRep::_S_unref(__old);
#   endif
    _M_root->_M_tree_ptr = __result;
    return *this;
}

template <class _CharT, class _Alloc>
inline _Rope_char_ref_proxy<_CharT, _Alloc>::operator _CharT () const
{
    if (_M_current_valid) {
	return _M_current;
    } else {
        return _My_rope::_S_fetch(_M_root->_M_tree_ptr, _M_pos);
    }
}
template <class _CharT, class _Alloc>
_Rope_char_ptr_proxy<_CharT, _Alloc>
_Rope_char_ref_proxy<_CharT, _Alloc>::operator& () const {
    return _Rope_char_ptr_proxy<_CharT, _Alloc>(*this);
}

template <class _CharT, class _Alloc>
rope<_CharT, _Alloc>::rope(size_t __n, _CharT __c,
			   const allocator_type& __a)
: _Base(__a)
{
    rope<_CharT,_Alloc> __result;
    const size_t __exponentiate_threshold = 32;
    size_t __exponent;
    size_t __rest;
    _CharT* __rest_buffer;
    _RopeRep* __remainder;
    rope<_CharT,_Alloc> __remainder_rope;

    if (0 == __n)
      return;
    
    __exponent = __n / __exponentiate_threshold;
    __rest = __n % __exponentiate_threshold;
    if (0 == __rest) {
	__remainder = 0;
    } else {
	__rest_buffer = _Data_allocate(_S_rounded_up_size(__rest));
	uninitialized_fill_n(__rest_buffer, __rest, __c);
	_S_cond_store_eos(__rest_buffer[__rest]);
	__STL_TRY {
	    __remainder = _S_new_RopeLeaf(__rest_buffer, __rest, __a);
        }
	__STL_UNWIND(_RopeRep::__STL_FREE_STRING(__rest_buffer, __rest, __a))
    }
    __remainder_rope._M_tree_ptr = __remainder;
    if (__exponent != 0) {
	_CharT* __base_buffer =
	  _Data_allocate(_S_rounded_up_size(__exponentiate_threshold));
	_RopeLeaf* __base_leaf;
	rope __base_rope;
	uninitialized_fill_n(__base_buffer, __exponentiate_threshold, __c);
	_S_cond_store_eos(__base_buffer[__exponentiate_threshold]);
	__STL_TRY {
          __base_leaf = _S_new_RopeLeaf(__base_buffer,
                                        __exponentiate_threshold, __a);
        }
	__STL_UNWIND(_RopeRep::__STL_FREE_STRING(__base_buffer, 
	                                         __exponentiate_threshold, __a))
	__base_rope._M_tree_ptr = __base_leaf;
 	if (1 == __exponent) {
	  __result = __base_rope;
#         ifndef __GC
	    __stl_assert(2 == __result._M_tree_ptr->_M_refcount);
		// One each for base_rope and __result
#         endif
	} else {
	  // XXX what is power()?
	  __result = power(__base_rope, __exponent, _Concat_fn());
	}
	if (0 != __remainder) {
	  __result += __remainder_rope;
	}
    } else {
	__result = __remainder_rope;
    }
    _M_tree_ptr = __result._M_tree_ptr;
    _M_tree_ptr->_M_ref_nonnil();
}

template<class _CharT, class _Alloc>
  _CharT rope<_CharT,_Alloc>::_S_empty_c_str[1];

# ifdef __STL_PTHREADS
    template<class _CharT, class _Alloc>
    pthread_mutex_t 
    rope<_CharT,_Alloc>::_S_swap_lock = PTHREAD_MUTEX_INITIALIZER;
# endif

template<class _CharT, class _Alloc>
const _CharT* rope<_CharT,_Alloc>::c_str() const {
    if (0 == _M_tree_ptr) {
        _S_empty_c_str[0] = _S_eos((_CharT*)0);  // Possibly redundant,
					     // but probably fast.
        return _S_empty_c_str;
    }
    __GC_CONST _CharT* __old_c_string = _M_tree_ptr->_M_c_string;
    if (0 != __old_c_string) return(__old_c_string);
    size_t __s = size();
    _CharT* __result = _Data_allocate(__s + 1);
    _S_flatten(_M_tree_ptr, __result);
    __result[__s] = _S_eos((_CharT*)0);
#   ifdef __GC
	_M_tree_ptr->_M_c_string = __result;
#   else
      if ((__old_c_string = 
             _S_atomic_swap(&(_M_tree_ptr->_M_c_string), __result)) != 0) {
	// It must have been added in the interim.  Hence it had to have been
	// separately allocated.  Deallocate the old copy, since we just
	// replaced it.
	destroy(__old_c_string, __old_c_string + __s + 1);
	_Data_deallocate(__old_c_string, __s + 1);
      }
#   endif
    return(__result);
}

template<class _CharT, class _Alloc>
const _CharT* rope<_CharT,_Alloc>::replace_with_c_str() {
    if (0 == _M_tree_ptr) {
        _S_empty_c_str[0] = _S_eos((_CharT*)0);
        return _S_empty_c_str;
    }
    __GC_CONST _CharT* __old_c_string = _M_tree_ptr->_M_c_string;
    if (_RopeRep::_S_leaf == _M_tree_ptr->_M_tag && 0 != __old_c_string) {
	return(__old_c_string);
    }
    size_t __s = size();
    _CharT* __result = _Data_allocate(_S_rounded_up_size(__s));
    _S_flatten(_M_tree_ptr, __result);
    __result[__s] = _S_eos((_CharT*)0);
    _M_tree_ptr->_M_unref_nonnil();
    _M_tree_ptr = _S_new_RopeLeaf(__result, __s, get_allocator());
    return(__result);
}

// Algorithm specializations.  More should be added.

#ifndef _MSC_VER
// I couldn't get this to work with VC++
template<class _CharT,class _Alloc>
void
_Rope_rotate(_Rope_iterator<_CharT,_Alloc> __first,
              _Rope_iterator<_CharT,_Alloc> __middle,
              _Rope_iterator<_CharT,_Alloc> __last)
{
    __stl_assert(__first.container() == __middle.container()
                 && __middle.container() == __last.container());
    rope<_CharT,_Alloc>& __r(__first.container());
    rope<_CharT,_Alloc> __prefix = __r.substr(0, __first.index());
    rope<_CharT,_Alloc> __suffix = 
      __r.substr(__last.index(), __r.size() - __last.index());
    rope<_CharT,_Alloc> __part1 = 
      __r.substr(__middle.index(), __last.index() - __middle.index());
    rope<_CharT,_Alloc> __part2 = 
      __r.substr(__first.index(), __middle.index() - __first.index());
    __r = __prefix;
    __r += __part1;
    __r += __part2;
    __r += __suffix;
}

#if !defined(__GNUC__)
// Appears to confuse g++
inline void rotate(_Rope_iterator<char,__STL_DEFAULT_ALLOCATOR(char)> __first,
                   _Rope_iterator<char,__STL_DEFAULT_ALLOCATOR(char)> __middle,
                   _Rope_iterator<char,__STL_DEFAULT_ALLOCATOR(char)> __last) {
    _Rope_rotate(__first, __middle, __last);
}
#endif

# if 0
// Probably not useful for several reasons:
// - for SGIs 7.1 compiler and probably some others,
//   this forces lots of rope<wchar_t, ...> instantiations, creating a
//   code bloat and compile time problem.  (Fixed in 7.2.)
// - wchar_t is 4 bytes wide on most UNIX platforms, making it unattractive
//   for unicode strings.  Unsigned short may be a better character
//   type.
inline void rotate(
		_Rope_iterator<wchar_t,__STL_DEFAULT_ALLOCATOR(char)> __first,
                _Rope_iterator<wchar_t,__STL_DEFAULT_ALLOCATOR(char)> __middle,
                _Rope_iterator<wchar_t,__STL_DEFAULT_ALLOCATOR(char)> __last) {
    _Rope_rotate(__first, __middle, __last);
}
# endif
#endif /* _MSC_VER */

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma reset woff 1174
#endif

__STL_END_NAMESPACE

// Local Variables:
// mode:C++
// End:
