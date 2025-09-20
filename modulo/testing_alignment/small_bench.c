#define _POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
# error "This demo assumes little-endian for the byte-gather variant."
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
#  define ALLOW_UB_TYPEPUN 1
#else
#  define ALLOW_UB_TYPEPUN 0
#endif

// ---- unaligned-safe helpers ----
static inline uint64_t load_u64_memcpy(const void *p) {
    uint64_t v; __builtin_memcpy(&v, p, sizeof v); return v;
}
static inline void store_u64_memcpy(void *p, uint64_t v) {
    __builtin_memcpy(p, &v, sizeof v);
}

// byte-gather (portable, may compile worse)
static inline uint64_t load_u64_bytes(const void *p) {
    const uint8_t *b = (const uint8_t*)p;
    return (uint64_t)b[0]
         | (uint64_t)b[1] << 8
         | (uint64_t)b[2] << 16
         | (uint64_t)b[3] << 24
         | (uint64_t)b[4] << 32
         | (uint64_t)b[5] << 40
         | (uint64_t)b[6] << 48
         | (uint64_t)b[7] << 56;
}

static inline uint32_t load_u32_memcpy(const void *p) {
    uint32_t v; __builtin_memcpy(&v, p, sizeof v); return v;
}
static inline uint64_t load_u64_2x32(const void *p) {
    uint32_t lo = load_u32_memcpy(p);
    uint32_t hi = load_u32_memcpy((const uint8_t*)p + 4);
    return ((uint64_t)hi << 32) | lo;
}

#if ALLOW_UB_TYPEPUN
// NOTE: This is UB unless 'p' actually points to an object of that type,
// *and* is suitably aligned (aliasing + alignment). Included for comparison.
static inline uint64_t load_u64_typepun(const void *p) { return *(const uint64_t*)p; }
#endif

static inline double now_s(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static volatile uint64_t g_sink;

typedef uint64_t (*load_fn)(const void*);
typedef struct { double sec; uint64_t ops; } Bench;

// Walk with configurable stride; run for ~target_sec.
// Keeps start..start+working_set_bytes hot while stepping by 'stride'.
static Bench bench_load_stride(load_fn fn, const uint8_t* base, size_t offset,
                               size_t working_set_bytes, size_t stride,
                               double target_sec) {
    const uint8_t* start = base + offset;
    const uint8_t* end   = base + offset + working_set_bytes - stride;
    const uint8_t* p     = start;

    // warm-up
    for (int i = 0; i < 2000; ++i) {
        g_sink += fn(p);
        p += stride; if (p > end) p = start;
    }

    double t0 = now_s();
    uint64_t ops = 0, acc = 0;

    do {
        // unroll a bit for fewer branches
        for (int k = 0; k < 4096; ++k) {
            acc += fn(p);
            p += stride; if (p > end) p = start;
        }
        ops += 4096;
    } while ((now_s() - t0) < target_sec);

    g_sink ^= acc;
    double elapsed = now_s() - t0;
    return (Bench){ elapsed, ops };
}

static void fill(uint8_t* p, size_t n){
    for (size_t i = 0; i < n; ++i) p[i] = (uint8_t)(i * 1315423911u);
}

static void* xaligned_alloc(size_t align, size_t bytes){
    void* ptr = NULL;
    int rc = posix_memalign(&ptr, align, bytes);
    if (rc != 0) { errno = rc; perror("posix_memalign"); exit(1); }
    return ptr;
}

static void run_case(const char* name, load_fn fn,
                     const uint8_t* buf, size_t off, size_t ws_bytes, size_t stride) {
    Bench b = bench_load_stride(fn, buf, off, ws_bytes, stride, 1.0);
    double loads_per_sec = (double)b.ops / b.sec;
    double gb_per_sec    = (loads_per_sec * 8.0) / 1e9;   // 8 bytes per load
    printf("%-28s : %.3f Mops/s, %.2f GB/s  (sink=%" PRIu64 ")\n",
           name, loads_per_sec / 1e6, gb_per_sec, (uint64_t)g_sink);
}

int main(void){
    const size_t L1   = 32 * 1024;         // keep it hot in L1
    const size_t S64  = 64;                // stride 64B => fixed modulo-64 pattern
    const size_t PAGE = 4096;              // typical page size (parameterize if needed)
    const size_t SLACK = 128;              // room for misaligned offsets
    const size_t BUF   = L1 + SLACK + 64;  // base 64B aligned

    uint8_t* buf = (uint8_t*)xaligned_alloc(64, BUF);
    fill(buf, BUF);

    printf("Working set: %zu bytes (L1)\n\n", L1);

    // --- Cache-line tests (stride = 64B) ---
    run_case("memcpy  +4  (intra-line)",   load_u64_memcpy,  buf, 4,   L1, S64);
    run_case("bytes   +4  (intra-line)",   load_u64_bytes,   buf, 4,   L1, S64);
    run_case("memcpy  +60 (cross-line)",   load_u64_memcpy,  buf, 60,  L1, S64);
    run_case("bytes   +60 (cross-line)",   load_u64_bytes,   buf, 60,  L1, S64);

    run_case("2x32   +4  (intra-line)",    load_u64_2x32,    buf, 4,   L1, S64);
    run_case("2x32   +60 (cross-line)",    load_u64_2x32,    buf, 60,  L1, S64);

#if ALLOW_UB_TYPEPUN
    run_case("typepun +4  (UB, intra)",    load_u64_typepun, buf, 4,   L1, S64);
    run_case("typepun +60 (UB, cross)",    load_u64_typepun, buf, 60,  L1, S64);
#endif

    // --- True page-split: every access crosses a page boundary (stride = 4 KiB) ---
    // Working set sized to an integer number of pages so we loop cleanly.
    printf("\n-- Page-split stride = PAGE --\n");
    run_case("memcpy  +4092 (page-split)", load_u64_memcpy,  buf, 4092, PAGE * 32, PAGE);
    run_case("bytes   +4092 (page-split)", load_u64_bytes,   buf, 4092, PAGE * 32, PAGE);
#if ALLOW_UB_TYPEPUN
    run_case("typepun +4092 (UB, page)",   load_u64_typepun, buf, 4092, PAGE * 32, PAGE);
#endif

    free(buf);
    return 0;
}

