( === test/fetch_store.fs ===)
( -- These should all give errors: --)
-1 @
-1 b@
-1 w@
65536 @
65535 @
65534 @
65533 @
65536 b@
65536 w@
65535 w@
clearstack
1 0 !
1 0 w!
1 0 b!
1 65536 !
1 65535 !
1 65534 !
clearstack
1 65533 !
1 65536 w!
1 65535 w!
1 65536 b!
clearstack
( -- These should give OK: --)
1 2000 !
2000 @ .
3 2000 w!
2000 w@ .
2 2000 b!
2000 b@ .
1 65532 !
65532 @ .
3 65534 w!
65534 w@ .
2 65535 b!
65535 b@ .
