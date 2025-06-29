    .text
    .globl putchar
putchar:
    pushq %rbp
	movq %rsp, %rbp
    subq $16, %rsp
    movb %dil, -1(%rbp)
    leaq -1(%rbp), %rsi
    movq $1, %rdx
    movq $1, %rax
    movq $1, %rdi
    syscall
    leave
    ret
