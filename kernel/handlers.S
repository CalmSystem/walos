#define SERV_EXC_TASK(id) \
	.text; \
trap##id: \
	movq	$(0), %rsi; \
	movq	$(id), %rdi; \
	cld;	/* for gcc */ \
	call	trap_handler; \
	iretq; \
	jmp	trap##id; \
	.data; \
	.quad	trap##id

#define SERV_ERR_TASK(id) \
	.text; \
trap##id: \
	popq	%rsi; \
	movq	$(id), %rdi; \
	cld;	/* for gcc */ \
	call	trap_handler; \
	iretq; \
	jmp	trap##id; \
	.data; \
	.quad	trap##id

	.data
.globl exception_handlers
exception_handlers:
SERV_EXC_TASK(0)
SERV_EXC_TASK(1)
SERV_EXC_TASK(2)
SERV_EXC_TASK(3)
SERV_EXC_TASK(4)
SERV_EXC_TASK(5)
SERV_EXC_TASK(6)
SERV_EXC_TASK(7)
SERV_ERR_TASK(8)
SERV_EXC_TASK(9)
SERV_ERR_TASK(10)
SERV_ERR_TASK(11)
SERV_ERR_TASK(12)
SERV_ERR_TASK(13)
SERV_ERR_TASK(14)
SERV_EXC_TASK(15)
SERV_EXC_TASK(16)
SERV_ERR_TASK(17)
SERV_EXC_TASK(18)
SERV_EXC_TASK(19)
SERV_EXC_TASK(20)
SERV_EXC_TASK(21)
SERV_EXC_TASK(22)
SERV_EXC_TASK(23)
SERV_EXC_TASK(24)
SERV_EXC_TASK(25)
SERV_EXC_TASK(26)
SERV_EXC_TASK(27)
SERV_EXC_TASK(28)
SERV_EXC_TASK(29)
SERV_EXC_TASK(30)
SERV_EXC_TASK(31)

# Interrupt handlers
#define IT_HANDLER(kind, target) \
	.globl IT_##kind##_handler; \
IT_##kind##_handler: \
/* save important registers */ \
	pushq	%rdi; \
	pushq	%rsi; \
	pushq	%rdx; \
	pushq	%rcx; \
	pushq	%rax; \
	pushq	 %r8; \
	pushq	 %r9; \
	pushq	%r10; \
	pushq	%r11; \
/* call C function dealing with interrupt */ \
	call target; \
/* restore important registers */ \
	popq	%r11; \
	popq	%r10; \
	popq	 %r9; \
	popq	 %r8; \
	popq	%rax; \
	popq	%rcx; \
	popq	%rdx; \
	popq	%rsi; \
	popq	%rdi; \
/* end interrupt handler */ \
	iretq

IT_HANDLER(PIT, tic_PIT)
