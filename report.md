# CSAPP Bomb Lab 实验报告

## 一、实验概述

本实验使用 objdump 反汇编和 GDB 调试，分析无源码的二进制炸弹，并在 defuser.c 中用 C 语言实现从 challenge 计算出口令的函数。
环境：Ubuntu 24.04.5，x86-64。
完成情况：正式六关和 Honor 全部通过，六关 Public tests 60/60，Honor Secret public tests 10/10。

## 二、各关卡分析

### Phase 1
汇编逻辑：
p1_compute 拆分 challenge 为 lo, hi, mid。
p1_step0: (lo * 3) ^ (hi + 0x51dd)
p1_step1: (hi * 3 + mid) ^ 0xc394
p1_step2: (s1 - lo) * 3 + 0x2bf6
最终返回 step2 ^ step0。

C 实现：
uint32_t lo = (uint32_t)challenge;
uint32_t hi = (uint32_t)(challenge >> 32);
uint32_t mid = (uint32_t)(challenge >> 16);
uint32_t r0 = (lo * 3u) ^ (hi + 0x51ddu);
uint32_t r1 = (hi * 3u + mid) ^ 0xc394u;
uint32_t r2 = (r1 - lo) * 3u + 0x2bf6u;
values[0] = r2 ^ r0;

验证：seed 42 通过，公开测试 10/10。

### Phase 2
汇编逻辑：
要求 6 个整数。第 1 个字段为 ((challenge >> 33) & 0xF) + 0x15。
后续 5 个字段递推：v[k+1] = ((challenge >> (4k+9)) & 0x3F) ^ (v[k] * 2) + (0xC * (k+1))。

C 实现：
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

验证：seed 42 通过，公开测试 10/10。

### Phase 3
汇编逻辑：
读 2 个整数。a = challenge & 7，b = (challenge>>32) ^ (challenge>>3)。
p3_dispatch 根据 a 查跳转表 0x405820。
注意：表项下标与 case 编号不一致。必须用 GDB 读表：下标 0~7 分别对应 case 7, 6, 4, 1, 2, 0, 3, 5。

C 实现：
uint32_t a = challenge & 0x7u;
uint32_t b = (challenge >> 32) ^ (challenge >> 3);
switch (a) {
    case 0: r = (b ^ 0xbe12u) + (b >> 2); break; // 表[0]
    case 1: r = (b * 4u) ^ 0x6c61u; break;       // 表[1]
    // ... (按实际表顺序填入)
}

踩坑：第一次没看 GDB 直接按编号写 switch 导致 BOOM，看表后修正。
验证：seed 42 通过，公开测试 10/10。

### Phase 4
汇编逻辑：
二分查找递归。p4_walk(a, lo, hi, c)。
mid = lo + (hi - lo) / 2。
若 mid == a 返回 (mid + 0x8A) ^ c。
若 a > mid 向右递归，返回 r * 5 ^ (mid + c) ^ 0x12。
若 a < mid 向左递归，返回 r * 5 ^ (mid + c) ^ 0xE。

C 实现：
用循环模拟二分路径，记录每层 mid 和方向，回溯时计算：
values[0] = (r * 5u) ^ (mid + c) ^ 0x12u; (或 ^ 0xEu)

验证：seed 42 通过，公开测试 10/10。

### Phase 5
汇编逻辑：
链表遍历。从 p5_heads 数组选入口，沿 p5_nodes 遍历 steps 次。
a = ((challenge>>6) ^ (challenge>>32)) ^ node.val_b ^ 0xe17e + node.val_a。
同时通过指针写回第二个值 *out = node.val_a。

C 实现：
通过 GDB 读出 p5_nodes 和 p5_heads 数据，硬编码为数组。
按上述逻辑遍历，最后 values[0] = a, values[1] = out。

验证：seed 42 通过，公开测试 10/10。

### Phase 6
汇编逻辑：
两次查函数指针表。p6_ops 表在 0x405e00。
arg1 = (challenge>>6) ^ (challenge>>32)。
第一次查表 idx = challenge & 7，结果 ^ 0x6f9a。
第二次查表 idx = (challenge>>3) & 7，结果 + 0x3736 即为答案。

C 实现：
通过 GDB 读出 p6_ops 表，实现 p6_call_table 函数。
r1 = p6_call_table(challenge & 7, arg1);
r1 ^= 0x6f9a;
r2 = p6_call_table((challenge>>3) & 7, r1);
values[0] = r2 + 0x3736;

验证：seed 42 通过，公开测试 10/10。

### Secret (Honor)
前置校验：
secret_gate 检查输入长度 (36~163) 且首 4 字节为 "OPEN"。
偏移坑：strcpy 写到 -0xd0，但检查 "OPEN" 在 -0xb0 (偏移 32)。因此必须输出 32 个填充字符 + "OPEN v1 v2 v3"。

三个数校验：
v1 = context & 3
v2 = secret_mix(edi) ^ r9
v3 = v2 ^ (r8d + (context&3) + 0x7d8 + (esi>>5))

C 实现：
通过 snprintf(out, cap, "%32sOPEN %u %u %u", "", v1, v2, v3); 补齐偏移。
踩坑：之前只有 29 字符，未过长度检查，未进 secret_phase；且 + 和 ^ 混用需加括号防编译报错。

验证：seed 42 通过，公开测试 10/10。

## 三、验证记录

- 公开测试：六关全 10/10，Honor 10/10。
- 非公开 seed 43：六关和 Honor 全部通过。

## 四、工具与资料
使用 objdump 和 GDB 分析，AI 辅助理解汇编。所有代码均经实际测试验证。
