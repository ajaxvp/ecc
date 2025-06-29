    .text
    .globl _start
_start:
    xorl %ebp, %ebp
    movl (%rsp), %edi
    leaq 8(%rsp), %rsi
    leaq 16(%rsp, %rdi, 8), %rdx
    xorl %eax, %eax
    call main
    movl %eax, %edi
    movq $0x3c, %rax
    syscall
