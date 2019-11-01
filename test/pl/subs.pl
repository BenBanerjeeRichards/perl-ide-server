sub signatureNoAttribute($a, $b = 54, $c=5) {
    say "f(...) " . $a;
    say ($b);
}

# #Prototype, no attributes
sub protoNoAttribute($$@) {
}

# Signature with attributes
sub signatureWithAttribute:lvalue(a,b) method :  test(34, $fv, tr) ($a, $b)     {
}

sub protoWithAttribute($$@[$%]) :lvalue {

}

sub f : attr('()', '(', ')', '\(') {
}
