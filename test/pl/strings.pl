#!/usr/bin/perl

use feature "say";

say qq H\Hello World!H;
say qw@Hello \@ World@;
say q{Hello!};

say qr"Hello";
say qr{Hi!};
say qr(Helllo);
tr TlTxT;
say qq {Hello {World} \{};

s/Hello/World/;
s WTHingWAreW;
s{Hello}{Wor{}sld}i;

$subject =~ s{A} {B}gi;
$event   =~ s{C} {D}gi;

q {Hello};
q ZHEllo {} WorldZ;

tr {Hello}{World};
tr {Hello} {World};
tr {Hello} /World/;
tr {Hello}/World/;
tr /Hello/World/;


tr{Hello}<World>;
tr{Hello}!World!;

s /
    Hello
    World
   /
    Indeed!

/;

$program =~ s {
    /\*	# Match the opening delimiter.
        .*?	# Match a minimal number of characters.
        \*/	# Match the closing delimiter.
} []gsx;

tr///;
tr{}[];


my $x = 50;
#
sub clean_and_trim_subject {
    $$ref =~ s/\n.*//s;
}

my $event_eat = [qw[head title style layer applet object xml param base]];
my $event_remove = [qw[bgsound embed object link body meta noscript plaintext noframes]];

s WHelloWorldWsdxx;
qr /Hello/xs;
m /pattern/sxx;
m {Pattern}sxx;
/Hello/s;
tr/hello/world/cdsr;
y/world/helllo/cdsr;
s/PATTERN/REPLACEMENT/msixpodualngcer;
m/PATTERN/msixpodualngc;
/PATTERN/msixpodualngc;
qr/STRING/msixpodualn;



m/PATTERN/msixpongc;
/PATTERN/msixplngc;


q{\\};
q{\{};
q{\}};
