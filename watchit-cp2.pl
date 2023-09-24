#!/usr/bin/perl -w

#if (@ARGV != 2)
#{
#    print "Usage: watchit.pl <cmd> <str>\n";
#    exit(1);
#}

#$cmd = $ARGV[0];
#$str = $ARGV[1];

$str = "[Testing] FAIL";

@commands = ("./bin/cp2/testcase_sp_uniform_ring -a; echo;",
             "./bin/cp2/testcase_sp_uniform_mesh -a; echo;",
             "./bin/cp2/testcase_sp_symmetric_ring -a; echo;",
             "./bin/cp2/testcase_sp_symmetric_mesh -a; echo;",
             "./bin/cp2/testcase_sp_asymmetric_ring -a; echo;",
             "./bin/cp2/testcase_sp_asymmetric_mesh -a; echo;",
             "./bin/cp2/testcase_mixing -a; echo;",
#             "./bin/cp2/testcase_random -a; echo;",
             "./bin/cp2/testcase_ping -a; echo;");

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
