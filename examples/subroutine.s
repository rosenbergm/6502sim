    .include "includes/debug.inc"
    .include "includes/print.inc"
    
    .org $0000
        
    .org $8000

start:
    lda #1
    jsr reset_a
    stp

reset_a:
    DBGBREAK
    lda #$BA
    DBGBREAK
    rts

    .org $fffc  ; reset vector
    .word start
    