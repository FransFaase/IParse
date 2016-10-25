#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "Ident.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"
#include "RcScanner.h"
#include "ParserGrammar.h"


#define IDENT_START_CHAR(X) (isalpha(X)||X=='_')
#define IDENT_FOLLOW_CHAR(X) (isalpha(X)||isdigit(X)||X=='_')

void ResourceScanner::initScanning(Grammar* grammar)
{
	AbstractScanner::initScanning(grammar);
	_last_space_pos.clear();
}





void ResourceScanner::skipSpace(TextFileBuffer& text)
{
	bool cpp_comments = true;
	bool nested_comments = true;

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
    for(;;)
    {   if (*text == ' ' || *text == '\t')
        {
            text.next();
        }
        //else if (cpp_comments && text[0] == '/' && text[1] == '/')
        //{
        //    while (!text.eof() && text[0] != '\n')
        //        text.next();
        //}
        else if (text[0] == '/' && text[1] == '*')
        {   int nesting_depth = 1;
            TextFilePos sp = text;
 
 			text.next();
 			text.next();
            while (!text.eof() && !(text[0] == '*' && text[1] == '/') && nesting_depth > 0)
            {
            	if (nested_comments)
            	{
            		if (text[0] == '/' && text[1] == '*')
            		{	nesting_depth++;
            			text.next();
            		}
            		else if (text[0] == '*' && text[1] == '/')
            		{	nesting_depth--;
            			text.next();
            		}
            	}
                text.next();
            }
            if (text.eof())
            {   //error("Warning: %d.%d comment not terminated\n");
            }
            text.next();
            text.next();
        }
        else
            break;
    }

	/* Fixed part. Do not modify!! */
    _last_space_end_pos = text;
    /* _print_state(); */
}


bool ResourceScanner::acceptEOF(TextFileBuffer& text)
{
	return text.eof();
}

bool ResourceScanner::acceptLiteral(TextFileBuffer& text, const char* sym)
{
	if (IDENT_START_CHAR(*sym))
	{
		TextFilePos start_pos = text;

		Ident ident;
		bool is_keyword;
		if (accept_ident(text, ident, is_keyword, sym) && ident == sym)
			return true;

		text = start_pos;
		return false;
	}
	
    int i;
    const char *s;
    for (i = 0, s = sym; *s != '\0' && text[i] == *s; i++, s++);

	if (*s != '\0')
		return false;
	
	if (i == 1)
		text.next();
	else
		text.advance(i);
    skipSpace(text);

    return true;
}

bool ResourceScanner::acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result)
{
	static Ident id_comment = "comment";
	static Ident id_ident = "ident";
	static Ident id_string = "string";
	static Ident id_int = "int";
	static Ident id_hexint = "hexint";
	static Ident id_hexintL = "hexintL";
	
	if (name == id_comment)
		return accept_comment(text, result);
	if (name == id_ident)
		return accept_ident(text, result);
	if (name == id_string)
		return accept_string(text, result);
	//if (name == id_char)
	//	return accept_char(text, result);
	if (name == id_int || name == id_hexint || name == id_hexintL)
		return accept_int(text, result);
	//if (name == id_double)
	//	return accept_double(text, result);

	return false;
}

bool ResourceScanner::acceptWhiteSpace(TextFileBuffer& text, Ident name)
{
	static Ident id_cnl = "cnl";
	static Ident id_stnl = "stnl";
	static Ident id_cm = "cm";

	if (name == id_cnl || name == id_stnl)
	{
		if (!text.eof() && *text == '\n')
		{
			text.next();
			skipSpace(text);
		}
		return true;
	}
	if (name == id_cm)
	{
		if (!text.eof() && *text == ',')
		{
			text.next();
			skipSpace(text);
			if (!text.eof() && *text == '\n')
			{
				text.next();
				skipSpace(text);
			}
			return true;
		}
		return false;
	}
	return true;
}

bool ResourceScanner::accept_comment(TextFileBuffer& text, AbstractParseTree& result)
{
	if (text.eof() || text[0] != '/' || text[1] != '/')
		return false;
	
	text.advance(2);
    TextFilePos sp = text;

	String comment;
	String::filler filler(comment);
    
    while (!text.eof() && *text != '\0' && *text != '\n')
    {
		filler << *text;
		text.next();
	}
	filler << '\0';

	skipSpace(text);

	result = comment;

    return true;
}

bool ResourceScanner::accept_ident(TextFileBuffer& text, Ident& ident, bool& is_keyword, const char* keyword)
{
	if (text == _last_ident_pos)
	{
		ident = _last_ident;
		is_keyword = _last_ident_is_keyword;
		text = _last_ident_end_pos;

		return true;
	}

	/* Does it look an identifier? */
    if (text.eof() || !IDENT_START_CHAR(*text))
        return false;

	_last_ident_pos = text;

	/* Determine the lengte of the identifier */
    int i = 1;
    while (IDENT_FOLLOW_CHAR(text[i])) 
        i++;

	{	
		assert(i < 200);
    	char buf[201];
		int j;
		for (j = 0; j < i && j < 200; j++)
			buf[j] = text[j];
		buf[j] = '\0';
		ident = buf;
	}

	text.advance(i);
	skipSpace(text);

	is_keyword = (keyword != 0 && ident == keyword) || _grammar->isLiteral(ident);
	
	_last_ident_end_pos = text;
	_last_ident = ident;
	_last_ident_is_keyword = is_keyword;

	return true;
}


bool ResourceScanner::accept_ident(TextFileBuffer& text, AbstractParseTree& result)
{
	TextFilePos start_pos = text;

	Ident ident;
	bool is_keyword;
	if (accept_ident(text, ident, is_keyword, 0) && !is_keyword)
	{
		result = ident;
		return true;
	}

	text = start_pos;
	return false;
}

bool ResourceScanner::accept_string(TextFileBuffer& text, AbstractParseTree& result)
{
    if (text.eof() || text[0] != '"')
        return false;

    text.next();

	String string;
	String::filler filler(string);
    
    for (;;)
    {
        if (text.eof() || *text == '\0' || *text == '\n')
        {   
        	//error("incorrectly terminated string");
            break;
        }
        else if (*text == '"')
        {   
			text.next();
			if (!text.eof() && *text == '"')
			{
				filler << '"';
				text.next();
			}
			else
				break;
        }
        else
        {   
			filler << *text;
            text.next();
        }
    }
	filler << '\0';

	skipSpace(text);

	result = string;
    return true;
}

bool ResourceScanner::accept_char(TextFileBuffer& text, AbstractParseTree& result)
{
    if (text.eof() || *text != '\'')
    	return false;

	char v;

    text.next();
    if (*text == '\\')
    {
        text.next();
        if ('0' <= *text && *text <= '7')
        {   char d1 = *text;

            v = *text - '0';
            text.next();
            if ('0' <= *text && *text <= '7')
            {   v = v*8 + *text - '0';
                text.next();
                if ((d1 == '0' || d1 == '1') && '0' <= *text && *text <= '7')
                {   v = v*8 + *text - '0';
                    text.next();
                }
            }
        }
        else
		{
			if (*text == 'n')
	            v = '\n';
			else if (*text == 'r')
	            v = '\r';
			else if (*text == 't')
				v = '\t';
			else
				v = *text;
			text.next();
		}
    }
    else
	{
        v = *text;
		text.next();
	}
    if (*text != '\'')
    {   // error("ill terminated character");
    }
    else
    {   text.next();
    }

	skipSpace(text);

	result = v;
    return true;
}

bool ResourceScanner::accept_int(TextFileBuffer& text, AbstractParseTree& result)
{
    if (text.eof() || (   !isdigit(*text)
                 && (   (*text != '+' && *text != '-') 
                     || !isdigit(text[1]))))
        return false;

    int i = 0;
	long v = 0;
	bool minus = false;
    if (text[i] == '-' || text[i] == '+')
	{
		minus = text[i] == '-';
        i++;
	}
    if (text[i] == '0' && text[i+1] == 'x')
    {   i += 2;
        for (;; i++)
			if (isdigit(text[i]))
				v = 16 * v + text[i] - '0';
			else if ('a' <= text[i] && text[i] <= 'f') 
				v = 16 * v + 10 + text[i] - 'a';
			else if ('A' <= text[i] && text[i] <= 'F') 
				v = 16 * v + 10 + text[i] - 'A';
			else
				break;
    }
    else
    {   
		for (; isdigit(text[i]); i++)
			v = v * 10 + text[i] - '0';
        if (text[i] == '.')
            return false;
    }
    if (text[i] == 'L')
        i++;

    text.advance(i);
	skipSpace(text);

	result = minus ? -v : v;
    return true;
}

bool ResourceScanner::accept_double(TextFileBuffer& text, AbstractParseTree& result)
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
	double v;
    sscanf(text, "%lf", &v);

    text.advance(i);
	skipSpace(text);

	result = v;
    return true;
}












