# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(student-test-2) begin
(student-test-2) create "testfile"
(student-test-2) open "testfile"
(student-test-2) true
(student-test-2) test coalesce
(student-test-2) close "testfile"
(student-test-2) end
EOF
pass;
