# REQUIRES: avr
# RUN: llvm-mc -filetype=obj -triple=avr-unknown-linux -mcpu=attiny40 %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe -Ttext=40
# RUN: llvm-objdump -d %t.exe | FileCheck %s

main:
  lds r17, foo
foo:
  rjmp foo

# CHECK:      main:
# CHECK-NEXT:  40: 12 a1 <unknown>
# CHECK:      foo:
# CHECK-NEXT:  42: ff cf <unknown>
