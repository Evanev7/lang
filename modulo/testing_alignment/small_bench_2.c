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

// ---------- tiny platform helpers ----------
static inline double now_s(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
static void* xaligned_alloc(size_t align, size_t bytes){
    void* ptr = NULL;
    int rc = posix_memalign(&ptr, align, bytes);
    if (rc != 0) { errno = rc; perror("posix_memalign"); exit(1); }
    return ptr;
}
static inline void fill(uint8_t* p, size_t n){
    for (size_t i = 0; i < n; ++i) p[i] = (uint8_t)(i * 1315423911u);
}
#if defined(__GNUC__)
#define assume_aligned __builtin_assume_aligned
static inline void compiler_barrier(void){ __asm__ __volatile__("" ::: "memory"); }
#else
static inline const void* assume_aligned(const void* p, unsigned n) { (void)n; return p; }
static inline void compiler_barrier(void){ /* no-op */ }
#endif

// ---------- unaligned-safe accessors ----------
static inline uint64_t load_u64_memcpy(const void *p) {
    uint64_t v; __builtin_memcpy(&v, p, sizeof v); return v;
}
static inline void store_u64_memcpy(void *p, uint64_t v) {
    __builtin_memcpy(p, &v, sizeof v);
}

static inline uint32_t load_u32_memcpy(const void *p) {
    uint32_t v; __builtin_memcpy(&v, p, sizeof v); return v;
}
static inline void store_u32_memcpy(void *p, uint32_t v) {
    __builtin_memcpy(p, &v, sizeof v);
}

// Byte-gather/scatter (portable)
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
static inline void store_u64_bytes(void *p, uint64_t v) {
    uint8_t *b = (uint8_t*)p;
    b[0] = (uint8_t)(v      );
    b[1] = (uint8_t)(v >>  8);
    b[2] = (uint8_t)(v >> 16);
    b[3] = (uint8_t)(v >> 24);
    b[4] = (uint8_t)(v >> 32);
    b[5] = (uint8_t)(v >> 40);
    b[6] = (uint8_t)(v >> 48);
    b[7] = (uint8_t)(v >> 56);
}

// Two 32-bit ops (safe; often better on split accesses)
static inline uint64_t load_u64_2x32(const void *p) {
    uint32_t lo = load_u32_memcpy(p);
    uint32_t hi = load_u32_memcpy((const uint8_t*)p + 4);
    return ((uint64_t)hi << 32) | lo;
}
static inline void store_u64_2x32(void *p, uint64_t v) {
    store_u32_memcpy(p,              (uint32_t)v);
    store_u32_memcpy((uint8_t*)p+4, (uint32_t)(v >> 32));
}

// Aligned baseline (only call when truly aligned)
static inline uint64_t load_u64_aligned(const void *p) {
    const uint64_t* a = (const uint64_t*)assume_aligned(p, 64);
    return *a;
}
static inline void store_u64_aligned(void *p, uint64_t v) {
    uint64_t* a = (uint64_t*)assume_aligned(p, 64);
    *a = v;
}

#if ALLOW_UB_TYPEPUN
// UB unless aligned & correctly typed; included for curiosity
static inline uint64_t load_u64_typepun(const void *p) { return *(const uint64_t*)p; }
static inline void     store_u64_typepun(void *p, uint64_t v){ *(uint64_t*)p = v; }
#endif

// ---------- benchmarking harness (inlined variants) ----------
typedef struct { double sec; uint64_t ops; } Bench;
static volatile uint64_t g_sink;

#define TARGET_SEC 0.25  // per-case time budget

// Generate a load-bench function with the given name and load expression (must yield uint64_t).
#define DEFINE_LOAD_BENCH(FNNAME, LOAD_EXPR)                                              \
static Bench FNNAME(const uint8_t* base, size_t offset, size_t ws_bytes, size_t stride) { \
    const uint8_t* start = base + offset;                                                 \
    const uint8_t* end   = base + offset + ws_bytes - stride;                             \
    const uint8_t* p     = start;                                                         \
    for (int i = 0; i < 2000; ++i) { (void)(LOAD_EXPR); p += stride; if (p > end) p = start; } \
    double t0 = now_s();                                                                   \
    uint64_t acc = 0, ops = 0;                                                             \
    do {                                                                                   \
        for (int k = 0; k < 4096; ++k) {                                                   \
            acc ^= (LOAD_EXPR);                                                            \
            acc = (acc << 1) | (acc >> 63);                                                \
            p += stride; if (p > end) p = start;                                           \
        }                                                                                  \
        ops += 4096;                                                                       \
    } while ((now_s() - t0) < TARGET_SEC);                                                 \
    g_sink ^= acc;                                                                         \
    return (Bench){ now_s() - t0, ops };                                                   \
}

// Generate a store-bench function with the given name and store statement (uses 'val' and 'p').
#define DEFINE_STORE_BENCH(FNNAME, STORE_STMT)                                            \
static Bench FNNAME(uint8_t* base, size_t offset, size_t ws_bytes, size_t stride) {       \
    uint8_t* start = base + offset;                                                       \
    uint8_t* end   = base + offset + ws_bytes - stride;                                   \
    uint8_t* p     = start;                                                               \
    uint64_t val = 0x9E3779B97F4A7C15ull;                                                 \
    for (int i = 0; i < 2000; ++i) { { STORE_STMT; } p += stride; if (p > end) p = start; val += 0x9e37; } \
    double t0 = now_s();                                                                   \
    uint64_t ops = 0;                                                                      \
    do {                                                                                   \
        for (int k = 0; k < 4096; ++k) {                                                   \
            { STORE_STMT; }                                                                \
            val ^= val << 7; val ^= val >> 9;                                             \
            p += stride; if (p > end) p = start;                                           \
        }                                                                                  \
        ops += 4096;                                                                       \
    } while ((now_s() - t0) < TARGET_SEC);                                                 \
    compiler_barrier();                                                                    \
    /* fold some bytes (safely) into g_sink to check side-effects */                       \
    uint64_t chk = 0;                                                                      \
    for (size_t i = 0; i < ws_bytes; i += 64) chk ^= load_u64_memcpy(start + i);          \
    g_sink ^= chk ^ val;                                                                   \
    return (Bench){ now_s() - t0, ops };                                                   \
}

// ---- instantiate benches ----
// Loads
DEFINE_LOAD_BENCH(bench_load_aligned,     load_u64_aligned(p))
DEFINE_LOAD_BENCH(bench_load_memcpy,      load_u64_memcpy(p))
DEFINE_LOAD_BENCH(bench_load_bytes,       load_u64_bytes(p))
DEFINE_LOAD_BENCH(bench_load_2x32,        load_u64_2x32(p))
#if ALLOW_UB_TYPEPUN
DEFINE_LOAD_BENCH(bench_load_typepun,     load_u64_typepun(p))
#endif

// Stores
DEFINE_STORE_BENCH(bench_store_aligned,   store_u64_aligned(p, val))
DEFINE_STORE_BENCH(bench_store_memcpy,    store_u64_memcpy(p, val))
DEFINE_STORE_BENCH(bench_store_bytes,     store_u64_bytes(p, val))
DEFINE_STORE_BENCH(bench_store_2x32,      store_u64_2x32(p, val))
#if ALLOW_UB_TYPEPUN
DEFINE_STORE_BENCH(bench_store_typepun,   store_u64_typepun(p, val))
#endif

static void print_load(const char* name, Bench b){
    double lps = (double)b.ops / b.sec;
    double gb  = (lps * 8.0) / 1e9;
    printf("%-28s : %7.3f Mops/s, %5.2f GB/s  (sink=%" PRIu64 ")\n",
           name, lps/1e6, gb, (uint64_t)g_sink);
}
static void print_store(const char* name, Bench b){
    double sps = (double)b.ops / b.sec;
    double gb  = (sps * 8.0) / 1e9;
    printf("%-28s : %7.3f Mops/s, %5.2f GB/s  (sink=%" PRIu64 ")\n",
           name, sps/1e6, gb, (uint64_t)g_sink);
}

int main(void){
    const size_t L1    = 32 * 1024;   // keep hot in L1
    const size_t S64   = 64;          // cache-line stride
    const size_t PAGE  = 4096;        // typical; parameterize if needed
    const size_t SLACK = PAGE + 128;  // room for misaligned/page offsets

    const size_t WS_LINE = L1;
    const size_t WS_PAGE = PAGE * 32;

    // allocate enough for the largest working set + offsets
    const size_t BUF = (WS_PAGE > WS_LINE ? WS_PAGE : WS_LINE) + SLACK + 64;
    uint8_t* buf = (uint8_t*)xaligned_alloc(64, BUF);
    fill(buf, BUF);

    printf("=== LOADS: cache-line stride (64B), WS=%zu ===\n", WS_LINE);
    print_load("aligned +0 (baseline)",   bench_load_aligned(buf, 0,   WS_LINE, S64));
    print_load("memcpy  +4  (intra)",     bench_load_memcpy (buf, 4,   WS_LINE, S64));
    print_load("bytes   +4  (intra)",     bench_load_bytes  (buf, 4,   WS_LINE, S64));
    print_load("2x32    +4  (intra)",     bench_load_2x32   (buf, 4,   WS_LINE, S64));
    print_load("memcpy  +60 (line-split)",bench_load_memcpy (buf, 60,  WS_LINE, S64));
    print_load("bytes   +60 (line-split)",bench_load_bytes  (buf, 60,  WS_LINE, S64));
    print_load("2x32    +60 (line-split)",bench_load_2x32   (buf, 60,  WS_LINE, S64));
#if ALLOW_UB_TYPEPUN
    print_load("typepun +4  (UB,intra)",  bench_load_typepun(buf, 4,   WS_LINE, S64));
    print_load("typepun +60 (UB,split)",  bench_load_typepun(buf, 60,  WS_LINE, S64));
#endif

    printf("\n=== LOADS: page stride (4 KiB), WS=%zu (true page-split @+4092) ===\n", WS_PAGE);
    print_load("memcpy  +4092 (page)",    bench_load_memcpy (buf, 4092, WS_PAGE, PAGE));
    print_load("bytes   +4092 (page)",    bench_load_bytes  (buf, 4092, WS_PAGE, PAGE));
    print_load("2x32    +4092 (page)",    bench_load_2x32   (buf, 4092, WS_PAGE, PAGE));
#if ALLOW_UB_TYPEPUN
    print_load("typepun +4092 (UB,page)", bench_load_typepun(buf, 4092, WS_PAGE, PAGE));
#endif

    // --- STORES ---
    fill(buf, BUF);

    printf("\n=== STORES: cache-line stride (64B), WS=%zu ===\n", WS_LINE);
    print_store("aligned +0 (baseline)",  bench_store_aligned(buf, 0,   WS_LINE, S64));
    print_store("memcpy  +4  (intra)",    bench_store_memcpy (buf, 4,   WS_LINE, S64));
    print_store("bytes   +4  (intra)",    bench_store_bytes  (buf, 4,   WS_LINE, S64));
    print_store("2x32    +4  (intra)",    bench_store_2x32   (buf, 4,   WS_LINE, S64));
    print_store("memcpy  +60 (line-split)",bench_store_memcpy(buf, 60,  WS_LINE, S64));
    print_store("bytes   +60 (line-split)",bench_store_bytes (buf, 60,  WS_LINE, S64));
    print_store("2x32    +60 (line-split)",bench_store_2x32  (buf, 60,  WS_LINE, S64));
#if ALLOW_UB_TYPEPUN
    print_store("typepun +4  (UB,intra)", bench_store_typepun(buf, 4,   WS_LINE, S64));
    print_store("typepun +60 (UB,split)", bench_store_typepun(buf, 60,  WS_LINE, S64));
#endif

    fill(buf, BUF);
    printf("\n=== STORES: page stride (4 KiB), WS=%zu (true page-split @+4092) ===\n", WS_PAGE);
    print_store("memcpy  +4092 (page)",   bench_store_memcpy (buf, 4092, WS_PAGE, PAGE));
    print_store("bytes   +4092 (page)",   bench_store_bytes  (buf, 4092, WS_PAGE, PAGE));
    print_store("2x32    +4092 (page)",   bench_store_2x32   (buf, 4092, WS_PAGE, PAGE));
#if ALLOW_UB_TYPEPUN
    print_store("typepun +4092 (UB,page)",bench_store_typepun(buf, 4092, WS_PAGE, PAGE));
#endif

    free(buf);
    return 0;
}

