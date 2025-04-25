	.file	1 "s0.c"
	.section .mdebug.abi32
	.previous
	.nan	legacy
	.module	fp=xx
	.module	nooddspreg
	.abicalls
	.text
	.rdata
	.align	2
$LC0:
	.ascii	"%d \000"
	.text
	.align	2
	.globl	main
	.set	nomips16
	.set	nomicromips
	.ent	main
	.type	main, @function
main:
	.frame	$fp,48,$31		# vars= 16, regs= 2/0, args= 16, gp= 8
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-48
	sw	$31,44($sp)
	sw	$fp,40($sp)
	move	$fp,$sp
	lui	$28,%hi(__gnu_local_gp)
	addiu	$28,$28,%lo(__gnu_local_gp)
	.cprestore	16
	lw	$2,%got(__stack_chk_guard)($28)
	lw	$2,0($2)
	sw	$2,36($fp)
	addiu	$2,$fp,28
	move	$5,$2
	lui	$2,%hi($LC0)
	addiu	$4,$2,%lo($LC0)
	lw	$2,%call16(scanf)($28)
	move	$25,$2
	.reloc	1f,R_MIPS_JALR,scanf
1:	jalr	$25
	nop

	lw	$28,16($fp)
	li	$4,256			# 0x100
	lw	$2,%call16(malloc)($28)
	move	$25,$2
	.reloc	1f,R_MIPS_JALR,malloc
1:	jalr	$25
	nop

	lw	$28,16($fp)
	sw	$2,32($fp)
	move	$2,$0
	move	$4,$2
	lw	$2,%got(__stack_chk_guard)($28)
	lw	$3,36($fp)
	lw	$2,0($2)
	beq	$3,$2,$L3
	nop

	lw	$2,%call16(__stack_chk_fail)($28)
	move	$25,$2
	.reloc	1f,R_MIPS_JALR,__stack_chk_fail
1:	jalr	$25
	nop

$L3:
	move	$2,$4
	move	$sp,$fp
	lw	$31,44($sp)
	lw	$fp,40($sp)
	addiu	$sp,$sp,48
	jr	$31
	nop

	.set	macro
	.set	reorder
	.end	main
	.size	main, .-main
	.ident	"GCC: (Ubuntu 10.5.0-4ubuntu2) 10.5.0"
	.section	.note.GNU-stack,"",@progbits
