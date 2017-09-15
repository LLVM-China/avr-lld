# REQUIRES: avr
# RUN: llvm-mc -filetype=obj -triple=avr-unknown-linux -mcpu=atmega328p %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe -Ttext=6
# RUN: llvm-objdump -d %t.exe | FileCheck %s

main:
  call foo
  .word gs(foo)
  .byte foo
  .byte lo8(foo)
  .byte hi8(foo)
  .byte hlo8(foo)
  ldi r16, lo8(foo)
  ldi r17, hi8(foo)
  ldi r18, hh8(foo)
  ldi r19, hhi8(foo)
  ldi r20, foo
  ldi r21, lo8(-(foo))
  ldi r22, hi8(-(foo))
  ldi r23, hh8(-(foo))
  ldi r24, hhi8(-(foo))
  ldi r25, pm_lo8(foo)
  ldi r26, pm_hi8(foo)
  ldi r27, pm_hh8(foo)
  ldi r28, pm_lo8(-(foo))
  ldi r29, pm_hi8(-(foo))
  ldi r30, pm_hh8(-(foo))
  brne foo
  rjmp foo
foo:
  ldd r2, Y+2
  ldd r0, Y+0
  ldd r9, Z+12
  ldd r7, Z+30
  ldd r9, Z+foo
  lds r16, foo
  ldi r17, lo8(gs(foo))
  ldi r18, hi8(gs(foo))
  in r20, foo+1
  sbi main, 0
  adiw r30, foo

# CHECK:      main:
# CHECK-NEXT:   6: 0e 94 19 00 <unknown>
# CHECK-NEXT:   a: 19 00 <unknown>
# CHECK-NEXT:   c: 32 32 <unknown>
# CHECK-NEXT:   e: 00 00 <unknown>
# CHECK-NEXT:  10: 02 e3 <unknown>
# CHECK-NEXT:  12: 10 e0 <unknown>
# CHECK-NEXT:  14: 20 e0 <unknown>
# CHECK-NEXT:  16: 30 e0 <unknown>
# CHECK-NEXT:  18: 42 e3 <unknown>
# CHECK-NEXT:  1a: 5e ec <unknown>
# CHECK-NEXT:  1c: 6f ef <unknown>
# CHECK-NEXT:  1e: 7f ef <unknown>
# CHECK-NEXT:  20: 8f ef <unknown>
# CHECK-NEXT:  22: 99 e1 <unknown>
# CHECK-NEXT:  24: a0 e0 <unknown>
# CHECK-NEXT:  26: b0 e0 <unknown>
# CHECK-NEXT:  28: c7 ee <unknown>
# CHECK-NEXT:  2a: df ef <unknown>
# CHECK-NEXT:  2c: ef ef <unknown>
# CHECK-NEXT:  2e: 09 f4 <unknown>
# CHECK-NEXT:  30: 00 c0 <unknown>
# CHECK:      foo:
# CHECK-NEXT:  32: 2a 80 <unknown>
# CHECK-NEXT:  34: 08 80 <unknown>
# CHECK-NEXT:  36: 94 84 <unknown>
# CHECK-NEXT:  38: 76 8c <unknown>
# CHECK-NEXT:  3a: 92 a8 <unknown>
# CHECK-NEXT:  3c: 00 91 32 00 <unknown>
# CHECK-NEXT:  40: 19 e1 <unknown>
# CHECK-NEXT:  42: 20 e0 <unknown>
# CHECK-NEXT:  44: 43 b7 <unknown>
# CHECK-NEXT:  46: 30 9a <unknown>
# CHECK-NEXT:  48: f2 96 <unknown>
