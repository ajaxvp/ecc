    .text
    .globl puts
puts:
	pushq %rbp
	movq %rsp, %rbp
    movq %rdi, %rsi
    call strlen
    movq %rax, %rdx
    movq $1, %rax
    movq $1, %rdi
    syscall
    leave
    ret
