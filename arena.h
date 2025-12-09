// Copyright 2025 aoi-buh
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <threads.h>

/* internal helpers */
#define ARENA_FAILED_TO_ALLOC_ARENA "failed to allocate for arena"
#define ARENA_FAILED_TO_FREE_ARENA "failed to free arena"

#define arena__err(msg) (fprintf(stderr, "arena.h: " msg "\n"),exit(EXIT_FAILURE))

static void arena__noop(void) {}

#if defined(__linux__) || defined(__APPLE__)
#    include <sys/mman.h>
#    define arena__perr(msg) (perror(msg),exit(EXIT_FAILURE))
#    define arena__alloc(size) mmap(NULL,                           \
                                    size,                           \
                                    PROT_READ | PROT_WRITE,         \
                                    MAP_PRIVATE | MAP_ANONYMOUS,    \
                                    -1,                             \
                                    0)
#    define arena__free(ptr,size) (munmap(ptr,size) == -1               \
                                   ? arena__perr(ARENA_FAILED_TO_FREE_ARENA) \
                                   : arena__noop())
#    define arena__alloc_failed(ptr) (ptr==MAP_FAILED)
#elif defined(_WIN32) || defined(_WIN64)
#    include <windows.h>
#    define arena__perr(m) printf("arena.h: " m " with error code %lu\n", GetLastError())
#    define arena__alloc(size) VirtualAlloc(NULL,                       \
                                            size,                       \
                                            MEM_COMMIT | MEM_RESERVE,   \
                                            PAGE_READWRITE)
#    define arena__free(ptr,_) (!VirtualFree(ptr,0,MEM_RELEASE)         \
                                ? arena__perr(ARENA_FAILED_TO_FREE_ARENA) \
                                : arena__noop())
#    define arena__alloc_failed(ptr) (ptr==NULL)
#else
#    error failed to find platform
#endif


/* options */
#ifndef ARENA_DEPTH
#    define ARENA_DEPTH 10
#endif


/* type definitions */
typedef struct {
    size_t size;
    void *base;
    void *current;
} arena_t;

typedef struct {
    size_t len;
    arena_t stack[ARENA_DEPTH];
} arena_a;


/* initializing the arena stack */
thread_local arena_a ARENAS = {0};


/* internals */
#define arena__cur ARENAS.stack[ARENAS.len]

// void arena__push(size_t)
#define arena__push(w)                              \
    (ARENAS.len+1 > ARENA_DEPTH                     \
     ? arena__err("ran out of depth")               \
     : (arena__cur.size = (w),                      \
        arena__cur.base = arena__alloc(w),          \
        arena__cur.current = arena__cur.base,       \
        ARENAS.len++,                               \
        (arena__alloc_failed(arena__cur.base)       \
         ? arena__perr(ARENA_FAILED_TO_ALLOC_ARENA) \
         : arena__noop())))

// void arena__pop(void)
#define arena__pop()                                        \
    (ARENA.len == 0                                         \
     ? arena__err("attempted to pop with no existing arena, \
                  this is not supposed to happen")          \
     : (arena__free(arena__cur.base, arena__cur.size),      \
        ARENAS.len--,                                       \
        arena__noop()))


/* api helpers (internal) */
#define ARENA__OPENq(p,...) p##__VA_ARGS__
#define ARENA__NOTHING

#define ARENA__NO_MACRO(f,...) OPENq(,f ARENA__NOTHING (__VA_ARGS__))


/* api */
// arena(size_t)
// the size supplied should have no side effects
#define arena(w) for (bool arena__run = true;                       \
                      arena__run? (arena__push(w), true) : false;   \
                      (arena__pop(), arena__run = false))

// void *malloc(size_t)
#define malloc(s)
// void *calloc(size_t,size_t)
#define calloc(n,s)
// void *realloc(void*,size_t)
#define realloc(p,s)
// void free(void)
#define free(p)
// void *ralloc(void*,size_t,void* (*)(void*,void*))
// ralloc :: void* -> size_t -> (void* -> void* -> void*)
#define ralloc(p,s,f)

// same types as std
#define std_malloc(...) ARENA__NO_MACRO(malloc,__VA_ARGS__)
#define std_calloc(...) ARENA__NO_MACRO(calloc,__VA_ARGS__)
#define std_realloc(...) ARENA__NO_MACRO(realloc,__VA_ARGS__)
#define std_free(...) ARENA__NO_MACRO(free,__VA_ARGS__)


#endif // ARENA_H
