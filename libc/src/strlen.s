    .text
    .globl strlen
strlen:
    pushq %rbp
	movq %rsp, %rbp
    xor %rax, %rax
    jmp .L2
.L1:
    addq $1, %rax
    addq $1, %rdi
.L2:
    cmpb $0, (%rdi)
    jne .L1
    leave
    ret
