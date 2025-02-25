    .include "includes/debug.inc"
    .include "includes/print.inc"
    
    .org $0000
        
    .org $8000

start:
    lda #1
    brk
    stp

int:
    DBGBREAK
    lda #$BA
    DBGBREAK
    rti

    .org $fffc  ; reset vector
    .word start
    .org $fffe  ; interrupt handler
    .word int