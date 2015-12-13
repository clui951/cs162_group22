# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(student-test-1) begin
(student-test-1) create "testfile"
(student-test-1) open "testfile"
(student-test-1) true
(student-test-1) test hitrate
(student-test-1) close "testfile"
(student-test-1) end
EOF
pass;