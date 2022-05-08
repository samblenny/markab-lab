( === test/fetch_store.fs ===)
( -- These should all give errors: --)
-1 @
-1 b@
16384 @
16383 @
16382 @
16381 @
16384 b@
clearstack
1 16384 !
1 16383 !
1 16382 !
1 16381 !
1 16384 !
clearstack
( -- These should give OK: --)
1 0 !
0 @ .
2 0 b!
0 b@ .
-1 16380 !
16380 @ .
35 16383 b!
16383 b@ .
