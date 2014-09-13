	.file	"handle.c"
	.section	.rodata
.LC0:
	.string	"Nice try.\n"
	.text
	.globl	handler
	.type	handler, @function
handler:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movl	$1, -4(%rbp)
	movl	-4(%rbp), %eax
	movl	$10, %edx
	movl	$.LC0, %esi
	movl	%eax, %edi
	call	write
	movq	%rax, -16(%rbp)
	cmpq	$10, -16(%rbp)
	je	.L1
	movl	$-999, %edi
	call	exit
.L1:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	handler, .-handler
	.section	.rodata
.LC1:
	.string	"exiting\n"
	.text
	.globl	sigusr1_handler
	.type	sigusr1_handler, @function
sigusr1_handler:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movl	$1, -4(%rbp)
	movl	-4(%rbp), %eax
	movl	$10, %edx
	movl	$.LC1, %esi
	movl	%eax, %edi
	call	write
	movq	%rax, -16(%rbp)
	cmpq	$10, -16(%rbp)
	je	.L4
	movl	$-999, %edi
	call	exit
.L4:
	movl	$1, %edi
	call	exit
	.cfi_endproc
.LFE1:
	.size	sigusr1_handler, .-sigusr1_handler
	.section	.rodata
.LC2:
	.string	"This process's ID is %d\n"
.LC3:
	.string	"Still here"
	.text
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$64, %rsp
	movl	%edi, -52(%rbp)
	movq	%rsi, -64(%rbp)
	call	getpid
	movl	%eax, -8(%rbp)
	movl	$.LC2, %ecx
	movq	stdout(%rip), %rax
	movl	-8(%rbp), %edx
	movq	%rcx, %rsi
	movq	%rax, %rdi
	movl	$0, %eax
	call	fprintf
	movl	$handler, %esi
	movl	$2, %edi
	call	Signal
	movl	$sigusr1_handler, %esi
	movl	$10, %edi
	call	Signal
	movq	$1, -48(%rbp)
	movq	$0, -40(%rbp)
	jmp	.L7
.L8:
	nop
.L7:
	leaq	-32(%rbp), %rdx
	leaq	-48(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	nanosleep
	movl	%eax, -4(%rbp)
	movl	$.LC3, %edi
	call	puts
	cmpl	$-1, -4(%rbp)
	jne	.L8
	leaq	-32(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	nanosleep
	jmp	.L8
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3"
	.section	.note.GNU-stack,"",@progbits
