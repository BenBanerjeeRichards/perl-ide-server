use warnings;
use strict;

$hello = 2;
$_Hello = 3;
%hello = 3;
@_83Hel34 = 5;
$H3ello = 54;
$::::'foo;
$foo::'bar;
$Hello::world::thing;
$World::Hello'thing;

$1 = 2;
$0 = 3;
$20384 = 23;

$!;
$?;
$^;

$^A;
$^B;
$^?;
$^^;

${^HE34LLO};
${^Hello};
${^_Hell2o};

# Single dereferences
$$hello;

# Multiple dereferences
$$$hello;
%$$hello;


# These are not valid and should not be interpreted as variables
${Hello};
$::'::foo;
$foo'::bar;
${^Hello;
