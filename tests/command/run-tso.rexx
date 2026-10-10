/* REXX: exact freeze-3 CALL, actual RC and borrowed caller survival. */
say 'ENV HARNESS BEGIN FREEZE3'
address tso "CALL 'LABA01.ENVCMD.LOAD(ENV31)' 'DD:ENVINPUT'"
native_rc=rc
say 'ENV NATIVE RC='native_rc
if native_rc<>0 then do
  say 'ENV HARNESS FAIL'
  exit native_rc
end
say 'ENV CALLER SURVIVED'
say 'ENV HARNESS PASS'
exit 0
