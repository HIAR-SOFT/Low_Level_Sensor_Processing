; int sp_peak_detect_asm(const double *x, size_t n, double threshold,
;                        size_t *indices, size_t max_indices, size_t *count_out)
;
; ABI: integer args and float args are assigned INDEPENDENTLY:
;   rdi = x, rsi = n, rdx = indices, rcx = max_indices, r8 = count_out
;   xmm0 = threshold (first floating-point argument)
; Registers: r9 = i, r10 = count, xmm1 = scratch, xmm2 = abs mask.

default rel
global sp_peak_detect_asm:function

SP_ERR_NULL  equ -1
SP_ERR_EMPTY equ -2
SP_ERR_PARAM equ -3

section .rodata
align 16
abs_mask: dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF   ; clears the sign bit

section .text
sp_peak_detect_asm:
    test    rdi, rdi
    jz      .null
    test    r8, r8
    jz      .null
    test    rsi, rsi
    jz      .empty

    xorpd   xmm1, xmm1              ; xmm1 = 0.0
    ucomisd xmm0, xmm1              ; compare threshold with 0 (sets CF/ZF/PF)
    jp      .param                  ; PF=1 means "unordered": threshold is NaN
    jb      .param                  ; CF=1 means threshold < 0

    test    rdx, rdx                ; indices == NULL ?
    jnz     .have_buf
    xor     ecx, ecx                ; then treat max_indices as 0
.have_buf:
    movsd   xmm2, [abs_mask]        ; RIP-relative load of the mask
    xor     r9d, r9d                ; i = 0
    xor     r10d, r10d              ; count = 0
.loop:
    movsd   xmm1, [rdi + r9*8]      ; x[i]
    andpd   xmm1, xmm2              ; |x[i]|  (bitwise AND clears the sign bit)
    comisd  xmm1, xmm0              ; compare |x[i]| with threshold
    jbe     .next                   ; CF=1 or ZF=1: |x| <= thr, or NaN (NaN sets CF=ZF=PF=1)
    cmp     r10, rcx                ; room left in indices[] ?
    jae     .nostore
    mov     [rdx + r10*8], r9       ; indices[count] = i
.nostore:
    inc     r10                     ; count++
.next:
    inc     r9
    cmp     r9, rsi
    jb      .loop

    mov     [r8], r10               ; *count_out = count
    xor     eax, eax
    ret
.null:
    mov     eax, SP_ERR_NULL
    ret
.empty:
    mov     eax, SP_ERR_EMPTY
    ret
.param:
    mov     eax, SP_ERR_PARAM
    ret

section .note.GNU-stack noalloc noexec nowrite progbits