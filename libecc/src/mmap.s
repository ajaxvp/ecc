    .text
    .globl __ecc_lsys_mmap
__ecc_lsys_mmap:
    pushq %rbp
	movq %rsp, %rbp
    movq %r10, %rcx
    movq $9, %rax
    syscall
    leave
    ret
