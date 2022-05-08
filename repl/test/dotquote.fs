( === test/dotquote.fs ===)
( -- These should give errors: --)
( no space)      .""
( CR)            ."
( space CR)      ." 
( space word CR) ." word
( -- These should give OK: --)
." "
."  "
." word"
."  word"
