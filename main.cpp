#include <iostream>
#include "token/Token.h"

int main() {
    auto t1 = new StringToken("Hello");
    auto t2 = new KeywordToken("my");
    auto t3 = new StringToken("World");

    std::cout << t1->toString() << " " << t2->toString() << " " << t3->toString() << std::endl;

    std::cout << "Hello, World!" << std::endl;
    return 0;
}