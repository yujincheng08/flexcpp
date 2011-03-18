#include <istream>
$insert class_ih

$insert namespace-open

// s_ranges: use (unsigned) characters as index to obtain
//           that character's range-number.
//           Ranges for BOL and EOF are in constants in the
//           class header file
$insert ranges

// s_dfa contains the rows of *all* DFAs ordered by start state.
// The enum class StartCondition is defined in the baseclass header
// INITIAL is always 0.
// Each entry defines the row to transit to if the column's
// character range was sensed. Row numbers are relative to the
// used DFA and d_dfaBase is set to the first row of the subset to use.
// The row's final two values are begin and end indices in
// s_accept[], defining the state's final and LA details
$insert DFAs

// The first value is the rule index
// The second value is the final indicator:
//  -2: not a final state (NO_FINAL_STATE), 
//  -1: final state, matching all text
//  >= 0: final state, the value is the LA tail length.
// The third value indicates other LA uses:
//  -1: Not a LA state tail length,
//  >=0: LA tail on transit FROM this state.
// The fourth value indicates an incrementing (1) tail:
// the tail length is incremented at each subsequent transition
$insert finAcs

$insert DFAbases

$insert inputMembers

size_t \@Base::Input::next()
{
    size_t ch;

    if (d_deque.empty())                    // deque empty: next char fm
    {
        ch = d_in->get();                   // istream
        return *d_in ? ch : AT_EOF;
    }

    ch = d_deque.front();
    d_deque.pop_front();

    return ch;
}

void \@Base::Input::push_front(size_t ch)
{
    if (ch < 0x100)
    {
$insert 8 debug.input "Input::push_front(" << ch << "), d_returnBOL = false"
        d_returnBOL = false;
        d_deque.push_front(ch);
    }
}

void \@Base::Input::push_front(std::string const &str, size_t fm)
{
    for (size_t idx = str.size(); idx-- > fm; )
        d_deque.push_front(str[idx]);
}

\@Base::\@Base(std::istream &in, std::ostream &out)
:
    d_state(0),
    d_out(&out),
    d_input(in),
$insert debugInit
    d_dfaBase(s_dfa),
    d_lineno(1)
{}

$insert debugFunctions

void \@Base::updateLineno__()
{
    for 
    (
        size_t pos = 0; 
            (pos = d_matched.find('\n', pos)) != std::string::npos; 
                ++pos
    )
        ++d_lineno;
}

\@Base::ActionType__ \@Base::actionType__(size_t range)
{
$insert 4 checkBOL

    d_nextState = d_dfaBase[d_state][range];

    if (d_nextState != -1)                  // transition is possible
        return ActionType__::CONTINUE;

    if (atFinalState(d_finalInfo.finac))    // FINAL state reached
        return ActionType__::MATCH;

$insert 4 ignoreBOLaction

    if (range == s_rangeOfEOF)
        return ActionType__::EOF_REACHED;
    
    return ActionType__::ECHO_FIRST;        // no match, echo the 1st char
}

inline size_t \@Base::tailLength(int const *finac) const
{
    size_t ruleIdx = finac[R];
    if (d_LAtail[ruleIdx] == NO_INCREMENTS)
    {
        int fixedLAtail = finac[F];
$insert 8 debug.finac "Fixed tail = " << fixedLAtail
        return fixedLAtail  < 0 ? 0 :  fixedLAtail;
    }
$insert 4 debug.finac "Variable LA tail = " << d_LAtail[ruleIdx]
    return d_LAtail[ruleIdx];
}

  // At this point a rule has been matched.  The next character is not part of
  // the matched rule and can be sent back to the input.  The index of the
  // matched rule is determined and the length of the matched text is reduced
  // by LA-tail (if available) and by d_less. The reduced chars are sent back
  // to the input.  The rule index is returned, which is then used in lex__ by
  // executeAction__ to execute the matching action block.
  // In lex__ the action block may return, otherwise return__ returns false 
  // and the scanner is reset for another run
int \@Base::matched__(size_t ch)
{
$insert 4 debug.action "MATCH"

    d_input.push_front(ch);

    size_t length = d_finalInfo.matchLen;   // length of the matched text

    int const *finac = d_finalInfo.finac;   // final info for this state
    int ruleIdx = finac[R];
    
                                            // maybe reduce by LA's length
    if (size_t tail = tailLength(finac))
        length -=  tail;
                                            // maybe further reduction
    length -=  d_less < length ? d_less : length;
        
    d_input.push_front(d_matched, length);  // push front the tail
    d_matched.resize(length);               // return what's left

$insert 4 debug.action "match buffer contains `" << d_matched << "'"

    updateLineno__();        
    return ruleIdx;
}

size_t \@Base::getRange__(size_t ch)
{
    switch (ch)
    {
        case AT_EOF:
        return s_rangeOfEOF;

$insert 8 rangeAtBOL

        default:
        return s_ranges[ch];
    }
}

void \@Base::incLAtails()
{
    for
    (
        auto iter = d_LAtail.begin(), end = d_LAtail.end(); 
            iter != end; 
                ++iter
    )
    {
        if (*iter != NO_INCREMENTS)
            ++*iter;
    }
}

  // At this point d_nextState contains the next state and continuation is
  // possible. The just read char. is appended to d_match, and any LA tail
  // counts are incremented.
void \@Base::continue__(size_t ch)
{
$insert 4 debug.action "CONTINUE, NEW STATE: " << d_nextState

    d_state = d_nextState;

    switch (ch)
    {
        case AT_EOF:
        case AT_BOL:
        break;

        default:
            d_matched += ch;
            incLAtails();
        break;
    }
}

   // At this point there is no continuation possible. The last character is
   // pushed back into the input stream as well as all but the first char. in
   // the buffer. The first char. in the buffer is echoed to stderr. 
   // If there isn't any 1st char yet then the current char doesn't fit any
   // rules and that char is then echoed
void \@Base::echoFirst__(size_t ch)
{
$insert 4 debug.action "ECHO_FIRST"

    if (d_matched.empty())          // no match possible: echo ch itself
        std::cerr << ch;
    else                            // echo the 1st matched char, push_front
    {                               // the rest
        d_input.push_front(ch);
        d_input.push_front(d_matched, 1);
        std::cerr << (ch = d_matched[0]);
    }
    d_lineno += ch == '\n';
}

$insert pushFront

    // Inspect all s_finAc elements associated with the current state
    // 
    // If the current state is a final state then store the address of the 
    // s_finAc element and the current buffer length in d_finalInfo.
    //
    // Otherwise, if an incremental tail then store the initial tail length
    //
    // Later, when transiting:
    //      Increment all incremental tail's d_LAtail's values;
    //      At a MATCHED action: 
    //          if the rule is an incrementail tail rule, use that tail
    //          or accept the buffer's d_finalInfo[1] length
    //      At an EOF_REACHED or ECHO_FIRST action:
    //          if d_finalInfo specifies a rule, do matched__() for that rule, 
    //          otherwise do ECHO_FIRST
void \@Base::inspectFinac__()
{
    for 
    (
        size_t begin = d_dfaBase[d_state][s_finacIdx], 
                 end = d_dfaBase[d_state][s_finacIdx + 1];
            begin != end;
                ++begin
    )
    {
        int const *finac = s_finAc[begin];

            // If the current state is a rule's final state then store 
            // the rule, the current buffer length, and the LA tail value
            // in d_finalInfo.
        if (atFinalState(finac))
            d_finalInfo = 
                {
                    finac,              // store the finac info's location
                    d_matched.size()    // and the match length
                };

            // Otherwise, if incremental tail store the initial tail length
            //      at d_LAtail
        else if (incrementalTail(finac))
        {
$insert 12 debug.finac "Setting LAtail [" << finacInfo[R] << "] to " +
$insert 12 debug.finac finacInfo[T] << ", incrementing"
        }
    }   
}

void \@Base::reset__()
{
    d_state = 0;
    d_return = true;

    if (!d_more)
        d_matched.clear();
    d_more = false;
    d_less = 0;
$insert 4 resetStartsAtBOL

    d_finalInfo = {0, };
    d_LAtail = VectorInt(s_nRules, NO_INCREMENTS);
}

int \@::executeAction__(int ruleIdx)
{
$insert 4 debug.action  "Executing actions of rule " << ruleIdx
    switch (ruleIdx)
    {
$insert 8 actions
    }
$insert 4 debug.action "Rule " << ruleIdx << " did not do 'return'"
    noReturn__();
    return 0;
}

int \@::lex__()
{
    reset__();
    preCode();

    while (true)
    {
        size_t ch = get__();                // fetch next char
        size_t range = getRange__(ch);      // determine the range

$insert 8 debugStep

        inspectFinac__();

        switch (actionType__(range))        // determine the action
        {
            case ActionType__::CONTINUE:
                continue__(ch);
            break;

            case ActionType__::ECHO_FIRST:
                echoFirst__(ch);
                reset__();                      // fresh start 
                preCode();
            break;

            case ActionType__::EOF_REACHED:
$insert 16 debug.action  "EOF_REACHED"
                updateLineno__();
            return 0;

            case ActionType__::IGNORE_BOL:
//$insert 16 debug.action "IGNORE_BOL";
            break;

            case ActionType__::MATCH:
            {
                int ret = executeAction__(matched__(ch));
                if (return__())
                    return ret;
                reset__();
                preCode();
                continue;
            }

            case ActionType__::PUSH_FRONT:
$insert 16 pushFrontCall
            break;
        } // switch
    } // while
}

$insert namespace-close