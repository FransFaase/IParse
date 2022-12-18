#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "TextFilePos.h"
#include "Scanner.h"
#include "AbstractParser.h"


#define DEBUG_NL if (_debug_parse) printf("\n")
#define DEBUG_PT(X) if (_debug_parse) X.print(stdout, true)
#define DEBUG_PO(X) if (_debug_parse && X != 0) X->print(stdout)
#define DEBUG_PR(X) if (_debug_parse && X != 0) X->print(stdout)
#define DEBUG_(X)  if (_debug_parse) printf(X)
#define DEBUG_P1(X,A) if (_debug_parse) printf(X,A)




AbstractParser::AbstractParser()
{
	_debug_nt = false;
	_debug_parse = false;
	_debug_scan = false;
}


void AbstractParser::expected_string(TextFilePos& pos, const char *s, bool is_keyword)
{
    if (pos < _f_file_pos)
        return;
    if (pos > _f_file_pos)
    {   _nr_exp_syms = 0;
        _f_file_pos = pos;
    }

    if (_nr_exp_syms >= 200)
        return;

    _expect[_nr_exp_syms].sym = s;
    _expect[_nr_exp_syms].in_nt = _current_nt;
    _expect[_nr_exp_syms].rule = _current_rule;
    _expect[_nr_exp_syms].is_keyword = is_keyword;
    _nr_exp_syms++;
}


void AbstractParser::printExpected(FILE *f, const char* filename, const TextFileBuffer& textBuffer)
{   
    fprintf(f, "%s (%d.%d) `", filename, _f_file_pos.line(), _f_file_pos.column());
	TextFileBuffer buffer = textBuffer;
	buffer = _f_file_pos;
	for (int j = 0; j < 10; j++)
	{
		if (buffer.eof())
		{	fprintf(f, "<eof>");
			break;
		}
		char ch = *buffer;
		if (ch == '\0')
			fprintf(f, "\\0");
		else if (ch == '\t')
			fprintf(f, "\\t");
		else if (ch == '\n')
			fprintf(f, "\\n");
		else if (ch > '\0' && ch < ' ')
			fprintf(f, "\\?");
		else	
			fprintf(f, "%c", ch);
		buffer.next();
	}
	fprintf(f, "` expected:\n");
    for (int i = 0; i < _nr_exp_syms; i++)
    {   bool unique = true;
        int j;
        for (j = 0; unique && j < i; j++)
            if (_expect[i].rule == _expect[j].rule)
                unique = false;
        if (unique)
        {
            fprintf(f, "    ");
            if (_expect[i].rule != 0)
                _expect[i].rule->print(f);
            else
                printf("%s ", _expect[i].sym);
            fprintf(f, ":");
            if (!_expect[i].in_nt.empty())
                fprintf(f, " in %s", _expect[i].in_nt.val());
            if (_expect[i].is_keyword != 0)
                fprintf(f, " (is a keyword)");
            fprintf(f, "\n");
        }
    }
}

/* Special strings for identifiers and context */
const char *tt_identdef = "<identdef>";
const char *tt_identdefadd = "<identdefadd>";
const char *tt_identuse = "<identuse>";
const char *tt_identfield = "<identfield>";

