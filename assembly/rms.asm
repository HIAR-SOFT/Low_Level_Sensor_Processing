; int sp_rms_asm(const double *x, size_t n, double *out)
;
; System V AMD64 ABI:  rdi = x, rsi = n, rdx = out;  return int in eax.
; Registers used (all caller-saved, so no push/pop and no stack frame needed):
;   rcx  = loop index i          xmm0 = running sum
;   xmm1 = scratch (x[i], later n as double)
; Stack: this is a leaf function (calls nothing), so it never touches rsp
; beyond the implicit return address pushed by `call`.
; Memory addressing: [rdi + rcx*8] = base + index*scale, i.e. &x[i] for doubles.

default rel
global sp_rms_asm:function

SP_ERR_NULL  equ -1
SP_ERR_EMPTY equ -2

section .text
sp_rms_asm:
    test    rdi, rdi                ; x == NULL ?  (test sets ZF if rdi AND rdi == 0)
    jz      .null
    test    rdx, rdx                ; out == NULL ?
    jz      .null
    test    rsi, rsi                ; n == 0 ?
    jz      .empty

    xorpd   xmm0, xmm0              ; sum = 0.0 (xor with itself clears the register)
    xor     ecx, ecx                ; i = 0 (writing ecx zero-extends into rcx)
.loop:
    movsd   xmm1, [rdi + rcx*8]     ; xmm1 = x[i]  (load one double)
    mulsd   xmm1, xmm1              ; xmm1 = x[i] * x[i]
    addsd   xmm0, xmm1              ; sum += x[i]^2
    inc     rcx                     ; i++
    cmp     rcx, rsi                ; compare i with n
    jb      .loop                   ; jump if below (unsigned i < n)

    cvtsi2sd xmm1, rsi              ; xmm1 = (double)n  (integer -> double)
    divsd   xmm0, xmm1              ; sum / n
    sqrtsd  xmm0, xmm0              ; sqrt(sum / n)
    movsd   [rdx], xmm0             ; *out = result
    xor     eax, eax                ; return SP_OK (0)
    ret
.null:
    mov     eax, SP_ERR_NULL
    ret
.empty:
    mov     eax, SP_ERR_EMPTY
    ret

section .note.GNU-stack noalloc noexec nowrite progbits