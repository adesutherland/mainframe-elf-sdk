* SPDX-License-Identifier: MIT
* Copyright (c) 2026 Adrian Sutherland
* Hosted TSO31 bridge. PDPCLIB entries stay native.
* Private, non-reentrant, synchronous invocation; no IBM LE dependency.
* Selected explicit services follow z-pdos/tso31-bridge/src/entry31.asm.
LABTSO  CSECT
LABTSO  AMODE 31
LABTSO  RMODE ANY
GMANY    EQU   48
GMBELOW  EQU   16
FMCOND   EQU   1
SVCMEM   EQU   120
SVCTERM  EQU   93
SVCDYN   EQU   99
         STM   14,12,12(13)
         LR    12,15
         USING LABTSO,12
         ST    1,RAWPL
         ST    13,ORIGSA
         LA    11,ENTRYSA
         ST    13,4(11)
         ST    11,8(13)
         LR    13,11
         SR    0,0
         ST    0,STACKLO
         ST    0,OUTBUF
* Require problem-state AMODE31 at the actual TSO entry.
         EPSW  2,3
         ST    2,ENTRYPSW
         N     2,=X'00010001'
         C     2,=X'00010000'
         BNE   BADSTATE
         N     3,=X'80000000'
         BZ    BADSTATE
         L     0,=F'1048576'
         SR    1,1
         LA    15,GMANY
         SVC   SVCMEM
         LTR   15,15
         BNZ   STARTBAD
         ST    1,STACKLO
         A     1,=F'1048480'
         ST    1,CSTACK
         LA    0,256
         SR    1,1
         LA    15,GMBELOW
         SVC   SVCMEM
         LTR   15,15
         BNZ   STARTBAD
         ST    1,OUTBUF
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
         B     CLEANUP
BADSTATE LA    2,36
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
         USING LABTSO,12
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
         USING LABTSO,12
         LA    13,ENTRYSA
         LR    0,2
         SR    1,1
         LA    15,GMANY
         SVC   SVCMEM
         LTR   15,15
         BNZ   ALCBAD
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
         USING LABTSO,12
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
         USING LABTSO,12
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
         USING LABTSO,12
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
         USING LABTSO,12
         B     CLEANUP
* Called directly from C, in its mode. Preserve R6-R15.
         DROP  12
GETSTATE BASR  1,0
GSBASE   EQU   *
         USING GSBASE,1
         STM   6,15,SVCSAVE
         L     12,NATBASE
         DROP  1
         USING LABTSO,12
         EPSW  2,3
         N     2,=X'00010001'
         C     2,=X'00010000'
         BNE   GSBAD
         N     3,=X'80000000'
         BZ    GSBAD
         LA    2,31
         B     GSRET
GSBAD    SR    2,2
GSRET    LM    6,15,SVCSAVE
         BR    14
NATBASE  DC    A(LABTSO)
* C32 R2=name, R3=parameter list, R4=ENVBLOCK, R5=loader status.
* LOAD RETURN requested: absent services return an error instead of S806.
* Each successful LOAD is balanced by DELETE, even on service failure.
COMMANDC STM   6,15,24(15)
         LR    10,15
         BASR  12,0
CMDBASE  SH    15,CMDSTACK-CMDBASE(12)
         LA    13,96(15)
         XC    0(72,13),0(13)
* Chain the native frame to the entry save area; C's R13 is not a native SA.
         L     6,CMDENTRY-CMDBASE(12)
         ST    6,4(13)
         L     7,8(6)
         ST    7,72(13)
         ST    13,8(6)
         LR    6,2
         LR    7,3
         LR    8,4
         LR    9,5
         LR    0,6
         L     1,CMDRET-CMDBASE(12)
         SVC   8
         ST    15,0(9)
         LTR   15,15
         BNZ   CMDDONE-CMDBASE(12)
         LR    15,0
         LR    0,8
         LR    1,7
         DC    X'0CEF'       BASSM 14,15 (ESA/390 RR instruction)
         LR    8,15
         LR    0,6
         SR    1,1
         SVC   9
         ST    15,0(9)
         LR    15,8
CMDDONE  LR    2,15
         L     6,CMDENTRY-CMDBASE(12)
         L     7,72(13)
         ST    7,8(6)
         LM    6,15,24(10)
         BR    14
         DS    0F
CMDRET   DC    X'80000000'
CMDENTRY DC    A(ENTRYSA)
CMDSTACK DC    H'256'
SVCTAB   DC    F'7',A(PUTLINE),A(ALLOCATE),A(RELEASE)
         DC    A(FILECALL),A(FINISH),A(READLINE),A(GETSTATE)
         DC    A(COMMANDC)
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
ENTRYPSW DS    F
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
         END   LABTSO
