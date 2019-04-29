
.text

#ifdef __APPLE__
#include "arm_arch_common_macro.S"

#define  XCORR_KERNEL_NOEN   _xcorr_kernel_neon
#else
.include "arm_arch_common_macro_android.S"
.equ  XCORR_KERNEL_NOEN,  xcorr_kernel_neon
#endif

WELS_ASM_FUNC_BEGIN xcorr_kernel_neon
VLD1.16      {d5}, [r5]!
  SUBS         r12, r3, #8
  BLE xcorr_kernel_neon_process4
xcorr_kernel_neon_process8:
  VLD1.16      {d6, d7}, [r4]!
  VAND         d3, d5, d5
  SUBS         r12, r12, #8
  VLD1.16      {d4, d5}, [r5]!
  VMLAL.S16    q0, d3, d6[0]
  VEXT.16      d16, d3, d4, #1
  VMLAL.S16    q0, d4, d7[0]
  VEXT.16      d17, d4, d5, #1
  VMLAL.S16    q0, d16, d6[1]
  VEXT.16      d16, d3, d4, #2
  VMLAL.S16    q0, d17, d7[1]
  VEXT.16      d17, d4, d5, #2
  VMLAL.S16    q0, d16, d6[2]
  VEXT.16      d16, d3, d4, #3
  VMLAL.S16    q0, d17, d7[2]
  VEXT.16      d17, d4, d5, #3
  VMLAL.S16    q0, d16, d6[3]
  VMLAL.S16    q0, d17, d7[3]
  BGT xcorr_kernel_neon_process8
xcorr_kernel_neon_process4:
  ADDS         r12, r12, #4
  BLE xcorr_kernel_neon_process2
  VLD1.16      d6, [r4]!
  VAND         d4, d5, d5
  SUB          r12, r12, #4
  VLD1.16      d5, [r5]!
  VMLAL.S16    q0, d4, d6[0]
  VEXT.16      d16, d4, d5, #1
  VMLAL.S16    q0, d16, d6[1]
  VEXT.16      d16, d4, d5, #2
  VMLAL.S16    q0, d16, d6[2]
  VEXT.16      d16, d4, d5, #3
  VMLAL.S16    q0, d16, d6[3]
xcorr_kernel_neon_process2:
  ADDS         r12, r12, #2
  BLE xcorr_kernel_neon_process1
  VLD2.16      {d6[],d7[]}, [r4]!
  VAND         d4, d5, d5
  SUB          r12, r12, #2
  VLD1.32      {d5[]}, [r5]!
  VMLAL.S16    q0, d4, d6
  VEXT.16      d16, d4, d5, #1
  VSRI.64      d5, d4, #32
  VMLAL.S16    q0, d16, d7
xcorr_kernel_neon_process1:
  VLD1.16      {d6[]}, [r4]!
  ADDS         r12, r12, #1
  VMLAL.S16    q0, d5, d6
  MOVLE        pc, lr
  VLD1.16      {d4[]}, [r5]!
  VSRI.64      d4, d5, #16
  VLD1.16      {d6[]}, [r4]!
  VMLAL.S16    q0, d4, d6
  MOV          pc, lr


WELS_ASM_FUNC_BEGIN celt_pitch_xcorr_neon
  STMFD        sp!, {r4-r6, lr}
  LDR          r6, [sp, #16]
  VMOV.S32     q15, #1
  SUBS         r6, r6, #4
  BLT celt_pitch_xcorr_neon_process4_done
celt_pitch_xcorr_neon_process4:
  MOV          r4, r0
  MOV          r5, r1
  VEOR         q0, q0, q0
BL XCORR_KERNEL_NOEN
  SUBS         r6, r6, #4
  VST1.32      {q0}, [r2]!
  ADD          r1, r1, #8
  VMAX.S32     q15, q15, q0
  BGE celt_pitch_xcorr_neon_process4
celt_pitch_xcorr_neon_process4_done:
  ADDS         r6, r6, #4
  VMAX.S32     d30, d30, d31
  VPMAX.S32    d30, d30, d30
  BLE celt_pitch_xcorr_neon_done

celt_pitch_xcorr_neon_process_remaining:
  MOV          r4, r0
  MOV          r5, r1
  VMOV.I32     q0, #0
  SUBS         r12, r3, #8
  BLT celt_pitch_xcorr_neon_process_remaining4
celt_pitch_xcorr_neon_process_remaining_loop8:

  VLD1.16      {q1}, [r4]!

  VLD1.16      {q2}, [r5]!
  SUBS         r12, r12, #8
  VMLAL.S16    q0, d4, d2
  VMLAL.S16    q0, d5, d3
  BGE celt_pitch_xcorr_neon_process_remaining_loop8

celt_pitch_xcorr_neon_process_remaining4:
  ADDS         r12, r12, #4
  BLT celt_pitch_xcorr_neon_process_remaining4_done
VLD1.16      {d2}, [r4]!

  VLD1.16      {d3}, [r5]!
  SUB          r12, r12, #4
  VMLAL.S16    q0, d3, d2
celt_pitch_xcorr_neon_process_remaining4_done:

  VADD.S32     d0, d0, d1
  VPADDL.S32   d0, d0
  ADDS         r12, r12, #4
  BLE celt_pitch_xcorr_neon_process_remaining_loop_done
celt_pitch_xcorr_neon_process_remaining_loop1:
  VLD1.16      {d2[]}, [r4]!
  VLD1.16      {d3[]}, [r5]!
  SUBS         r12, r12, #1
  VMLAL.S16    q0, d2, d3
  BGT celt_pitch_xcorr_neon_process_remaining_loop1
celt_pitch_xcorr_neon_process_remaining_loop_done:
  VST1.32      {d0[0]}, [r2]!
  VMAX.S32     d30, d30, d0
  SUBS         r6, r6, #1
ADD          r1, r1, #2
BGT celt_pitch_xcorr_neon_process_remaining
celt_pitch_xcorr_neon_done:
  VMOV.32      r0, d30[0]
  LDMFD        sp!, {r4-r6, lr}
WELS_ASM_FUNC_END

