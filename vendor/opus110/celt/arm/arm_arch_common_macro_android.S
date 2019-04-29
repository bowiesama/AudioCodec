.macro WELS_ASM_FUNC_BEGIN funcName
.align 2
.arm
.global \funcName
\funcName:
.endm



.macro WELS_ASM_FUNC_END
mov pc, lr
.endm
