; int sp_moving_average_asm(const double *x, size_t n, size_t window, double *out)
;
; ABI: rdi = x, rsi = n, rdx = window, rcx = out;  return int in eax.
; Registers:
;   r8  = index (j in the init loop, i in the sliding loop)
;   r9  = n - window  (last output index)
;   r10 = i + window - 1  (index of the sample entering the window)
;   xmm0 = running sum    xmm1 = scratch    xmm2 = (double)window
; All caller-saved -> no stack frame. Leaf function.

default rel
global sp_moving_average_asm:function

SP_ERR_NULL  equ -1
SP_ERR_EMPTY equ -2
SP_ERR_PARAM equ -3

section .text
sp_moving_average_asm:
    test    rdi, rdi
    jz      .null
    test    rcx, rcx
    jz      .null
    test    rsi, rsi
    jz      .empty
    test    rdx, rdx                ; window == 0 ?
    jz      .param
    cmp     rdx, rsi                ; window > n ?  (unsigned compare)
    ja      .param

    ; ---- sum of the first window ----
    xorpd   xmm0, xmm0              ; sum = 0
    xor     r8d, r8d                ; j = 0
.init:
    addsd   xmm0, [rdi + r8*8]      ; sum += x[j]  (memory operand: no separate load)
    inc     r8
    cmp     r8, rdx
    jb      .init

    cvtsi2sd xmm2, rdx              ; xmm2 = (double)window
    movapd  xmm1, xmm0              ; copy sum (movapd copies a whole XMM register)
    divsd   xmm1, xmm2              ; sum / window
    movsd   [rcx], xmm1             ; out[0]

    ; ---- slide the window ----
    mov     r9, rsi
    sub     r9, rdx                 ; r9 = n - window  (last valid output index)
    mov     r8d, 1                  ; i = 1
.slide:
    cmp     r8, r9
    ja      .done                   ; stop when i > n - window
    lea     r10, [r8 + rdx - 1]     ; r10 = i + window - 1  (lea = address math, no memory access)
    movsd   xmm1, [rdi + r10*8]     ; x[i + window - 1]  (entering sample)
    subsd   xmm1, [rdi + r8*8 - 8]  ; minus x[i - 1]     (leaving sample)
    addsd   xmm0, xmm1              ; sum += (in - out)
    movapd  xmm1, xmm0
    divsd   xmm1, xmm2              ; sum / window
    movsd   [rcx + r8*8], xmm1      ; out[i]
    inc     r8
    jmp     .slide
.done:
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