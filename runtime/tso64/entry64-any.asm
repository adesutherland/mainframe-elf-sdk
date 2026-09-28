* Beta3 LP64 entry. TPUT/DCB/OPEN control storage remains belowline.
* Private, non-reentrant, synchronous invocation; no IBM LE dependency.
LABTS64  CSECT
LABTS64  AMODE 64
LABTS64  RMODE ANY
* Preserve full caller GPRs before the PSW probe. R13 addresses the
* standard caller save area; slots 16 and 24 are scratch until STM.
         STG   0,16(13)
         STG   1,24(13)
         LARL  1,ENTRY64SAVE
         STMG  2,15,16(1)
         LG    0,16(13)
         STG   0,0(1)
         LG    0,24(13)
         STG   0,8(1)
         LG    0,0(1)
         LG    1,8(1)
         STM   14,12,12(13)
* Capture true entry PSW before using 31-bit OS services.
         EPSW  2,3
         SAM31
* AMODE64 CALL did not supply an entry-address base in R15 on z/OS 1.5.
         LARL  12,LABTS64
         USING LABTS64,12
         ST    1,RAWPL
         ST    13,ORIGSA
         LA    11,ENTRYSA
         ST    13,4(11)
         ST    11,8(13)
         LR    13,11
         SR    0,0
         ST    0,STACKLO
         ST    0,OUTBUF
         ST    0,WORKLO
         ST    2,ENTRYPSW
         N     2,=X'00010001'
         C     2,=X'00010001'
         BNE   BADSTATE
         GETMAIN RC,LV=1048576,LOC=ANY
         LTR   15,15
         BNZ   STARTBAD
         ST    1,STACKLO
         A     1,=F'1048416'
         ST    1,CSTACK
         GETMAIN RC,LV=256,LOC=BELOW
         LTR   15,15
         BNZ   STARTBAD
         ST    1,OUTBUF
         GETMAIN RC,LV=65536,LOC=ANY
         LTR   15,15
         BNZ   STARTBAD
         ST    1,WORKLO
         LLGFR 1,1
         STG   1,WORKPTR
         L     1,RAWPL
         L     1,0(1)
         N     1,=X'7FFFFFFF'
         LH    3,0(1)
         LA    2,2(1)
         LA    4,SVCTAB
         L     15,CSTACK
         L     1,=V(ELFPOC)
         LLGFR 1,1
         LLGFR 2,2
         LLGFR 3,3
         LLGFR 4,4
         LLGFR 12,12
         LLGFR 15,15
         SAM64
         BASR  14,1
         SAM31
* C preserves R12, but R13 belongs to C until restored here.
         B     CLEANUP
STARTBAD LA    2,20
         B     CLEANUP
BADSTATE LA    2,36
CLEANUP  ST    2,RETCODE
         LA    13,ENTRYSA
         L     9,WORKLO
         LTR   9,9
         BZ    NOWORK
         FREEMAIN RC,LV=65536,A=(9)
NOWORK   L     9,OUTBUF
         LTR   9,9
         BZ    NOBUFFER
         FREEMAIN RC,LV=256,A=(9)
NOBUFFER L     9,STACKLO
         LTR   9,9
         BZ    NOSTACK
         FREEMAIN RC,LV=1048576,A=(9)
NOSTACK  LARL  15,ENTRY64SAVE
         LMG   0,14,0(15)
         LARL  15,RETCODE
         L     15,0(15)
         BR    14
*
* Each callback saves C's R6-R15 before establishing native R12/R13.
* R2 is the C result. The static save area forbids nested callbacks.
         DROP  12
PUTLINE  LARL  1,SVCSAVE
         STMG  6,15,0(1)
         LARL  12,LABTS64
         USING LABTS64,12
         LA    13,ENTRYSA
         LR    8,3
         LTR   8,8
         BZ    PUTOK
         CL    8,=F'132'
         BH    PUTBAD
         LLGF  9,OUTBUF
         LGR   10,9
         LR    11,8
PUTCOPY  MVC   0(1,10),0(2)
         LA    10,1(10)
         LA    2,1(2)
         BCT   11,PUTCOPY
* Printable ASCII uses the inverse of runtime/cms/text1047.h.
         TR    0(132,9),ATOE
         SAM31
         TPUT  (9),(8)
         SAM64
         LARL  12,LABTS64
PUTOK    SR    2,2
         B     PUTRET
PUTBAD   LA    2,1
PUTRET   LARL  1,SVCSAVE
         LMG   6,15,0(1)
         BR    14
         DROP  12
ALLOCATE LARL  1,SVCSAVE
         STMG  6,15,0(1)
         LARL  12,LABTS64
         USING LABTS64,12
         LA    13,ENTRYSA
         LLGFR 0,2
         AG    0,=AD(1048575)
         SRLG  0,0,20
         STG   0,SEGMENTS
         SAM31
         IARV64 REQUEST=GETSTOR,COND=YES,SEGMENTS=SEGMENTS,            X
               ORIGIN=HIGHADDR,RETCODE=MEMRC,RSNCODE=MEMRSN,           X
               MF=(E,MEMPL)
         SAM64
         LARL  12,LABTS64
         LTR   15,15
         BNZ   ALCBAD
         LG    2,HIGHADDR
         B     ALCRET
ALCBAD   LGHI  2,0
ALCRET   LARL  1,SVCSAVE
         LMG   6,15,0(1)
         BR    14
         DROP  12
RELEASE  LARL  1,SVCSAVE
         STMG  6,15,0(1)
         LARL  12,LABTS64
         USING LABTS64,12
         LA    13,ENTRYSA
         STG   2,HIGHADDR
         SAM31
         IARV64 REQUEST=DETACH,COND=YES,MEMOBJSTART=HIGHADDR,          X
               RETCODE=MEMRC,RSNCODE=MEMRSN,MF=(E,MEMPL)
         SAM64
         LARL  12,LABTS64
         LGFR  2,15
         LARL  1,SVCSAVE
         LMG   6,15,0(1)
         BR    14
         DROP  12
FILECALL LARL  1,SVCSAVE
         STMG  6,15,0(1)
         LARL  12,LABTS64
         USING LABTS64,12
         LA    13,ENTRYSA
         SAM31
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
FILRET   SAM64
         LARL  12,LABTS64
         LGFR  2,2
         LARL  1,SVCSAVE
         LMG   6,15,0(1)
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
         DYNALLOC
         LR    2,15
         B     FILRET
         DROP  12
READLINE LARL  1,SVCSAVE
         STMG  6,15,0(1)
         LARL  12,LABTS64
         USING LABTS64,12
         LA    13,ENTRYSA
         STG   2,INPUTPTR
         STG   4,COUNTPTR
         LGR   6,2
         LLGFR 7,3
         LGR   8,4
         SR    0,0
         ST    0,0(8)
         LTR   7,7
         BNP   INBAD
         CL    7,=F'256'
         BH    INBAD
         L     9,OUTBUF
* EDIT removes 3270 terminal controls. Application case is preserved.
         SAM31
         TGET  (9),(7)
         SAM64
         LARL  12,LABTS64
         LG    6,INPUTPTR
         LG    8,COUNTPTR
         LLGF  9,OUTBUF
         LLGFR 7,7
         LLGFR 1,1
         LGFR  2,15
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
INRET    LARL  1,SVCSAVE
         LMG   6,15,0(1)
         BR    14
         DROP  12
FINISH   SAM31
         LARL  12,LABTS64
         USING LABTS64,12
         B     CLEANUP
* Called directly from LP64 C; EPSW precedes any SAM31. Return 64 only
* when the executing C path is both AMODE64 and problem state.
         DROP  12
GETSTATE LARL  1,SVCSAVE
         STMG  6,15,0(1)
         EPSW  2,3
         LARL  12,LABTS64
         USING LABTS64,12
         N     2,=X'00010001'
         C     2,=X'00010001'
         BNE   GSBAD
         LA    2,64
         B     GSRET
GSBAD    SR    2,2
GSRET    LARL  1,SVCSAVE
         LMG   6,15,0(1)
         BR    14
         DS    0D
SVCTAB   DC    F'25602',F'0'
         DC    F'0',A(PUTLINE),F'0',A(ALLOCATE)
         DC    F'0',A(RELEASE),F'0',A(FILECALL)
         DC    F'0',A(FINISH),F'0',A(READLINE)
WORKPTR  DC    D'0'
         DC    F'65536',F'0'
         DC    F'0',A(GETSTATE)
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
WORKLO   DS    F
RETCODE  DS    F
ENTRYPSW DS    F
ENTRYSA  DC    18F'0'
         DS    0D
ENTRY64SAVE DS 16D
SVCSAVE  DS    10D
INPUTPTR DS    D
COUNTPTR DS    D
SEGMENTS DS    D
HIGHADDR DS    D
MEMRC    DS    F
MEMRSN   DS    F
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
         IARV64 MF=(L,MEMPL),PLISTVER=MAX
         END   LABTS64
