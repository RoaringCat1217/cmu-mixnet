#!/usr/bin/perl -w

if (@ARGV != 2)
{
    print "Usage: watchit.pl <cmd> <str>\n";
    exit(1);
}

$cmd = $ARGV[0];
$str = $ARGV[1];

# ./watchit.pl './bin/cp1/testcase_tiebreak_pathlen -a; echo;' '[Testing] FAIL'
# zip -j submission.zip attr.c attr.h ll.c ll.h logger.c logger.h lsa.c lsa.h node.c node.h routing.c routing.h stp.c stp.h utils.c utils.h

while (1)
{
    my $output = `$cmd`;
    my $new_output = substr($output, -16, 14);
    print $output;

    if (($new_output cmp $str) == 0)
    {
        exit(0);
    }
}
