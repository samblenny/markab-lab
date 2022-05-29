( === test/fetch_store.fs ===)
( -- These should all give errors: --)
-1 @
-1 b@
65536 @
65545 @
65534 @
65533 @
65536 b@
clearstack
1 65536 !
1 65535 !
1 65534 !
1 65533 !
1 65536 !
clearstack
( -- These should give OK: --)
1 60000 !
60000 @ .
2 60000 b!
60000 b@ .

