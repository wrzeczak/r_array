# r_array.h

A dynamic array library. Originally from [wrzlib](https://github.com/wrzeczak/wrzlib), I moved this to its own repository because it's grown beyond the single-header implementation I originally used.

### History + Theory

The first version of `ra` was a single-header library that could store ints and strings (ish) in a union value. This had some big annoying problems, and was also unergonomic (accessing members looked like `ra->members[i].i_value`). I reimplemented `ra` in a second version, but this had a problem also shared with the first one that strings require more dynamic memory allocation than normal (`int`) values do. In these first two versions, the size of elements in arrays wasn't really standard, because arrays were simply blocks of memory that strings were written into. So, instead of the element size of a string array being `sizeof(char *)`, it was the `strlen()` (give or take a few characters) of the longest string you intended to write. These both also relied on `realloc()`, which meant they had no size limitations, but pointers to their members were unreliable. I have tried to fix all three of these issues in this third version.

This is an API and not a set of macros (like Tsoding's `da_append()` system for dynamic arrays) because a) functions are easier to document, maintain, debug, and use (I leverage strongly VSCode's inbuilt half-support for Doxygen comments), b) function-like macros are really dubious to me (they shouldn't pretend to be functions, like `da_append()`; I myself violate this principle for convenience, but I have a much lower tolerance for it than some). Macros should be used for code generation, and I do this extensively in my code, but I try to limit as much as possible the presnece of macros in the surface of my APIs to be configurations and maybe convenient pseudo-compile-time functions (this has one such function).

The benefit of Tsoding's macro approach is that the type information of the values stored in the array are obscure to the macro; it's simply looking for a structure with the right fields. A dynamic array of any type can be created very simply by the user. This major benefit is one I have tried to accomodate with code generation.

### Theory, cont.

```c
typedef struct {
    aa_arena memory;
    ra_type type;
    size_t count;
} r_array;
```

The above is the structure `r_array`. Arrays store values in a memory block of a fixed size (`aa_arena`). It counts how many elements are in the array, and stores information on how to interpret the values in the memory block (`ra_type`).

```c
typedef struct {
    const char * typename;
    const int size; // sign indicates pointer-ness (positive for no, negative for yes)
} _ra_type;

typedef const _ra_type ra_type;

#define DEFINE_RA_TYPE(typename) (ra_type) { #typename, (int) sizeof(typename) }
#define DEFINE_RA_PNT_TYPE(typename) (ra_type) { #typename, -1 * (int) sizeof(typename) }

ra_type RA_INT = DEFINE_RA_TYPE(int);
ra_type RA_STR = DEFINE_RA_PNT_TYPE(char *);
//gen 2 "Define types."
```

With `ra_type` I'm doing something unusual, which is `const` members and type information by default. The primary rationale for this is to leverage the compiler's ability to prevent modifications to `const` variables to ensure type information is invariate in an array. The way `ra_type` stores type information is by storing a string representation of the name of the type (what you type in when labelling a variable: `int`, `char *`, etc.) and, for efficiency's sake (preventing multiple `sizeof()` calls) the size of that type. Below is an example of a function using `ra_type`:

```c
void * ra_append(r_array * ra, ...) {
    /* ... */

    #define RA_APPEND_TYPE(pair, _type) \
        if(strcmp(#_type, ra->type.typename) == 0) { \
            _type value = va_arg(args, _type); \
            ra->count++; \
            return aa_alloc(&ra->memory, &value, pair.size); \
        }

    /* ... */

    RA_APPEND_TYPE(RA_INT, int);

    /* ... */
}
```

Here, we are essentially saying "check for `ra_append()` calls for arrays of type `RA_INT`." This may seem clunky, but because this appears in a very limited and repeatable form (which is why I use macros), this is very easily code-generateable; more on this later. We take in the pair (`RA_INT`) and the type (`int`) associated with it, and compare the string representation of the type (`#_type`) to the `typename` variable held in the passed array's type-pair. In this way we can get all the type information we need from the user.

There is a wrinkle here that the above code will not work for strings. There is a buffer in this library that holds all the string (really, all the pointers that need 'dynamic' allocation) data elsewhere so that an `RA_STR`-typed array can just hold a `char *` (which points to this internal buffer). Currently, this internal buffer is just a big arena, so it's not *truly* dynamic (because it does not dynamically resize, because doing so with `realloc()` would break all existing pointers; either I need refcounting, or to implement some sort of dynamically-expanding buffer for this that doesn't use `realloc()`; maybe a linked list of large buffers?). Most of the macros here have `_PNT` variants, but this should be relatively obscure to the user.

### How To Use

This library relies on code generation, something I have also relied on in my [anecs](https://github.com/wrzeczak/anecs) project. The reason there is no provided file `r_array.h` is because this file is supposed to be the result of running `ra_generator.c`. The "base" library, which provides `RA_INT` and `RA_STR`, is in `r_array_template.h`, and you can just use that if that's all you need. However, if you need another type (say, `RA_DOUBLE`, or in Raylib, `RA_VECTOR2`), you can use `ra_generator.c` to generate an ra library that can handle those types. Here's how it works:

```c
// ra_generator.c
int main(void) {
    header_files.memory = aa_create(1024);

    register_new_type("RA_DOUBLE", "double", NULL, NULL);
    register_new_type("RA_VECTOR2", "Vector2", "<raylib.h>", "Vector2Equals");
    
    generate_ra("r_array.h");

    return 0;
}

```

That first line is necessary for obscure reasons; essentially, because of the `const` nature of `ra_type`, arrays cannot really be forward-declared, only initialized with `NULL` memory to be assigned later. See `RA_STATIC_INIT()`. To create a new type, `register_new_type()` will take a typename (pair name, preferably styled `RA_TYPENAME`, in all caps), the associated type (the actual type you would type in while editing, `int`, `char *`, `Vector2`, etc.), and if necessary, a header file (which will automatically be included into `r_array.h`; pass `NULL` if your type is built-in), and if necessary a comparison function (useful for `ra_member_of()` and `ra_member_at`; **NOTE**: this was designed to accomodate `strcmp()`, so these comparison functions are expected to return `0` on equal, and non-zero otherwise; pass `NULL` to just use `==`).

This system makes use of generator comments:
```c
//gen step_number "Label of step"
```
Which here and in ANECS identify what to do in code generation. This reads in `ra_template.h`, applies the necessary changes with the information given, and spits out `r_array.h` (the argument to `generate_ra()`).

Some details here:
1) For file inclusion, "`<raylib.h>`" will generate "`#include <raylib.h>`", whereas just "`raylib.h`" will generate "`#include "raylib.h"`"
2) You cannot create `RA_FLOAT` associated with `float` in modern C. This is because `va_args` does not support passing float, only `double`. If you need floats, create `RA_DOUBLE`.
3) The only reason for including files that define types is such that the debugger/syntax highlighter when editing doesn't blow up; if this symbol can already be found, then there's no reason to double-include it.
4) I'll be fully honest in saying I have not fully tested every code-generation quirk, especially for the pointer types. SORRY!
5) If you have an STB-style library that requires a pound-define, you'll need to add it manually.

### So, Step-by-step

1) Compile + run `ra_generator.c` with the types you need; if you only need `RA_INT` and `RA_STR`, just don't register any types:
```
> gcc -o gen ra_generator.c
> ./gen
...
```
2) Use `r_array.h`! See `demo.c` for examples on how to do this.