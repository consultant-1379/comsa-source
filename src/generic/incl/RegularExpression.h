#ifndef COMREGEXP_H_
#define COMREGEXP_H_

#include <regex.h>
#include <vector>
#include "ComSA.h"
#include "trace.h"

namespace Com
{

/**
 * helper for evaluating of regular expressions
 */
class RegularExpression
{
public:


    /**
     * constructor
     * @param pattern a regular expression
     * @param cflags flags for compiling of the regular expression. Defined flags are :
        REG_EXTENDED
              Use POSIX Extended Regular Expression syntax when interpreting regex.  If not set, POSIX  Basic  Regular
              Expression syntax is used.

       REG_ICASE
              Do  not differentiate case.  Subsequent regexec() searches using this pattern buffer will be case insen-
              sitive.

       REG_NOSUB
              Support for substring addressing of matches is  not  required.   The  nmatch  and  pmatch  arguments  to
              regexec() are ignored if the pattern buffer supplied was compiled with this flag set.

       REG_NEWLINE
              Match-any-character operators don't match a newline.

              A non-matching list ([^...])  not containing a newline does not match a newline.

              Match-beginning-of-line operator (^) matches the empty string immediately after a newline, regardless of
              whether eflags, the execution flags of regexec(), contains REG_NOTBOL.

              Match-end-of-line operator ($) matches the empty string immediately  before  a  newline,  regardless  of
              whether eflags contains REG_NOTEOL.
     */
    RegularExpression(const std::string& pattern, int cflags = REG_EXTENDED | REG_NOSUB) : _cflags(cflags) {
        int r = ::regcomp(&_regex, pattern.c_str(), cflags);
        _isValid = (r == 0);
        if (!_isValid) {
            DEBUG_MWSA_GENERAL("Invalid regexp  %s", pattern.c_str());
            char errbuf[1025];
            ::regerror(r ,&_regex, errbuf,(size_t)1024);
            _errorMsg = errbuf;
        }
    }


    /**
     * destructor
     */
    virtual ~RegularExpression()
    {
    	DEBUG_MWSA_GENERAL("~RegularExpression(): destructor called");
        ::regfree(&_regex);
    }

    /**
     * returns true if the regular expression provided in the constructor is possible to parse
     */
    bool isExpressionValid() const {
        return _isValid;
    }

    /**
     * if the regular expression is not valid, this method can give a verbose error message
     */
    const std::string& getExpressionErrorMsg() const {
        return _errorMsg;
    }

    /** Matches the given subject string against the pattern.
     * @param value a string to match
     * @param eflags flags to use for the evaluation. Possible flags are:
      REG_NOTBOL
              The  match-beginning-of-line  operator  always  fails to match (but see the compilation flag REG_NEWLINE
              above) This flag may be used when different portions of a string are passed to regexec() and the  begin-
              ning of the string should not be interpreted as the beginning of the line.

       REG_NOTEOL
              The match-end-of-line operator always fails to match (but see the compilation flag REG_NEWLINE above)
        @return true if the value matches the regular expression, otherwise false

     */
    bool match(const std::string& value, int eflags = 0) const {
        return _isValid && (::regexec(&_regex, value.c_str(), 0, NULL, eflags) == 0);
    }


    /** Matches the given subject string against the pattern. For further details, see the manpage for regexec
     * @param value a string to match
     * @param nMatch  Unless  REG_NOSUB  was  set for the compilation of the pattern buffer, it is possible to obtain substring match
       addressing information.  pmatch must be dimensioned to have at least nmatch elements.  These are filled  in  by
       regexec() with substring match addresses.  Any unused structure elements will contain the value -1.

     * @param eflags flags to use for the evaluation.
     * @return true if the value matches the regular expression, otherwise false
     */
    bool match(const std::string& value, size_t nMatch, regmatch_t matches[], int eflags = 0) const {
        if (_cflags & REG_NOSUB) {
            ERR_MWSA_GENERAL("This method does not work if REG_NOSUB is set in constructor");
            return false;
        }
        return _isValid && (::regexec(&_regex, value.c_str(), nMatch, matches, eflags) == 0);
    }



private:

    RegularExpression(const RegularExpression&);
    void operator=(const RegularExpression&);

    regex_t _regex;
    bool _isValid;
    std::string _errorMsg;
    int _cflags;

};

}

#endif
