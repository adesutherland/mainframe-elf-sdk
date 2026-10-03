/* REXX: prepare task-owned sequential and PDS DDs for sdk-file-smoke.c. */
parse upper arg prefix
if prefix='' then exit 8
address tso
command="ALLOC DA('"prefix".TXT') NEW CATALOG SPACE(2,1) TRACKS "
command=command"DSORG(PS) RECFM(V B) LRECL(80) BLKSIZE(800)"
command
if rc<>0 then do
  say 'SDK FILE TXT ALLOC RC='rc
  exit 12
end
"FREE DA('"prefix".TXT')"
if rc<>0 then exit 16
"ALLOC FI(SDKTXT) DA('"prefix".TXT') SHR"
if rc<>0 then exit 20
"ALLOC FI(SDKPDS) DA('"prefix".SRC(SDKCHK)') SHR"
if rc<>0 then exit 24
say 'SDK FILE DD SETUP PASS' prefix
exit 0
