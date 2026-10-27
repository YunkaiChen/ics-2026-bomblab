#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t lo  = (uint32_t)challenge;
    uint32_t hi  = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t r0 = (lo * 3u) ^ (hi + 0x51ddu);
    uint32_t r1 = (hi * 3u + mid) ^ 0xc394u;
    uint32_t r2 = (r1 - lo) * 3u + 0x2bf6u;
    values[0] = r2 ^ r0;
    count = 1;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;   
    values[0] = (uint32_t)(((challenge >> 33) & 0xFu) + 0x15u);
    uint32_t prev = values[0];
    uint32_t add  = 0xCu;
    for (int k = 0; k < 5; k++) {
        uint32_t shifted = (uint32_t)(challenge >> (k * 4 + 9)) & 0x3Fu;
        uint32_t x = shifted ^ (prev * 2u);
        x += add;
        values[k + 1] = x;
        prev = x;
        add += 0xCu;
    }
    count = 6;
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t a = (uint32_t)challenge & 0x7u;
    uint32_t b = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    uint32_t r;
    switch (a) {
        case 0: r = (b ^ 0xbe12u) + (b >> 2);      break;
        case 1: r = (b * 4u) ^ 0x6c61u;            break;
        case 2: r = b * 5u + 0x2a413u;             break;
        case 3: r = (b * 2u) ^ 0xf523u;            break;
        case 4: r = (b ^ 0xb144u) + (b >> 1);      break;
        case 5: r = b * 5u + 0x7c43u;              break;
        case 6: r = b * 5u + 0x3eb1bu;             break;
        case 7: r = b * 3u + 0x8c78u;              break;
        default: r = 0;                            break;   
   }
   values[0] = a;
   values[1] = r;
   count = 2;
   return write_answer(out, cap, values, count);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t a = (uint32_t)challenge & 0x7Fu;
    uint32_t c = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    uint32_t lo = 0, hi = 0x7Fu;
    uint32_t mids[64];
    int dirs[64];
    int depth = 0;
    while (1) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (mid == a) {
            values[0] = (mid + 0x8Au) ^ c;
            break;
        }
        mids[depth] = mid;
        if (a > mid) {
            dirs[depth] = 1;
            lo = mid + 1;
        } else {
            dirs[depth] = 0;
            hi = mid - 1;
        }
        depth++;
    }
    for (int i = depth - 1; i >= 0; i--) {
        uint32_t r = values[0];
        uint32_t mid = mids[i];
        if (dirs[i] == 1) {
            values[0] = (r * 5u) ^ (mid + c) ^ 0x12u;
        } else {
            values[0] = (r * 5u) ^ (mid + c) ^ 0xEu;
        }
    }
    count = 1;
    return write_answer(out, cap, values, count);
}

typedef struct {
    int next;
    uint32_t val_a;
    uint32_t val_b;
} p5_node;

static const p5_node p5_nodes[12] = {
    {  9, 0x83eu, 0xe40cu },
    {  8, 0x214u, 0x82fcu },
    {  0, 0x448u, 0x1c5eu },
    { -1, 0x675u, 0xf91du },
    {  1, 0x123u, 0x5ba9u },
    { 11, 0x91eu, 0x1842u },
    {  7, 0x71fu, 0x1efeu },
    { 10, 0x35bu, 0x193bu },
    {  2, 0xa86u, 0x7cc6u },
    {  6, 0x513u, 0x903fu },
    {  3, 0x2u,   0x3aaeu },
    {  4, 0xb2eu, 0xec54u },
};

static const int p5_heads[8] = { 5, 11, 4, 1, 8, 2, 0, 9 };

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t idx   = (uint32_t)challenge & 7u;
    uint32_t steps = (uint32_t)(challenge >> 3) & 3u;
    int p = p5_heads[idx];
    uint32_t a = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    a ^= p5_nodes[p].val_b;
    a ^= 0xe17eu;
    uint32_t second = p5_nodes[p].val_a;
    a += p5_nodes[p].val_a;
    for (uint32_t i = 1; i <= steps; i++) {
        p = p5_nodes[p].next;
        a ^= p5_nodes[p].val_b;
        a += p5_nodes[p].val_a;
        second = p5_nodes[p].val_a;
    }
    values[0] = a;
    values[1] = second;
    count = 2;
    return write_answer(out, cap, values, count);
}

static uint32_t p6_rol32(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
static uint32_t p6_ror32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static uint32_t p6_op_0(uint32_t x) { return (x * 8u + 0xa05du) ^ (x * 3u); }
static uint32_t p6_op_1(uint32_t x) { return p6_rol32(x, 5) ^ 0x3a27u; }
static uint32_t p6_op_2(uint32_t x) { return (x ^ 0x5ebcu) + x * 8u; }
static uint32_t p6_op_3(uint32_t x) { return (x ^ (x >> 1)) + 0x4aeu; }
static uint32_t p6_op_4(uint32_t x) { return (x + 0xd1cdu) ^ (x >> 1); }
static uint32_t p6_op_5(uint32_t x) { return p6_ror32(x, 3); }

static uint32_t p6_call_table(uint32_t idx, uint32_t arg) {
    switch (idx) {
        case 0: return p6_op_3(arg);
        case 1: return p6_op_0(arg);
        case 2: return p6_op_5(arg);
        case 3: return p6_op_4(arg);
        case 4: return p6_op_0(arg);
        case 5: return p6_op_2(arg);
        case 6: return p6_op_1(arg);
        case 7: return p6_op_1(arg);
        default: return 0;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t arg1 = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t idx1 = (uint32_t)challenge & 7u;
    uint32_t r1   = p6_call_table(idx1, arg1);
    r1 ^= 0x6f9au;
    uint32_t idx2 = ((uint32_t)challenge >> 3) & 7u;
    uint32_t r2   = p6_call_table(idx2, r1);
    values[0] = r2 + 0x3736u;
    count = 1;
    return write_answer(out, cap, values, count);
}

static uint32_t secret_mix(uint32_t x) {
    uint32_t r = (x << 5) | (x >> 27);
    return ((r + 0x7bfcu) ^ 0x7d8u);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t c_lo = (uint32_t)context;
    uint32_t c_hi = (uint32_t)(context >> 32);
    uint32_t c_mid = (uint32_t)(context >> 16);
    uint32_t v1 = c_lo & 3u;
    uint32_t esi = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t r8d = c_hi ^ c_lo;
    uint32_t edi = esi ^ r8d;
    uint32_t r9  = (c_lo & 3u) * 0x7bfcu;
    r9 ^= c_mid;
    uint32_t mix = secret_mix(edi);
    uint32_t v2 = mix ^ r9;
    uint32_t edx = r8d + (c_lo & 3u) + 0x7d8u;
    edx += (esi >> 5);
    uint32_t v3 = v2 ^ edx;
    int n = snprintf(out, cap, "%32sOPEN %u %u %u", "", v1, v2, v3);
    if (n < 0 || (size_t)n >= cap) return 1;
    while ((size_t)n < 36 && (size_t)n + 1 < cap) {
        out[n++] = ' ';
    }
    out[n] = '\0';
    return 0;
}
