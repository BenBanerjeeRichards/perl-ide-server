#include <iostream>
#include "token/Token.h"

int main() {
    auto t1 = new StringToken("Hello", 5, 3);
    auto t2 = new KeywordToken("my", 3, 4);
    auto t3 = new StringToken("World", 6, 5);

    std::cout << t1->toString() << " " << t2->toString() << " " << t3->toString() << std::endl;

    return 0;
}