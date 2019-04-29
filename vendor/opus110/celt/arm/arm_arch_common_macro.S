
.macro WELS_ASM_FUNC_BEGIN
.align 2
.arm
.globl _$0
_$0:
.endm

.macro WELS_ASM_FUNC_END
mov pc, lr
.endm
