//------------------------------------------------------------------------------
// r_array v3 - 3/4/2026, wrzeczak
//
// This header defines a dynamic array structure called an `r_array`.
//
// # CONFIGURATION POUND-DEFINES
// RA_NO_CRASH_ON_OVERFLOW - By default, things that would call illegal memory
//                           access will crash the program. Defining this will
//                           cause those functions to instead return `NULL`.
//
// RA_SILENT - This will silence any non-crashing messages. Some messages will
//             still print on crash regardless; maybe this can be changed.
//
// RA_DEFAULT_ARRAY_CAPACITY - The default number of elements in an array. 512
//                             unless otherwise specified.
//
// RA_INTERNAL_POINTER_SPACE_SIZE - The default space in bytes allocated to my
//                                  internal dynamic memory system. This is by
//                                  by default 8192 bytes.
//
// # USEFUL FUNCTION-LIKE MACROS
// RA_STATIC_INIT(type_pair) - This is useful for initializing an array before
//                             allocating its memory i.e. for creating globals
//                             or whatever. Necessary because ra_type is const
//                             so it cannot be modified or zeroed.

//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
//gen 1 "Include headers for types."
#include "#define RAYMATH_IMPLEMENTATION
<raymath.h>" // for Vector2

//----------------------------
// type-pair definition

typedef struct {
    const char * typename;
    const int size; // this is signed, and sign indicates pointer-ness (positive for no, negative for yes)
    // i do this instead of using a boolean because a) types almost never need the full breadth of size_t (so this reduces struct size) and b) it avoid the alignment waste of a single boolean
} _ra_type;

/**
 * r_array type pair (implicitly const).
 * 
 * @param typename `const char *`, string representation of the type held.
 * @param size `const int`, size of type held. If negative, this is a pointer type (i.e. a string).
 */
typedef const _ra_type ra_type;

#define DEFINE_RA_TYPE(typename) (ra_type) { #typename, (int) sizeof(typename) }
#define DEFINE_RA_PNT_TYPE(typename) (ra_type) { #typename, -1 * (int) sizeof(typename) }

ra_type RA_INT = DEFINE_RA_TYPE(int);
ra_type RA_STR = DEFINE_RA_PNT_TYPE(char *);
//gen 2 "Define types."
ra_type RA_DOUBLE = DEFINE_RA_TYPE(double); // from stdlib
ra_type RA_VECTOR2 = DEFINE_RA_TYPE(Vector2); // from #define RAYMATH_IMPLEMENTATION
<raymath.h>

//----------------------------
// arena definitions

/**
 * r_array arena.
 * 
 * @param root `void *`, allocated pointer to a block of memory of size `capacity`.
 * @param position `void *`, the end of allocated space in that memory.
 * @param capacity `size_t`, the size, in bytes, of the memory block.
 */
typedef struct {
    void * root;
    void * position;
    size_t capacity;
} aa_arena;

aa_arena ________________internal_ra_space_for_pointers;
bool internal_ra_space_for_pointers_has_been_initialized = false;
#ifndef RA_INTERNAL_POINTER_SPACE_SIZE
#define RA_INTERNAL_POINTER_SPACE_SIZE 8192
#endif

/**
 * Create an arena.
 * 
 * @param capacity The number of bytes to allocate.
 * @return An arena with an allocation of the requested size.
 */
aa_arena aa_create(size_t capacity);

/**
 * Free the memory assigned to an arena.
 * 
 * @param arena The arena to destroy.
 */
void aa_destroy(aa_arena arena);

/**
 * Allocate memory within an arena. If `RA_NO_CRASH_ON_OVERFLOW` is defined, then this will return `NULL` (and print an error message) when the arena runs out of space. Otherwise, this crash the program.
 * 
 * @param arena The arena to allocate within.
 * @param value A pointer to the value to place in memory.
 * @param size The amount of memory to allocate.
 * @return A pointer to the value allocated in the arena. If `RA_CRASH_ON_OVERFLOW` is **not** defined, this will return `NULL` when the arena is out of space.
 */
void * aa_alloc(aa_arena * arena, void * value, size_t size);

//----------------------------
// array defintions

/**
 * Dynamic array by Wrzeczak. I don't remember why I prefixed it 'r'.
 * 
 * @param memory `aa_arena`, arena that stores the values.
 * @param type `ra_type`, the type pair of the arena.
 * @param count `size_t`, number of elements in the arena.
 */
typedef struct {
    aa_arena memory;
    ra_type type;
    size_t count;
} r_array;

#ifndef RA_DEFAULT_ARRAY_CAPACITY
#define RA_DEFAULT_ARRAY_CAPACITY 512
#endif

/**
 * Create an r_array. This allocates the necessary memory, which must be destroyed with `ra_destroy()`. By default an array can hold 512 elements. If you need more than this, see `ra_create_cap()`.
 * 
 * @param type_pair The type of the array.
 * @return The new array.
 */
inline r_array ra_create(ra_type type_pair);

// use this to defer initialization for later i.e. to create global-variable arrays
#define RA_STATIC_INIT(type_pair) (r_array) { .type = type_pair, .count = 0, .memory = (aa_arena) { 0 } };
// after calling this, initialize the array's memory: "ra.memory = aa_alloc(RA_DEFAULT_ARRAY_CAPACITY * abs(ra.type.size))"

/**
 * Create an r_array. This allocates the necessary memory, which must be destroyed with `ra_destroy()`.
 * 
 * @param type_pair The type of the array.
 * @param count The prospective amount of elements in the array. Creates an arena of size `count * (size_t) abs(type_pair.size)`.
 * @return The new array.
 */
inline r_array ra_create_cap(ra_type type_pair, size_t count);

/**
 * Free the memory associated with an array.
 * 
 * @param ra The array to be destroyed.
 */
void ra_destroy(r_array * ra);

/**
 * Append an element to an array.
 * 
 * @param ra The array to append to.
 * @param ... The item to append. Will be interpreted as the type stored in `ra`'s type pair.
 * @return Pointer to the element appended, `NULL` if appending went wrong.
 */
void * ra_append(r_array * ra, ...);

/**
 * Access the values of an array. Performs bounds checking and returns `NULL` on failure. Negative values count from the back, like in Python, such that `-1` points to the last element of the array, `-2` to the second-to-last, etc.
 * 
 * @param ra The array to access.
 * @param idx The index to access at.
 * @return A pointer to the value in the array's internal memory. If access fails (i.e. out-of-bounds), then the program will crash. If `RA_NO_CRASH_ON_OVERFLOW` is defined, then this will return `NULL`.
 */
void * ra_get(r_array * ra, int idx);

/**
 * Access the values of an array. No bounds checking. Negative values count from the back, like in Python, such that `-1` points to the last element of the array, `-2` to the second-to-last, etc.
 * 
 * @param ra The array to access.
 * @param idx The index to access at.
 * @return A pointer to the value in the array's internal memory. No bounds checking, so no guarantees for correctness/not segfaulting.
 */
void * ra_get_unsafe(r_array * ra, int idx);

/**
 * Remove the last element of an array. This function is affected by `RA_NO_CRASH_ON_OVERFLOW`.
 * 
 * @param ra The array to pop from.
 * @return The value popped. This just decrements count and moves the arena pointer back, so the data is still there until overwritten by an append.
 */
void * ra_pop(r_array * ra);

/**
 * Remove an element of an array. Shift all the elements behind it back. This function is affected by `RA_NO_CRASH_ON_OVERFLOW`.
 * 
 * @param ra The array to pop from.
 * @return The value popped. This function moves this value to the back of the array, then calls `ra_pop()`.
 */
void * ra_pop_at(r_array * ra, int idx);


/**
 * Get a slice of an array. This imitates Python's slice syntax, so negative values are permitted like in `ra_get()`.
 * 
 * @param ra The array to get a slice of.
 * @param start_idx The beginning of the slice, **inclusive**.
 * @param end_idx The end of the slice, **inclusive**.
 * @return On failure, returns the passed array. On success, returns an array with an internal arena which **points at the passed array's memory**. This does **not** create a new array (i.e. no allocation of new memory, `ra_destroy()` not necessary); see `ra_copy()` for that. The array is completely filled, so (safe) appends will not work. In my opinion you should treat slices as read-only. It might be worth investigating if creating a separate type for slices would help this.
 */
r_array ra_slice(r_array * ra, int start_idx, int end_idx);

/**
 * Make a copy of an array. This allocates new memory, and so the new array must be destroyed. To copy a slice, compose this function with `ra_slice()`.
 * 
 * @param ra The array to copy.
 * @return A new array with the same data as `ra`, but in a new allocation.
 */
r_array ra_copy(r_array * ra);

/**
 * Make a new array which uniquely contains all of the members of another array. Like `ra_copy()`, this creates a new array i.e. allocates memory. Like `ra_slice()`, this array that is returned is a non-resizeable array. This is kind of like Python `list(set(a_list))`.
```c
[ 1, 1, 2, 3, 5, 5, 7, 8, 5 ] -> [ 1, 2, 3, 5, 7, 8 ]
```
 * 
 * @param ra The array whose elements to copy. 
 * @return A new (i.e. with new memory allocation) non-resizeable array in which all the elements of `ra` are uniquely represented (i.e. each one appears exactly once). 
 */
r_array ra_copy_uniques(r_array * ra);

/**
 * Check if a value is a member of an array.
 * 
 * @param ra The array to check in.
 * @param ... The value to check.
 * @return Whether or not the value is in the array. To get its index, see `ra_member_at()`.
 */
bool ra_member_of(r_array * ra, ...);

/**
 * Check where a value is in an array.
 * 
 * @param ra The array to check in.
 * @param ... The value to check.
 * @return `-1` if not in the array; otherwise, the index of the member in the array. This does not check for duplicates, so if those are present it will return the first matching value it finds.
 */
int ra_member_at(r_array * ra, ...);

/**
 * Print an array with a given format string.
```c
ra_print(&ra, "%d");
> printf("[ %d, %d, %d, %d ]", ra[0], ra[1]...);
```
 * @param ra The array to print.
 */
void ra_printf(r_array * ra, const char * format_string);

/**
 * A function which takes a value from an array and returns a string representation of it.
 * 
 * @param value A pointer to the value to be printed.
 * @param format_string The format string to print. This is intended to be used for structures where `ra_printf()` is not granular enough. So, for a two membered-struct, you would write a printer which takes a format string that has two format specifiers.
 * @return A string representation of the value. `NULL` on failure is acceptable.
 */
typedef char * (*ra_value_printer)(void * value, char * format_string);

/**
 * Print an array with a given format string generated by a given formatter.
 * @param ra The array to print.
 * @param printer A function which returns a string with the values formatted. This is useful for structs where `ra_printf` is not granular enough.
 */
void ra_printf_f(r_array * ra, char * format_string, ra_value_printer printer);

//------------------------------------------------------------------------------

aa_arena aa_create(size_t capacity) {
    aa_arena output = (aa_arena) { malloc(capacity), NULL, capacity };
    output.position = output.root;
    memset(output.root, 0, capacity);
    return output;
}

void aa_destroy(aa_arena arena) {
    free(arena.root);
}

void * aa_alloc(aa_arena * arena, void * value, size_t size) {
    #ifndef RA_NO_CRASH_ON_OVERFLOW
        if((arena->capacity - (arena->position - arena->root)) < size) {
            // this print is not silenceable because it crashes. maybe this will be revised in future.
            printf("aa_alloc: Attempted to allocate %zu bytes in arena <%p> (cap: %zu), but there were %zu bytes of space left!\nUndefine RA_CRASH_ON_OVERFLOW to prevent crashing at overflow, and return NULL instead.\n", size, arena, arena->capacity, (arena->capacity - (arena->position - arena->root)));
            exit(80085);
        }
    #else
        if((arena->capacity - (arena->position - arena->root)) < size) {
            #ifndef RA_SILENT
            printf("aa_alloc: (RA_NO_CRASH_ON_OVERFLOW) Attempted to allocate %zu bytes in arena <%p> (cap. %zu), but there were %zu bytes of space left!\n", size, arena, arena->capacity, (arena->capacity - (arena->position - arena->root)));
            #endif
            return NULL;
        }
    #endif

    void * output = arena->position;

    memcpy(arena->position, value, size);
    arena->position += size;

    return output;
}

r_array ra_create(ra_type type_pair) {
    return ra_create_cap(type_pair, RA_DEFAULT_ARRAY_CAPACITY);
}

r_array ra_create_cap(ra_type type_pair, size_t count) {
    return (r_array) { .memory = aa_create(count * (size_t) abs(type_pair.size)), .count = 0, .type = type_pair };
}

void ra_destroy(r_array * ra) {
    aa_destroy(ra->memory);
}

void * ra_append(r_array * ra, ...) {
    va_list args;
    va_start(args, ra);

    // do not init memory unless it's needed
    if(!internal_ra_space_for_pointers_has_been_initialized && (ra->type.size < 0)) {
        ________________internal_ra_space_for_pointers = aa_create(RA_INTERNAL_POINTER_SPACE_SIZE);
        internal_ra_space_for_pointers_has_been_initialized = true;
    }

    #define RA_APPEND_TYPE(pair, _type) \
        if(strcmp(#_type, ra->type.typename) == 0) { \
            _type value = va_arg(args, _type); \
            ra->count++; \
            return aa_alloc(&ra->memory, &value, pair.size); \
        }
    
    #define RA_APPEND_PNT_TYPE(pair, _type) \
        if(strcmp(#_type, ra->type.typename) == 0) { \
            _type value = va_arg(args, _type); \
            ra->count++; \
            void * allocated_value = aa_alloc(&________________internal_ra_space_for_pointers, value, strlen(value) + 1); \
            return aa_alloc(&ra->memory, &allocated_value, -1 * pair.size); \
        }

    RA_APPEND_TYPE(RA_INT, int);
    RA_APPEND_PNT_TYPE(RA_STR, char *);
    //gen 3 "Append types."
	RA_APPEND_TYPE(RA_DOUBLE, double);
	RA_APPEND_TYPE(RA_VECTOR2, Vector2);

    #ifndef RA_SILENT
    printf("ra_append: Append failed!\n");
    #endif
    return NULL; // no overflow-esque checking here because in reality, if this somehow fails, aa_alloc will probably fail...
}

void * ra_get(r_array * ra, int idx) {
    #ifndef RA_NO_CRASH_ON_OVERFLOW
    if((idx >= (int) ra->count) || (idx < (-1 * (int) ra->count))) {
        // this print is not silenceable because it crashes. maybe this will be revised in future.
        printf("ra_get: Attempted to access element %d (%d) of ra <%p>, which only contains %zu elements.\n", idx, (idx < 0) ? (int) ra->count + idx : idx, ra, ra->count);
        exit(80085);
    }
    #else
    if((idx >= (int) ra->count) || (idx < (-1 * (int) ra->count))) {
        #ifndef RA_SILENT
        printf("ra_get: (RA_NO_CRASH_ON_OVERFLOW) Overflow access attempted at element %d (%d) of ra <%p>, which only has %zu elements.\n NULL was returned; you might still get a segfault.\n", idx, (idx < 0) ? (int) ra->count + idx : idx, ra, ra->count);
        #endif
        return NULL;
    }
    #endif

    return ra_get_unsafe(ra, idx);
}

void * ra_get_unsafe(r_array * ra, int idx) {
    if(idx >= 0) return (ra->memory.root + (idx * abs(ra->type.size)));
    else return (ra->memory.root + (ra->count + idx) * abs(ra->type.size));
}

void * ra_pop(r_array * ra) {
    if(ra->count == 0) {
        #ifndef RA_NO_CRASH_ON_OVERFLOW
            printf("ra_pop: Tried to pop value of ra <%p>, but this array is empty!\n", ra);
            exit(80085);
        #else
            #ifndef RA_SILENT
            printf("ra_pop: (RA_NO_CRASH_ON_OVERFLOW) Tried to pop value of ra <%p>, but this array is empty!\nNULL was returned; you might still get a segfault.\n", ra);
            #endif
            return NULL;
        #endif
    }

    void * output = ra_get(ra, -1); 
    ra->count--;
    ra->memory.position -= ra->type.size;
    return output;
}

void * ra_pop_at(r_array * ra, int idx) {
    if(ra->count == 0) {
        #ifndef RA_NO_CRASH_ON_OVERFLOW
            printf("ra_pop_at: Tried to pop value of ra <%p> at index %d, but this array is empty!\n", ra, idx);
            exit(80085);
        #else
            #ifndef RA_SILENT
            printf("ra_pop_at: (RA_NO_CRASH_ON_OVERFLOW) Tried to pop value of ra <%p> at index %d, but this array is empty!\nNULL was returned; you might still get a segfault.\n", ra, idx);
            #endif
            return NULL;
        #endif
    }

    unsigned int actual_idx = (idx >= 0) ? idx : (ra->count + idx);

    if(actual_idx >= ra->count) {
        #ifndef RA_NO_CRASH_ON_OVERFLOW
            printf("ra_pop_at: Tried to pop value of ra <%p> at index %d (%zu), but this array only has %zu elements!\n", ra, idx, actual_idx, ra->count);
            exit(80085);
        #else
            #ifndef RA_SILENT
            printf("ra_pop_at: Tried to pop value of ra <%p> at index %d (%zu), but this array only has %zu elements!\nNULL was returned; you might still get a segfault.\n", ra, idx, actual_idx, ra->count);
            #endif
            return NULL;
        #endif
    }

    unsigned char back_buffer[ra->type.size];
    memcpy(back_buffer, ra->memory.root + (actual_idx * ra->type.size), ra->type.size);
    unsigned char rest_buffer[(ra->count - actual_idx) + 1];
    memcpy(rest_buffer, ra->memory.root + ((actual_idx + 1) * ra->type.size), ra->type.size * ((ra->count - actual_idx) - 1));

    memcpy(ra->memory.root + (actual_idx * ra->type.size), rest_buffer, ra->type.size * ((ra->count - actual_idx) + 1));
    memcpy(ra->memory.root + ((ra->count - 1) * ra->type.size), back_buffer, ra->type.size);

    return ra_pop(ra);
}

r_array ra_slice(r_array * ra, int start_idx, int end_idx) {
    unsigned int actual_start = (start_idx >= 0) ? start_idx : (ra->count + start_idx);
    unsigned int actual_end = (end_idx >= 0) ? end_idx : (ra->count + end_idx);

    if(actual_start >= actual_end) {
        printf("ra_slice: Attempted to slice array <%p> from %d (%zu) to %d (%zu), which is not allowed.\nThe array passed was returned.\n", ra, start_idx, actual_start, end_idx, actual_end);
        return *ra;
    }

    return (r_array) { .count = actual_end - actual_start + 1, 
                       .memory = (aa_arena) { .root = ra->memory.root + (abs(ra->type.size) * actual_start), 
                                              .position = ra->memory.root + (abs(ra->type.size) * (actual_end + 1)), 
                                              .capacity = abs(ra->type.size) * (actual_end - actual_start + 1) }, 
                       .type = ra->type };
}

r_array ra_copy(r_array * ra) {
    r_array output = { .count = ra->count, .type = ra->type, .memory = (aa_arena) { 0 } };

    output.memory = aa_create(ra->memory.capacity);
    output.memory.position += (ra->type.size * ra->count);
    memcpy(output.memory.root, ra->memory.root, ra->memory.capacity);

    return output;
}

r_array ra_copy_uniques(r_array * ra) {
    r_array uniques = ra_create_cap(ra->type, ra->count);

    for(size_t i = 0; i < ra->count; i++) {
        if(!ra_member_of(&uniques, ra_get(ra, i))) {
            ra_append(&uniques, ra_get(ra, i));
        }
    }
    
    r_array uniques_as_slice = ra_slice(&uniques, 0, -1);
    return ra_copy(&uniques_as_slice); // use slice and copy here to create a non-resizeable array
}

bool ra_member_of(r_array * ra, ...) {
    va_list args;
    va_start(args, ra);
    void * v = va_arg(args, void *);

    return (ra_member_at(ra, v) >= 0);
}

int ra_member_at(r_array * ra, ...) {
    if(ra->count == 0) return -1;

    va_list args;
    va_start(args, ra);

    #define RA_MEMBER_AT_TYPE(pair, _type) \
        if(strcmp(#_type, ra->type.typename) == 0) { \
            _type value = va_arg(args, _type); \
            for(size_t i = 0; i < ra->count; i++) { \
                if(value == *(_type *) ra_get(ra, i)) return i; \
            } \
        }
    
    #define RA_MEMBER_AT_TYPE_CMP(pair, _type, cmp) \
        if(strcmp(#_type, ra->type.typename) == 0) { \
            _type value = va_arg(args, _type); \
            for(size_t i = 0; i < ra->count; i++) { \
                if(cmp(value, *(_type *) ra_get(ra, i)) == 0) return i; \
            } \
        }
    
    RA_MEMBER_AT_TYPE(RA_INT, int);
    RA_MEMBER_AT_TYPE_CMP(RA_STR, char *, strcmp);
    //gen 4 "Get members."
	RA_MEMBER_AT_TYPE(RA_DOUBLE, double);
	RA_MEMBER_AT_TYPE_CMP(RA_VECTOR2, Vector2, Vector2Equals);
    
    return -1;
}

void ra_printf(r_array * ra, const char * format_string) {
    if(ra->count == 0) {
        printf("[ empty ]\n");
        return;
    }

    printf("[ ");
    char comma_format_string[256];
    memset(comma_format_string, 0, 256);
    strncpy(comma_format_string, ", ", 2);
    strncpy(comma_format_string + 2, format_string, strlen(format_string));

    #define RA_PRINTF_TYPE(pair, _type) \
        if(strcmp(pair.typename, ra->type.typename) == 0) { \
            _type value = *(_type *) ra_get(ra, 0); \
            printf(format_string, value); \
            for(size_t i = 1; i < ra->count; i++) { \
                _type value = *(_type *) ra_get(ra, i); \
                printf(comma_format_string, value); \
            } \
            printf(" ]\n"); \
            return; \
        }
    
    RA_PRINTF_TYPE(RA_INT, int);
    RA_PRINTF_TYPE(RA_STR, char *);
    //gen 5 "Print values."
	RA_PRINTF_TYPE(RA_DOUBLE, double);
	RA_PRINTF_TYPE(RA_VECTOR2, Vector2);
}

void ra_printf_f(r_array * ra, char * format_string, ra_value_printer printer) {
    if(ra->count == 0) {
        printf("[ empty ]\n");
        return;
    }

    printf("[ ");

    char * value = printer(ra_get(ra, 0), format_string);
    printf("%s", value);

    if(ra->count == 1) {
        printf(" ]\n");
        return;
    }

    for(size_t i = 1; i < ra->count; i++) {
        char * value = printer(ra_get(ra, i), format_string);
        printf(", %s", value);
    }

    printf(" ]\n");
}