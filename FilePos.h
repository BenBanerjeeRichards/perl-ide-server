//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#ifndef PERLPARSER_FILEPOS_H
#define PERLPARSER_FILEPOS_H

#include <iostream>

struct FilePos {
    FilePos() {
        this->line = 0;
        this->col = 0;
        this->position = -1;
    }

    FilePos(int line, int pos) {
        this->line = line;
        this->col = pos;
        this->position = -1;
    }

    FilePos(int line, int pos, int position) {
        this->line = line;
        this->col = pos;
        this->position = position;
    }


    std::string toStr();

    int line;
    int col;
    int position;
};



#endif //PERLPARSER_FILEPOS_H
