; int sp_stats_asm(const double *x, size_t n, sp_stats_t *out)
;
; ABI: rdi = x, rsi = n, rdx = out.
; sp_stats_t layout: min @ +0, max @ +8, mean @ +16, stddev @ +24.
; Registers: rcx = i; xmm0 = sum then mean; xmm1 = min; xmm2 = max;
;            xmm3 = scratch; xmm4 = (double)n; xmm5 = sum of squared deviations.

default rel
global sp_stats_asm:function

SP_ERR_NULL  equ -1
SP_ERR_EMPTY equ -2

section .text
sp_stats_asm:
    test    rdi, rdi
    jz      .null
    test    rdx, rdx
    jz      .null
    test    rsi, rsi
    jz      .empty

    movsd   xmm1, [rdi]             ; min = x[0]
    movapd  xmm2, xmm1              ; max = x[0]
    xorpd   xmm0, xmm0              ; sum = 0
    xor     ecx, ecx
.pass1:
    movsd   xmm3, [rdi + rcx*8]     ; v = x[i]
    addsd   xmm0, xmm3              ; sum += v
    minsd   xmm1, xmm3              ; min = (min < v) ? min : v
    maxsd   xmm2, xmm3              ; max = (max > v) ? max : v
    inc     rcx
    cmp     rcx, rsi
    jb      .pass1

    cvtsi2sd xmm4, rsi              ; (double)n
    divsd   xmm0, xmm4              ; mean = sum / n

    xorpd   xmm5, xmm5              ; ss = 0
    xor     ecx, ecx
.pass2:
    movsd   xmm3, [rdi + rcx*8]
    subsd   xmm3, xmm0              ; d = x[i] - mean
    mulsd   xmm3, xmm3              ; d * d
    addsd   xmm5, xmm3              ; ss += d*d
    inc     rcx
    cmp     rcx, rsi
    jb      .pass2

    divsd   xmm5, xmm4              ; variance = ss / n
    sqrtsd  xmm5, xmm5              ; stddev

    movsd   [rdx],      xmm1        ; out->min
    movsd   [rdx + 8],  xmm2        ; out->max
    movsd   [rdx + 16], xmm0        ; out->mean
    movsd   [rdx + 24], xmm5        ; out->stddev
    xor     eax, eax
    ret
.null:
    mov     eax, SP_ERR_NULL
    ret
.empty:
    mov     eax, SP_ERR_EMPTY
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
