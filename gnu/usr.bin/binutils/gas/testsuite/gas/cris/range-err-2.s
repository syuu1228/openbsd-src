; Test error cases for constant ranges.

;  { dg-do assemble { target cris-*-* } }

 .text
start:
 moveq external_symbol,r0 ; { dg-error "Semantics error" }
 addq external_symbol,r0 ; { dg-error "Semantics error" }
 break external_symbol ; { dg-error "Semantics error" }

