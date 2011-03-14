#ifndef \@_H_INCLUDED_
#define \@_H_INCLUDED_

$insert baseclass.h

$insert namespace-open

class \@: public \@Base
{
    public:
        \@() = default;
        explicit \@(std::istream &iStream, std::ostream &out = std::cout);

$insert 8 lexFunctionDecl

    private:
        int executeAction(int ruleNr);
        int lex__();
        int executeAction__(int ruleNr);

        void preCode();     // re-implement this function for code that must 
                            // be exec'ed before the patternmatching starts
};

inline void Scanner::preCode()  // optionally replace by your own code
{}

inline \@::\@(std::istream &iStream, std::ostream &oStream)
:
    \@Base(iStream, oStream)
{}

$insert inlineLexFunction

inline int \@::executeAction(int ruleNr)
{
    return executeAction__(ruleNr);
}

$insert namespace-close

#endif // \@_H_INCLUDED_
