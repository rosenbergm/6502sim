  .macro DBGBREAK
    .byte $2
  .endm

  .macro stp
    .byte $DB
  .endm
