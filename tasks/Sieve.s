.text
.globl sieve
sieve:
	pushq %rbp
	movq %rsp, %rbp
	cmpl $0, %edi
	jle .Lbb2
	movb $0, (%rsi)
.Lbb2:
	cmpl $1, %edi
	jg .Lbb4
	movl $0, %eax
	jmp .Lbb5
.Lbb4:
	movq %rsi, %rax
	addq $1, %rax
	movb $0, 1(%rsi)
.Lbb5:
	movl $2, %ecx
.Lbb6:
	cmpl %edi, %ecx
	jge .Lbb8
	movq %rax, %rdx
	addq $1, %rax
	movb $1, 1(%rdx)
	addl $1, %ecx
	jmp .Lbb6
.Lbb8:
	movq %rsi, %rdx
	addq $1, %rdx
	movl $2, %ecx
.Lbb10:
	cmpl %edi, %ecx
	jge .Lbb16
	movq %rdx, %rax
	addq $1, %rdx
	movzbl 1(%rax), %eax
	cmpl $1, %eax
	jnz .Lbb15
	movl %ecx, %eax
	imull %ecx, %eax
.Lbb13:
	cmpl %edi, %eax
	jge .Lbb15
	movslq %eax, %r8
	movb $0, (%rsi, %r8, 1)
	addl %ecx, %eax
	jmp .Lbb13
.Lbb15:
	addl $1, %ecx
	jmp .Lbb10
.Lbb16:
	movl $0, %eax
	leave
	ret
/* end function sieve */

