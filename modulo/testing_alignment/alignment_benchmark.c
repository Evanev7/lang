#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>

#define ARRAY_SIZE        (1024u * 1024u * 64u)
#define NUM_ITERATIONS    100
#define ATOMIC_OPERATIONS 10000000
#define ALIGNMENT         64
#define SIMD_WIDTH        8
#define SLACK_FLOATS      16   // 64 bytes of slack for unaligned loads

static inline double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline float hsum256_ps(__m256 v){
    __m128 lo = _mm256_castps256_ps128(v);
    __m128 hi = _mm256_extractf128_ps(v, 1);
    __m128 s  = _mm_add_ps(lo, hi);
    s = _mm_hadd_ps(s, s);
    s = _mm_hadd_ps(s, s);
    float out;
    _mm_store_ss(&out, s);
    return out;
}

float benchmark_simd_once(const float* data, size_t n, int use_aligned_load) {
    __m256 sum_vec = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 v = use_aligned_load ? _mm256_load_ps(data + i)
                                    : _mm256_loadu_ps(data + i);
        sum_vec = _mm256_add_ps(sum_vec, v);
    }
    // (optional) handle tail if n%8 != 0
    return hsum256_ps(sum_vec);
}


static void benchmark_simd(const float* data, size_t n_floats, const char* label, int use_aligned_load) {
    double start = get_time();
    float total = 0.0f;
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        total += benchmark_simd_once(data, ARRAY_SIZE, use_aligned_load);
    }
    double end = get_time();
    printf("SIMD Sum (%s):\t\t%.9f seconds\n", label, end - start);

    printf("  \\-> Final checksum: %f\n", total);
}

typedef struct {
    uint64_t* atomic_var;
    int ops;
} atomic_thread_data_t;

static void* atomic_thread_func(void* arg) {
    atomic_thread_data_t* d = (atomic_thread_data_t*)arg;
    for (int i = 0; i < d->ops; ++i) {
        __atomic_fetch_add(d->atomic_var, 1u, __ATOMIC_SEQ_CST);
    }
    return NULL;
}

static void benchmark_atomics(uint64_t* atomic_var, int num_threads, int total_ops, const char* label) {
    pthread_t threads[num_threads];
    atomic_thread_data_t thread_data[num_threads];

    int base = total_ops / num_threads;
    int rem  = total_ops % num_threads;

    double start = get_time();
    for (int t = 0; t < num_threads; ++t) {
        thread_data[t].atomic_var = atomic_var;
        thread_data[t].ops = base + (t < rem ? 1 : 0);
        int rc = pthread_create(&threads[t], NULL, atomic_thread_func, &thread_data[t]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", rc);
            exit(1);
        }
    }
    for (int t = 0; t < num_threads; ++t) {
        pthread_join(threads[t], NULL);
    }
    double end = get_time();
    printf("Atomic Increment (%s):\t%.6f seconds\n", label, end - start);
}

void* wrapped_posix_memalign(size_t alignment, size_t bytes) {
    void* ptr = NULL;
    int err = posix_memalign(&ptr, alignment, bytes);
    if (err) {
        errno = err;
        return NULL;
    }
    return ptr;
}

int main(void) {
    // --- SIMD buffers ---
    const size_t total_floats = ARRAY_SIZE + SLACK_FLOATS;           // initialize slack too
    const size_t total_bytes  = total_floats * sizeof(float);        // multiple of 64 here

    float* aligned_buffer = (float*)wrapped_posix_memalign(ALIGNMENT, total_bytes);
    if (!aligned_buffer) { perror("posix_memalign(aligned_buffer)"); return 1; }

    // initialize full range (including slack)
    for (size_t i = 0; i < total_floats; ++i) aligned_buffer[i] = (float)i;

    printf("--- SIMD Benchmark ---\n");
    // aligned (32B-aligned loads; buffer is 64B-aligned, and i advances by 8 floats = 32 bytes)
    benchmark_simd(aligned_buffer, ARRAY_SIZE, "Aligned", /*use_aligned_load=*/1);

    // unaligned (4B offset)
    float* unaligned_4B = aligned_buffer + 1;            // +4 bytes
    benchmark_simd(unaligned_4B, ARRAY_SIZE, "Unaligned (4-byte offset)", /*aligned=*/0);

    // unaligned (60B offset = 15 floats) - likely to straddle cache lines frequently
    float* unaligned_60B = (float*)((char*)aligned_buffer + 60);
    benchmark_simd(unaligned_60B, ARRAY_SIZE, "Unaligned (60-byte offset)", /*aligned=*/0);

    printf("\n--- Atomic Operation Benchmark ---\n");

    // --- Aligned atomic (size must be a multiple of ALIGNMENT) ---
    uint64_t* aligned_atomic = (uint64_t*)wrapped_posix_memalign(ALIGNMENT, ALIGNMENT);
    if (!aligned_atomic) { perror("wrapped_posix_memalign(aligned_atomic)"); return 1; }
    *aligned_atomic = 0;
    benchmark_atomics(aligned_atomic, /*threads=*/4, ATOMIC_OPERATIONS, "Aligned");

    // --- Unaligned atomic (misaligned by 4 bytes) ---
    // Allocate enough and keep size multiple of ALIGNMENT.
    uint8_t* atomic_base = (uint8_t*)wrapped_posix_memalign(ALIGNMENT, 2 * ALIGNMENT);
    if (!atomic_base) { perror("wrapped_posix_memalign(atomic_base)"); return 1; }

    uint64_t* unaligned_atomic_4B = (uint64_t*)(atomic_base + 4);   // misaligned (doesn't straddle cacheline)
#if defined(__x86_64__) || defined(_M_X64)
    *unaligned_atomic_4B = 0;
    benchmark_atomics(unaligned_atomic_4B, /*threads=*/4, ATOMIC_OPERATIONS, "Unaligned (4-byte offset)");
#else
    printf("Unaligned atomics skipped on this architecture.\n");
#endif

    // Optional: worst-case unaligned that straddles a 64B line
    uint64_t* unaligned_atomic_60B = (uint64_t*)(atomic_base + 60); // spans bytes 60..67
#if defined(__x86_64__) || defined(_M_X64)
    *unaligned_atomic_60B = 0;
    benchmark_atomics(unaligned_atomic_60B, /*threads=*/4, ATOMIC_OPERATIONS, "Unaligned (60-byte offset, line-crossing)");
#endif

    free(aligned_buffer);
    free(aligned_atomic);
    free(atomic_base);
    return 0;
}

