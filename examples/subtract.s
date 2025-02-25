    .include "includes/debug.inc"
    .include "includes/print.inc"
    
    .org $0000
        
    .org $8000

start:
    ; Test Case 1: Basic subtraction (positive result)
    ; 5 - 3 = 2
    LDA #$05    ; A = 5
    SEC         ; Set carry (required for proper subtraction)
    SBC #$03    ; A = A - 3
    ; Expected: A = $02
    ; Flags: N=0, V=0, Z=0, C=1

    DBGBREAK

    ; Test Case 2: Subtraction resulting in zero
    ; 5 - 5 = 0
    LDA #$05    ; A = 5
    SEC         ; Set carry
    SBC #$05    ; A = A - 5
    ; Expected: A = $00
    ; Flags: N=0, V=0, Z=1, C=1

    DBGBREAK

    ; Test Case 3: Subtraction resulting in negative
    ; 5 - 6 = -1
    LDA #$05    ; A = 5
    SEC         ; Set carry
    SBC #$06    ; A = A - 6
    ; Expected: A = $FF
    ; Flags: N=1, V=0, Z=0, C=0

    DBGBREAK

    ; Test Case 4: Testing borrow (carry clear)
    ; 5 - 3 - 1 = 1 (carry clear subtracts an extra 1)
    LDA #$05    ; A = 5
    CLC         ; Clear carry (adding a borrow)
    SBC #$03    ; A = A - 3 - 1
    ; Expected: A = $01
    ; Flags: N=0, V=0, Z=0, C=1

    DBGBREAK

    ; Test Case 5: Overflow case (positive to negative)
    ; 127 - (-1) = 128 (overflow because result > 127)
    LDA #$7F    ; A = 127
    SEC         ; Set carry
    SBC #$FF    ; A = A - (-1)
    ; Expected: A = $80
    ; Flags: N=1, V=1, Z=0, C=0

    DBGBREAK

    ; Test Case 6: Overflow case (negative to positive)
    ; -128 - 1 = -129 (overflow because result < -128)
    LDA #$80    ; A = -128
    SEC         ; Set carry
    SBC #$01    ; A = A - 1
    ; Expected: A = $7F
    ; Flags: N=0, V=1, Z=0, C=1

    DBGBREAK

    ; Test Case 7: No overflow case
    ; -2 - 126 = -128 (no overflow, result is valid negative)
    LDA #$FE    ; A = -2
    SEC         ; Set carry
    SBC #$7E    ; A = A - 126
    ; Expected: A = $80
    ; Flags: N=1, V=0, Z=0, C=1

    DBGBREAK

    ; Test Case 8: Borrow and negative
    ; 0 - 1 = -1
    LDA #$00    ; A = 0
    SEC         ; Set carry
    SBC #$01    ; A = A - 1
    ; Expected: A = $FF
    ; Flags: N=1, V=0, Z=0, C=0

    DBGBREAK

    ; Test Case 9: Max unsigned value
    ; 255 - 255 = 0
    LDA #$FF    ; A = 255
    SEC         ; Set carry
    SBC #$FF    ; A = A - 255
    ; Expected: A = $00
    ; Flags: N=0, V=0, Z=1, C=1

    DBGBREAK

    ; Test Case 10: All bits set with borrow
    ; 255 - 255 - 1 = -1
    LDA #$FF    ; A = 255
    CLC         ; Clear carry (adding a borrow)
    SBC #$FF    ; A = A - 255 - 1
    ; Expected: A = $FF
    ; Flags: N=1, V=0, Z=0, C=0

    DBGBREAK

    stp

    .org $fffc
    .word start