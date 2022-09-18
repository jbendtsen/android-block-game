#include <time.h>

typedef unsigned int u32;
typedef unsigned long long u64;

#define ROTL64(x, r) (x << r) | (x >> (64 - r))

// Uses Murmur3 128bit
void generate_random_128(u64 *out1, u64 *out2, u64 twiddle)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    u64 a = ts.tv_sec;
    u64 b = ts.tv_nsec;

    const u64 c1 = 0x87c37b91114253d5ULL;
    const u64 c2 = 0x4cf5ad432745937fULL;

    u64 h1 = twiddle;
    u64 h2 = twiddle;

    a *= c1;
    a = ROTL64(a, 31);
    a *= c2;
    h1 ^= a;

    h1 = ROTL64(h1, 27);
    h1 += h2;
    h1 = h1*5 + 0x52dce729;

    b *= c2;
    b = ROTL64(b, 33);
    b *= c1;
    h2 ^= b;

    h2 = ROTL64(h2, 31);
    h2 += h1;
    h2 = h2*5 + 0x38495ab5;

    a *= c1;
    a = ROTL64(a, 31);
    a *= c2;
    h1 ^= a;

    const u64 len = 16;
    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    const u64 d1 = 0xff51afd7ed558ccdULL;
    const u64 d2 = 0xc4ceb9fe1a85ec53ULL;

    h1 ^= h1 >> 33;
    h1 *= d1;
    h1 ^= h1 >> 33;
    h1 *= d2;
    h1 ^= h1 >> 33;

    h2 ^= h2 >> 33;
    h2 *= d1;
    h2 ^= h2 >> 33;
    h2 *= d2;
    h2 ^= h2 >> 33;

    h1 += h2;
    h2 += h1;

    *out1 = h1;
    *out2 = h2;
}

u64 generate_random_64(int seed)
{
    u64 a = 0, b = 0;
    generate_random_128(&a, &b, (u64)seed);
    return a ^ b;
}

void generate_random_floats(float *out, int n)
{
    if (n <= 0)
        return;

    u32 r[4];
    for (int i = 0; i < n; i++) {
        if (i % 4 == 0)
            generate_random_128((u64*)&r[0], (u64*)&r[2], i / 4);

        u32 x = r[i % 4];
        u32 exp = 126;
        for (int j = 0; j < 24; j++) {
            if (x & (1 << 23))
                break;
            exp--;
            x <<= 1;
        }

        if ((x & 0xffffff) == 0) exp = 0;
        ((u32*)out)[i] = (exp << 23) | (x & 0x7fffff);
    }
}