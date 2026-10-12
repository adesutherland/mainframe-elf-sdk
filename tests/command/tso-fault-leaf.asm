* Intentional native TSO31 containment control. No child GETMAIN, C stack,
* file or heap is allocated. Run only through reviewed IKJEFTSR CALL;
* this is not a CMSCALL containment test or an ordinary application entry.
MFFLT31  CSECT
MFFLT31  AMODE 31
MFFLT31  RMODE ANY
         DC    X'0000'
         END   MFFLT31
