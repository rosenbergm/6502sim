    .include "includes/debug.inc"
    .include "includes/print.inc"
    
    .org $0000
        
    .org $8000

start:
    ldx #0          ; initialize index
loop:
    lda message,x   ; load character from message
    beq done        ; if character is 0, we're done
    PRINT           ; write to output device
    inx             ; increment index
    jmp loop        ; continue loop
done:
    stp             ; stop execution

message:
    .byte "Hello World!", 0

    .org $fffc
    .word start