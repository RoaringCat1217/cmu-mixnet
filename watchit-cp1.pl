#!/usr/bin/perl -w

#if (@ARGV != 2)
#{
#    print "Usage: watchit.pl <cmd> <str>\n";
#    exit(1);
#}

#$cmd = $ARGV[0];
#$str = $ARGV[1];

$str = "[Testing] FAIL";

@commands = ("./bin/cp1/testcase_line_easy -a; echo;",
             "./bin/cp1/testcase_line_hard -a; echo;",
             "./bin/cp1/testcase_tree_easy -a; echo;",
             "./bin/cp1/testcase_tree_hard -a; echo;",
             "./bin/cp1/testcase_ring_easy -a; echo;",
             "./bin/cp1/testcase_ring_hard -a; echo;",
             "./bin/cp1/testcase_full_mesh_easy -a; echo;",
             "./bin/cp1/testcase_full_mesh_hard -a; echo;",
             "./bin/cp1/testcase_tiebreak_pathlen -a; echo;",
             "./bin/cp1/testcase_tiebreak_parent_ring -a; echo;",
             "./bin/cp1/testcase_tiebreak_parent_mesh -a; echo;",
             "./bin/cp1/testcase_tiebreak_multi -a; echo;",
             "./bin/cp1/testcase_link_failure_root -a; echo",
             "./bin/cp1/testcase_link_failure_ring -a; echo;",
             "./bin/cp1/testcase_link_failure_mesh -a; echo;",
             "./bin/cp1/testcase_unreachable -a; echo;");

# ./watchit.pl './bin/cp1/testcase_tiebreak_pathlen -a; echo;' '[Testing] FAIL'

while (1)
{

    for my $i (0 .. $#commands)
    {
        my $output = `$commands[$i]`;
        my $new_output = substr($output, -16, 14);
        print $output;

        if (($new_output cmp $str) == 0)
        {
            exit(0);
        }
    }

    print "----------------------------------------------------------------\n\n"
}
