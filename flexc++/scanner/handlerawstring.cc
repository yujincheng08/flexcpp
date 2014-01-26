#include "scanner.ih"

bool Scanner::handleRawString()
{
    if (d_inCharClass)
    {
        accept(1);
        return false;
    };

cerr << "Starting raw string " << matched() << '\n';

    d_rawString = matched();
    d_rawString.erase(0, 1);        // rm the R

    d_rawString.front() = ')';      // end sentinel is )IDENTIFIER"
    d_rawString.back() = '"';

    push(StartCondition__::rawstring);
    return true;
}
