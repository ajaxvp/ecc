    .text
    .globl puts
puts:
	pushq %rbp
	movq %rsp, %rbp
    subq $16, %rsp
    movq %rdi, %rsi
    call strlen
    movq %rax, %rdx
    movq $1, %rax
    movq $1, %rdi
    syscall
    movw $0xA, -2(%rbp)
    leaq -2(%rbp), %rsi
    movq $1, %rdx
    movq $1, %rax
    movq $1, %rdi
    syscall
    leave
    ret
