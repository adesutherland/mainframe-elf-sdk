/* REXX: check one source-built SDK load member on a leased TSO account. */
parse upper arg loadlib member
if loadlib='' | member='' then exit 8
address tso "CALL '"loadlib"("member")' '' ASIS"
result=rc
say 'SDK MEMBER' member 'RETURN RC='result
if result<>42 then exit 16
say 'SDK MEMBER' member 'PASS'
exit 0
