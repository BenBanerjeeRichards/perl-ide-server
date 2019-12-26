//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#ifndef PERLPARSE_VARIABLE_H
#define PERLPARSE_VARIABLE_H

#include "Util.h"

class GlobalVariable {
    // Doesn't include sigil
    std::string package;

    std::string name;

    std::string sigil;

    FilePos filePos;


public:

    GlobalVariable(std::string sigil, std::string package, std::string name);

    const std::string getFullName() const;

    const std::string &getPackage() const;

    const std::string &getName() const;

    const std::string &getSigil() const;

    const FilePos &getFilePos() const;

    void setFilePos(const FilePos &filePos);

    std::string toStr();

    bool operator==(const GlobalVariable &other) const {
        return this->getFullName() == other.getFullName();
    }

};

struct Variable {
    std::string name;

    // Start and end point of variable location
    FilePos declaration;
    FilePos symbolEnd;

    int id;

    bool operator==(const Variable &other) const {
        return this->id == other.id;
    }

    virtual bool isAccessibleAt(const FilePos &pos) {
        return false;
    }

    virtual std::string toStr() { return ""; }

    virtual std::string getDetail() { return ""; }
};


namespace std {
    template<>
    struct hash<GlobalVariable> {
        std::size_t operator()(const GlobalVariable &var) const {
            return std::hash<std::string>()(var.getFullName());
        }
    };
}


class ScopedVariable : public Variable {
public:
    ScopedVariable(int id, const std::string &name, FilePos declaration, FilePos symbolEnd, FilePos scopeEnd) {
        this->name = name;
        this->declaration = declaration;
        this->scopeEnd = scopeEnd;
        this->id = id;
        this->symbolEnd = symbolEnd;
    }

    bool isAccessibleAt(const FilePos &pos) override;

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] (#" + std::to_string(id) + ")" + name;
    }


private:
    FilePos scopeEnd;
};


class OurVariable : public ScopedVariable {
public:
    OurVariable(int id, const std::string &name, FilePos declaration, FilePos symbolEnd, FilePos scopeEnd,
                const std::string &package)
            : ScopedVariable(id, name, declaration, symbolEnd, scopeEnd) {
        this->package = package;
    }

    std::string package;

    std::string getDetail() override {
        return this->package;
    }

    std::string toStr() override {
        if (name.empty()) return "<NO NAME>";
        return "[" + this->declaration.toStr() + "] (#" + std::to_string(id) + ") our " + name[0] + package + "::" +
               name.substr(1, name.size() - 1);
    }
};

// Local variables ARE NOT SCOPED
// But we can't determine places where they are used
// As it depends on the call stack
// So instead we treat them as scoped variables and ban renames on them when possible
class LocalVariable : public ScopedVariable {
public:
    LocalVariable(int id, const std::string &name, FilePos declaration, FilePos symbolEnd, FilePos scopeEnd)
            : ScopedVariable(id, name, declaration, symbolEnd, scopeEnd) {
    }

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] (#" + std::to_string(id) + ")" + name + " (LOCAL)";
    }
};


#endif //PERLPARSE_VARIABLE_H
