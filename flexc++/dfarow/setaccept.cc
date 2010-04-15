#include "dfarow.ih"

void DFARow::setAccept(size_t acceptType, size_t stateIdx)
{
    if (acceptType == State::NONE)
        return;

    d_acceptRules.insert(
        pair<size_t, size_t>(d_rules->hasAcceptState(stateIdx), acceptType)
    );
        
}