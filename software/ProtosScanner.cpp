#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"
#include "ProtosScanner.h"



#define IDENT_START_CHAR(X) (isalpha(X)||X=='_')
#define IDENT_FOLLOW_CHAR(X) (isalpha(X)||isdigit(X)||X=='_')
#define IS_CAPITAL(X) ('A' <= (X) && (X) <= 'Z')

void ProtosScanner::initScanning(Grammar* grammar)
{
	AbstractScanner::initScanning(grammar);
	_last_space_pos.clear();
}

Ident ProtosScanner::id_ident = "ident";
Ident ProtosScanner::id_string = "string";
Ident ProtosScanner::id_int = "int";
Ident ProtosScanner::id_double = "double";
Ident ProtosScanner::id_newline = "newline";
Ident ProtosScanner::id_indent = "indent";
Ident ProtosScanner::id_any = "any";


void ProtosScanner::skipSpace(TextFileBuffer& text)
{
	//bool cpp_comments = true;
	//bool nested_comments = true;

	/* Fixed part. Do not modify!! */
    if (text == _last_space_pos)
    {   text = _last_space_end_pos;
        return;
    }
    _last_space_pos = text;

	/* Flexible part. 
	   The code below increments scans the white space 
	   characters.
	*/
	if (text.column() != 1)
	{
		for(;;)
		{   if (*text == ' ')
			{
				text.next();
			}
			else
				break;
		}
	}

	/* Fixed part. Do not modify!! */
    _last_space_end_pos = text;
    /* _print_state(); */
}


bool ProtosScanner::acceptEOF(TextFileBuffer& text)
{
	return text.eof();
}

bool ProtosScanner::acceptLiteral(TextFileBuffer& text, const char* sym)
{
	if (*sym == '\0')
		return true;

    int i;
    const char *s;
    for (i = 0, s = sym; *s != '\0' && text[i] == *s; i++, s++);

	if (*s != '\0' || (IS_CAPITAL(*sym) && IS_CAPITAL(text[i])))
		return false;

    text.advance(i);
    skipSpace(text);

    return true;
}

bool ProtosScanner::acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result)
{
	if (name == id_ident)
		return accept_ident(text, result);
	if (name == id_string)
		return accept_string(text, result, false);
	if (name == id_int)
		return accept_int(text, result);
	if (name == id_double)
		return accept_double(text, result);
	if (name == id_newline)
		return accept_newline(text, result);
	if (name == id_indent)
		return accept_indent(text, result);
	if (name == id_any)
		return accept_any(text, result);

	return false;
}

bool ProtosScanner::accept_ident(TextFileBuffer& text, Ident& ident)
{
	if (text == _last_ident_pos)
	{
		ident = _last_ident;
		text = _last_ident_end_pos;

		return true;
	}

	/* Does it look an identifier? */
    if (text.eof() || (!IDENT_START_CHAR(*text) && *text != '\\'))
        return false;

	_last_ident_pos = text;

	char buf[2001];
	int i = 0;
	for (;;)
	{
		if (*text == '\\' && isdigit(text[1]) && isdigit(text[2]) && isdigit(text[3]))
		{
			buf[i++] = 100 * (text[1] - '0') + 10 * (text[2] - '0') + (text[3] - '0');
			text.advance(4);
		}
		else if (IDENT_FOLLOW_CHAR(*text))
		{
			buf[i++] = *text;
			text.next();
		}
		else
			break;
	}
	buf[i] = '\0';
	ident = buf;

	skipSpace(text);

	_last_ident_end_pos = text;
	_last_ident = ident;

	return true;
}


bool ProtosScanner::accept_ident(TextFileBuffer& text, AbstractParseTree& result)
{
	TextFilePos start_pos = text;

	Ident ident;
	if (accept_ident(text, ident))
	{
		result = ident;
		return true;
	}

	text = start_pos;
	return false;
}


bool ProtosScanner::accept_string(TextFileBuffer& text, AbstractParseTree& result, bool c_string)
/* Method for scanning a C-like string. If a string
   was scanned it is returned in 'string'. If 'c_string'
   is true, multiple strings only separated by white
   space will be returned as a single string as in C.
   This procedure is an auxilary procedure called from
   accept_string and accept_cstring.
*/
{
    if (text.eof() || text[0] != '"')
        return false;

    String string;
	String::filler filler(string);
    
    text.next();
    for (;;)
    {
        if (text.eof() || *text == '\0' || *text == '\n')
        {   
            break;
        }
        else if (*text == '\\')
        {
            text.next();
            if (isdigit(text[0]) && isdigit(text[1]) && isdigit(text[2]))
            {   filler << (100 * (text[0] - '0') + 10 * (text[1] - '0') + (text[2] - '0'));
				text.advance(3);
            }
            else if (!text.eof()) 
            {   if (*text == 'n')
                    filler << '\n';
                else if (*text == 't')
                    filler << '\t';
                else 
                    filler << *text;
                text.next();
            }
        }
        else if (*text == '"')
        {   text.next();
            if (c_string)
            {   skipSpace(text);
                if (text.eof() || *text != '"')
                     break;
                text.next();
            }
            else
                break;
        }
        else
        {   filler << *text;
            text.next();
        }
    }
	filler << '\0';

	skipSpace(text);

	result = string;
    return true;
}


bool ProtosScanner::accept_int(TextFileBuffer& text, AbstractParseTree& result)
{
    if (text.eof() || (   !isdigit(*text)
                 && (   (*text != '+' && *text != '-') 
                     || !isdigit(text[1]))))
        return false;

    int i = 0;
    if (text[i] == '-' || text[i] == '+')
        i++;
    if (text[i] == '0' && text[i+1] == 'x')
    {   i += 2;
        while (   isdigit(text[i]) 
               || ('a' <= text[i] && text[i] <= 'f') 
               || ('A' <= text[i] && text[i] <= 'F'))
            i++;
    }
    else
    {   while (isdigit(text[i]))
            i++;
        if (text[i] == '.')
            return false;
        if (text[i] == 'L')
            i++;
    }
	long v;    
    sscanf(text, "%ld", &v);

    text.advance(i);
	skipSpace(text);

	result = v;
    return true;
}

bool ProtosScanner::accept_double(TextFileBuffer& text, AbstractParseTree& result)
{

    if (text.eof() || (   !isdigit(*text) && *text != '.'
                 && (   (*text != '+' && *text != '-') 
                     || (!isdigit(text[1]) && text[1] != '.'))))
        return false;

    int i = 0;
    if (text[i] == '-' || text[i] == '+')
        i++;
    while (isdigit(text[i]))
        i++;
    if (text[i] != '.')
        return false;
    i++;
    while (isdigit(text[i]))
        i++;
	if (text[i] == 'e')
	{
		i++;
		if (text[i] == '-')
			i++;
		while (isdigit(text[i]))
			i++;
	}
	double v;
    sscanf(text, "%lf", &v);

    text.advance(i);
	skipSpace(text);

	result = v;
    return true;
}

bool ProtosScanner::accept_newline(TextFileBuffer& text, AbstractParseTree& result)
{
	if (text.eof() || *text != '\n')
		return false;

	text.next();
	
	return true;
}

bool ProtosScanner::accept_indent(TextFileBuffer& text, AbstractParseTree& result)
{
	if (text.eof() || text.column() != 1 || (*text != ' ' && *text != '\t'))
		return false;

	long i = 0;

	while (*text == ' ' || *text == '\t')
	{
		text.next();
		i++;
	}

	result = i;
	return true;
}


bool ProtosScanner::accept_any(TextFileBuffer& text, AbstractParseTree& result)
{
	while (!text.eof() && *text != '\n')
		text.next();

	return true;
}












