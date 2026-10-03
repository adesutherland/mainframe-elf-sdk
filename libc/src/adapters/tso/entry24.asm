* SPDX-License-Identifier: MIT
* Copyright (c) 2026 Adrian Sutherland
* Project-authored hosted TSO24 bridge. PDPCLIB entries stay native.
* Private, non-reentrant, synchronous invocation; no IBM LE dependency.
* Selected service register forms follow z-pdos/tso31-bridge/SERVICES.md.
LABTS24  CSECT
LABTS24  AMODE 24
LABTS24  RMODE 24
GMBELOW  EQU   16
FMCOND   EQU   1
SVCMEM   EQU   120
SVCTERM  EQU   93
SVCDYN   EQU   99
         STM   14,12,12(13)
         LR    12,15
         USING LABTS24,12
         ST    1,RAWPL
         ST    13,ORIGSA
         LA    11,ENTRYSA
         ST    13,4(11)
         ST    11,8(13)
         LR    13,11
         SR    0,0
         ST    0,STACKLO
         ST    0,OUTBUF
* BASR records zero in the high byte only while executing in AMODE24.
         BASR  2,0
         ST    2,MODEWORD
         TM    MODEWORD,X'FF'
         BNZ   STARTBAD
         L     0,=F'1048576'
         SR    1,1
         LA    15,GMBELOW
         SVC   SVCMEM
         LTR   15,15
         BNZ   STARTBAD
         ST    1,STACKLO
         CL    1,=F'16777216'
         BNL   STARTBAD
         A     1,=F'1048480'
         ST    1,CSTACK
         LA    0,256
         SR    1,1
         LA    15,GMBELOW
         SVC   SVCMEM
         LTR   15,15
         BNZ   STARTBAD
         ST    1,OUTBUF
         CL    1,=F'16777216'
         BNL   STARTBAD
         L     1,RAWPL
         L     1,0(1)
         N     1,=X'7FFFFFFF'
         LH    3,0(1)
         LA    2,2(1)
         LA    4,SVCTAB
         L     15,CSTACK
         L     1,=V(ELFPOC)
         BASR  14,1
* C preserves R12, but R13 belongs to C until restored here.
         B     CLEANUP
STARTBAD LA    2,20
CLEANUP  ST    2,RETCODE
         LA    13,ENTRYSA
         L     9,OUTBUF
         LTR   9,9
         BZ    NOBUFFER
         LA    0,256
         LR    1,9
         LA    15,FMCOND
         SVC   SVCMEM
NOBUFFER L     9,STACKLO
         LTR   9,9
         BZ    NOSTACK
         L     0,=F'1048576'
         LR    1,9
         LA    15,FMCOND
         SVC   SVCMEM
NOSTACK  L     15,RETCODE
         L     13,ORIGSA
         L     14,12(13)
         LM    0,12,20(13)
         BR    14
*
* Each callback saves C's R6-R15 before establishing native R12/R13.
* R2 is the C result. The static save area forbids nested callbacks.
         DROP  12
PUTLINE  BASR  1,0
PUTBASE  EQU   *
         USING PUTBASE,1
         STM   6,15,SVCSAVE
         L     12,NATBASE
         DROP  1
         USING LABTS24,12
         LA    13,ENTRYSA
         LR    8,3
         LTR   8,8
         BZ    PUTOK
         CL    8,=F'132'
         BH    PUTBAD
         L     9,OUTBUF
         LR    10,9
         LR    11,8
PUTCOPY  MVC   0(1,10),0(2)
         LA    10,1(10)
         LA    2,1(2)
         BCT   11,PUTCOPY
* Printable ASCII uses the inverse of libc/src/adapters/cms/text1047.h.
         C     4,=X'00524157'
         BE    PUTNTR
         TR    0(132,9),ATOE
PUTNTR   EQU   *
         LR    0,8
         LR    1,9
         SVC   SVCTERM
PUTOK    SR    2,2
         B     PUTRET
PUTBAD   LA    2,1
PUTRET   LM    6,15,SVCSAVE
         BR    14
         DROP  12
ALLOCATE BASR  1,0
ALCBASE  EQU   *
         USING ALCBASE,1
         STM   6,15,SVCSAVE
         L     12,NATBASE
         DROP  1
         USING LABTS24,12
         LA    13,ENTRYSA
         LR    0,2
         SR    1,1
         LA    15,GMBELOW
         SVC   SVCMEM
         LTR   15,15
         BNZ   ALCBAD
         CL    1,=F'16777216'
         BNL   ALCBAD
         LR    2,1
         B     ALCRET
ALCBAD   SR    2,2
ALCRET   LM    6,15,SVCSAVE
         BR    14
         DROP  12
RELEASE  BASR  1,0
RELBASE  EQU   *
         USING RELBASE,1
         STM   6,15,SVCSAVE
         L     12,NATBASE
         DROP  1
         USING LABTS24,12
         LA    13,ENTRYSA
         LR    0,3
         LR    1,2
         LA    15,FMCOND
         SVC   SVCMEM
         LR    2,15
         LM    6,15,SVCSAVE
         BR    14
         DROP  12
FILECALL BASR  1,0
FILBASE  EQU   *
         USING FILBASE,1
         STM   6,15,SVCSAVE
         L     12,NATBASE
         DROP  1
         USING LABTS24,12
         LA    13,ENTRYSA
         CL    2,=F'5'
         BE    FREEFILE
         BH    FILBAD
         LR    1,3
         SLL   2,2
         LA    4,FILESVC
         L     15,0(2,4)
         BALR  14,15
         LR    2,15
         B     FILRET
FILBAD   L     2,=F'-1'
FILRET   LM    6,15,SVCSAVE
         BR    14
* Only DDs obtained by this runtime are passed to this unallocator.
FREEFILE XC    FREERB(20),FREERB
         MVI   FREERB,20
         MVI   FREERB+1,2
         LA    4,FREETU
         O     4,=X'80000000'
         ST    4,FREETUP
         LA    4,FREETUP
         ST    4,FREERB+8
         MVC   FREETU+6(8),0(3)
         LA    4,FREERB
         O     4,=X'80000000'
         ST    4,FREEPTR
         LA    1,FREEPTR
         SVC   SVCDYN
         LR    2,15
         B     FILRET
         DROP  12
READLINE BASR  1,0
INBASE   EQU   *
         USING INBASE,1
         STM   6,15,SVCSAVE
         L     12,NATBASE
         DROP  1
         USING LABTS24,12
         LA    13,ENTRYSA
         LR    6,2
         LR    7,3
         LR    8,4
         SR    0,0
         ST    0,0(8)
         LTR   7,7
         BNP   INBAD
         CL    7,=F'256'
         BH    INBAD
         L     9,OUTBUF
* EDIT removes 3270 terminal controls. Application case is preserved.
         LR    0,7
         LR    1,9
         O     1,=X'80000000'
         SVC   SVCTERM
         LR    2,15
         LTR   2,2
         BZ    INCNT
         C     2,=F'12'
         BE    INCNT
         C     2,=F'24'
         BE    INCNT
         C     2,=F'28'
         BNE   INRET
INCNT    DS    0H
         CLR   1,7
         BH    INBAD
         ST    1,0(8)
         LR    10,1
         LTR   10,10
         BZ    INRET
INCOPY   MVC   0(1,6),0(9)
         LA    6,1(6)
         LA    9,1(9)
         BCT   10,INCOPY
         B     INRET
INBAD    LA    2,16
INRET    LM    6,15,SVCSAVE
         BR    14
         DROP  12
FINISH   BASR  1,0
FINBASE  EQU   *
         USING FINBASE,1
         L     12,NATBASE
         DROP  1
         USING LABTS24,12
         B     CLEANUP
NATBASE  DC    A(LABTS24)
SVCTAB   DC    F'6',A(PUTLINE),A(ALLOCATE),A(RELEASE)
         DC    A(FILECALL),A(FINISH),A(READLINE)
         DC    F'0'
FILESVC  DC    V(@@AOPEN),V(@@AREAD),V(@@AWRITE),V(@@ACLOSE)
         DC    V(@@DYNAL)
FREEPTR  DS    F
FREERB   DS    5F
FREETUP  DS    F
FREETU   DC    H'1',H'1',H'8',CL8' '
ORIGSA   DS    F
RAWPL    DS    F
STACKLO  DS    F
CSTACK   DS    F
OUTBUF   DS    F
RETCODE  DS    F
MODEWORD DS    F
ENTRYSA  DC    18F'0'
SVCSAVE  DS    10F
         LTORG
ATOE     DC    X'00010203372D2E2F1605250B0C0D0E0F'
         DC    X'101112133C3D322618193F271C1D1E1F'
         DC    X'405A7F7B5B6C507D4D5D5C4E6B604B61'
         DC    X'F0F1F2F3F4F5F6F7F8F97A5E4C7E6E6F'
         DC    X'7CC1C2C3C4C5C6C7C8C9D1D2D3D4D5D6'
         DC    X'D7D8D9E2E3E4E5E6E7E8E9ADE0BD5F6D'
         DC    X'79818283848586878889919293949596'
         DC    X'979899A2A3A4A5A6A7A8A9C04FD0A107'
         DC    X'202122232415061728292A2B2C090A1B'
         DC    X'30311A333435360838393A3B04143EFF'
         DC    X'41AA4AB19FB26AB5BBB49A8AB0CAAFBC'
         DC    X'908FEAFABEA0B6B39DDA9B8BB7B8B9AB'
         DC    X'6465626663679E687471727378757677'
         DC    X'AC69EDEEEBEFECBF80FDFEFBFCBAAE59'
         DC    X'4445424643479C485451525358555657'
         DC    X'8C49CDCECBCFCCE170DDDEDBDC8D8EDF'
         END   LABTS24
