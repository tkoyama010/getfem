$er = 0;
open F, "./dynamic_tas 2>&1 |" or die;
while (<F>) {
  # print $_;
  if ($_ =~ /error has been detected/)
  {
    $er = 1;
    print "=============================================================\n";
    print $_, <F>;
  }
}
if ($er == 1) { exit(1); }
`./dynamic_tas`;
if ($?) { exit(1); }

