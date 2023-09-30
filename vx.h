// vx.h - v1.0 - vector library in C
// by Alice Uu, 2023/09/30
// http://github.com/auul/vx.h
//
// License: Public Domain (www.unlicense.org)
// ==========================================
// This is free and unencumbered software released into the public domain.
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute
// this software, either in source code form or as a compiled binary, for any
// purpose, commercial or non-commercial, and by any means.
//
// In jurisdictions that recognize copyright laws, the author or authors of this
// software dedicate any and all copyright interest in the software to the
// public domain. We make this dedication for the benefit of the public at large
// and to the detriment of our heirs and successors. We intend this dedication
// to be an overt act of relinquishment in perpetuity of all present and future
// rights to this software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ==========================================
//
// Linking:
//      This is a single-header library. To use, add
//              #define VX_IMPLEMENT
//      to ONE .c file, prior to including the header.
//
// Usage:
//      The vectors produced by vx.h appear as plain heap-allocated arrays of
//      any type, and can be accessed and modified as such. Each vector holds
//      meta-information in a tag stored just prior to the returned pointer.
//      Altering the size of the vector should be performed entirely via the
//      library functions; otherwise, the contents of the vector may be altered
//      in any other way directly.
//
//      Most of the functionality is accessed via macros, to allow for vectors
//      of any type and to simplify the API. These macros alter the vector
//      in-place and generally return a bool to indicate success; thus, they can
//      be easily chained together using a series of && statements.
//
// API:
// ====
// (TYPE *) vx_new(TYPE, size_t count, void (*unit_free)(void *))
//      Creates a new vector of 'TYPE' (which can be any defined type of any
//      size), where 'count' is the initial number of units, and 'unit_free' is
//      an optional function for freeing individual units upon destruction (if
//      units do not need to be freed, set this equal to NULL)
// void vx_free(void *vx)
//      Frees the vector 'vx' and sets it to NULL, including freeing any
//      dynamically allocated members if unit_free() is set.
// int vx_count(void *vx)
//      Returns the number of units currently stored in a vector.
// bool vx_reserve(void *vx, size_t new_capacity)
//      Attempts to allocate the requested 'new_capacity' in terms of units for
//      use by the vector 'vx'. Returns a bool indicating success or failure.
// bool vx_grow(void *vx, size_t grow_by)
//      Attempts to grow the vector by 'grow_by' units, which will be zeroed
//      out. Returns a bool indicating success or failure.
// bool vx_push(void *vx, TYPE value)
//      Pushes a single value to the end of the vector, and returns a bool
//      indicating success or failure. This value must be of the same 'TYPE' as
//      the vector itself.
// bool vx_append(void *vx, void *src, size_t count)
//      Appends 'count' members of an array at 'src' to the end of vector 'vx'.
//      Returns a bool indicating success or failure.
// bool vx_shift(void *vx, size_t index, ptrdiff_t shift)
//      If 'shift' is negative, removes 'shift' units prior to 'index' from the
//      vector 'vx'; if positive, inserts 'shift' empty units prior to 'index'.
//      Returns bool indicating success or failure.
// bool vx_insert(void *vx, size_t index, TYPE value)
//      Inserts a single value prior to 'index' in the vector 'vx', shifting all
//      following units forward by 1. Returns a bool indicating success or
//      failure.
// bool vx_emplace(void *dest, size_t index, void *src, size_t count)
//      Inserts a series of 'count' units from array 'src' into the vector
//      'dest', prior to 'index', shifting all existing units forward to
//      compensate. Returns a bool indicating success or failure.
// bool vx_shrink(void *vx)
//      Removes any unused capacity allocated for the vector 'vx'. Returns a
//      bool indicating success or failure.
// char *vx_str_new(const char *fmt, ...)
//      Creates a string vector constructed using text formatted in the same
//      manner as printf()
// bool vx_str_push(char *vx, char c)
//      Pushes a single character 'c' to the end of string vector 'vx'. Returns
//      a bool indicating success or failure.
// bool vx_str_append(char *vx, const char *fmt, ...)
//      Appends a string constructed using text formatted in the same manner as
//      printf() to the end of the string vector 'vx'. Returns a bool indicating
//      success or failure.
// bool vx_str_emplace(char *vx, size_t index, const char *fmt, ...)
//      Inserts a string constructed using text formatted in the same manner as
//      printf() into the string vector 'vx', prior to 'index'. Returns a bool
//      indicating success or failure.

#ifndef VX_H
#define VX_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef VX_USER_ERRORS
#include <errno.h>
#endif

#ifndef VX_CHUNK_COUNT
#define VX_CHUNK_COUNT 16
#endif

struct vx_tag {
	void (*unit_free)(void *);
	size_t        unit;
	size_t        capacity;
	size_t        count;
	unsigned char data[];
};

#define vx_new(type, count, unit_free) \
	(type *)vx_new_(sizeof(type), count, unit_free)
#define vx_tag(vx) ((struct vx_tag *)(vx)-1)
#define vx_count(vx) (int)vx_tag(vx)->count
#define vx_free(vx) vx_free_((void **)&vx)
#define vx_reserve(vx, new_capacity) vx_reserve_((void **)&vx, new_capacity)
#define vx_grow(vx, grow_by) vx_grow_((void **)&vx, grow_by)
#define vx_push(vx, value) \
	(vx_grow(vx, 1) && ((vx[vx_count(vx) - 1] = value) ? true : true))
#define vx_append(vx, src, capacity) vx_append_((void **)&vx, src, capacity)
#define vx_shift(vx, index, shift) vx_shift_((void **)&vx, index, shift)
#define vx_insert(vx, index, value) \
	(vx_shift(vx, index, 1) && (vx[index] = value))
#define vx_emplace(dest, index, src, count) \
	vx_emplace_((void **)&dest, index, src, count)
#define vx_shrink(vx) vx_shrink_((void **)&vx)
#define vx_str_push(vx, c) vx_str_push_(&vx, c)
#define vx_str_append(vx, ...) vx_str_append_(&vx, __VA_ARGS__)
#define vx_str_emplace(vx, ...) vx_str_emplace_(&vx, __VA_ARGS__)

#ifdef VX_IMPLEMENT

void *vx_new_(size_t unit, size_t count, void (*unit_free)(void *))
{
	struct vx_tag *tag = calloc(1, sizeof(struct vx_tag) + unit * count);
	if (!tag) {
#ifdef VX_USER_ERRORS
		perror(strerror(errno));
#endif
		return NULL;
	}

	tag->unit_free = unit_free;
	tag->unit      = unit;
	tag->capacity  = count;
	tag->count     = count;

	return tag->data;
}

bool vx_unit_nonempty(struct vx_tag *tag, size_t index)
{
	// This function checks if a given unit is non-empty (i.e. not all
	// zeros/NULL). This is necessary to prevent freeing zero/NULL units of
	// arbitrary size if unit_free() is set.

	for (size_t i = 0; i < tag->unit; i++) {
		if (tag->data[tag->unit * index + i]) {
			return true;
		}
	}

	return false;
}

void vx_free_(void **vx_p)
{
	if (!*vx_p) {
		return;
	}

	struct vx_tag *tag = vx_tag(*vx_p);
	*vx_p              = NULL;

	if (tag->unit_free) {
		for (size_t i = 0; i < tag->count; i++) {
			if (vx_unit_nonempty(tag, i)) {
				tag->unit_free(tag->data + tag->unit * i);
			}
		}
	}

	free(tag);
}

bool vx_reserve_(void **vx_p, size_t new_capacity)
{
	struct vx_tag *tag = vx_tag(*vx_p);

	if (new_capacity < tag->count) {
#ifdef VX_USER_ERRORS
		fprintf(stderr,
		        "Error resizing vector below current contents.\n");
#endif
		return false;
	}

	tag = realloc(tag, sizeof(struct vx_tag) + tag->unit * new_capacity);
	if (!tag) {
#ifdef VX_USER_ERRORS
		perror(strerror(errno));
#endif
		return false;
	}

	tag->capacity = new_capacity;

	return true;
}

bool vx_grow_(void **vx_p, size_t grow_by)
{
	struct vx_tag *tag        = vx_tag(*vx_p);
	size_t         prev_count = tag->count;

	if (tag->capacity < tag->count + grow_by) {
		if (!vx_reserve_(vx_p, tag->count + grow_by)) {
			return false;
		}
		tag = vx_tag(*vx_p);

		for (size_t i = prev_count; i < tag->count; i++) {
			for (size_t j = 0; j < tag->unit; j++) {
				tag->data[tag->unit * i + j] = 0;
			}
		}
	}

	tag->count += grow_by;

	return true;
}

bool vx_append_(void **vx_p, void *src, size_t count)
{
	if (!vx_grow_(vx_p, count)) {
		return false;
	}

	struct vx_tag *tag = vx_tag(*vx_p);

	memmove(tag->data + tag->unit * (tag->count - count),
	        src,
	        tag->unit * count);

	return true;
}

bool vx_shift_(void **vx_p, size_t index, ptrdiff_t shift)
{
	struct vx_tag *tag        = vx_tag(*vx_p);
	size_t         prev_count = tag->count;

	if (shift > 0) {
		if (!vx_grow_(vx_p, shift)) {
			return false;
		}
		tag = vx_tag(*vx_p);
	}

	memmove(tag->data + tag->unit * (index + shift),
	        tag->data + tag->unit * index,
	        tag->unit * (prev_count - index));

	if (shift < 0) {
		if (tag->unit_free) {
			for (size_t i = prev_count; i < tag->count; i++) {
				if (vx_unit_nonempty(tag, i)) {
					tag->unit_free(tag->data
					               + tag->unit * i);
				}
			}
		}

		tag->count += shift;
	} else if (shift > 0 && tag->unit_free) {
		for (size_t i = index; i < index + shift; i++) {
			for (size_t j = 0; j < tag->unit; j++) {
				tag->data[tag->unit * i + j] = 0;
			}
		}
	}

	return true;
}

bool vx_emplace_(void **dest_p, size_t index, void *src, size_t count)
{
	if (!vx_shift_(dest_p, index, count)) {
		return false;
	}

	struct vx_tag *tag = vx_tag(*dest_p);
	memmove(tag->data + tag->unit * index, src, tag->unit * count);

	return true;
}

bool vx_shrink_(void **vx_p)
{
	struct vx_tag *tag = vx_tag(*vx_p);

	tag = realloc(tag, sizeof(struct vx_tag) + tag->unit * tag->count);
	if (!tag) {
#ifdef VX_USER_ERRORS
		perror(strerror(errno));
#endif
		return false;
	}

	tag->capacity = tag->count;
	*vx_p         = tag->data;

	return true;
}

char *vx_str_new(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	char *str = vx_new(char, len + 1, NULL);
	if (!str) {
		return NULL;
	}

	va_start(args, fmt);
	vsnprintf(str, len + 1, fmt, args);
	va_end(args);

	return str;
}

bool vx_str_push_(char **vx_p, char c)
{
	if (!vx_grow_((void **)vx_p, 1)) {
		return false;
	}

	struct vx_tag *tag        = vx_tag(*vx_p);
	tag->data[tag->count - 2] = c;
	tag->data[tag->count - 1] = 0;

	return true;
}

bool vx_str_append_(char **vx_p, const char *fmt, ...)
{
	size_t  prev_count = vx_tag(*vx_p)->count;
	va_list args;

	va_start(args, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (!vx_grow_((void **)vx_p, len)) {
		return false;
	}

	struct vx_tag *tag = vx_tag(*vx_p);
	va_start(args, fmt);
	vsnprintf((char *)tag->data + prev_count - 1, len + 1, fmt, args);
	va_end(args);

	return true;
}

bool vx_str_emplace_(char **vx_p, size_t index, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (!vx_shift_((void **)vx_p, index, len)) {
		return false;
	}

	char c = (*vx_p)[index];

	va_start(args, fmt);
	(*vx_p)[index + vsprintf(*vx_p + index, fmt, args)] = c;
	va_end(args);

	return true;
}

#endif

#endif
