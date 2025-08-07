    .text
    .globl __ecc_lsys_mmap
__ecc_lsys_mmap:
    pushq %rbp
	movq %rsp, %rbp
    movq %rcx, %r10
    movq $9, %rax
    syscall
    leave
    ret
