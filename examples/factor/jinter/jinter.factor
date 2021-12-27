USING: alien alien.c-types alien.libraries alien.syntax arrays
       combinators io kernel parser splitting system ;

IN:    jinter

! Load DLL at compile/parse time
! <<
!   "jinter" "C:\\Users\\martsa\\J80564\\bin\\jinter.dll" cdecl add-library
!   "jinter" load-library
!   drop
! >>
<<
  "jinter" "/home/martin/J903/bin/jinter.so" cdecl add-library
>>

! Declare functions
LIBRARY:  jinter
FUNCTION: long     jinit ( )
FUNCTION: long     jsend ( c-string inp )
FUNCTION: c-string jrecv ( )
FUNCTION: long     jstop ( )

! Useful function to send a sentence to J, receive it's result
! and converting it to an array.
! WARNING: This works only with number arrays, at this time !!!
: jexec ( str -- arr )
  jsend drop jrecv
  string-lines parse-lines >array
;
