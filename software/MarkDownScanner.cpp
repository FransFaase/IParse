#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"
#include "Scanner.h"
#include "MarkDownScanner.h"
#include "ParserGrammar.h"


#define IDENT_START_CHAR(X) (isalpha(X)||X=='_')
#define IDENT_FOLLOW_CHAR(X) (isalpha(X)||isdigit(X)||X=='_')

void MarkDownScanner::initScanning(Grammar* grammar)
{
	AbstractScanner::initScanning(grammar);
	_last_space_pos.clear();
	_in_text = true;
}

Ident MarkDownScanner::id_ident = "ident";
Ident MarkDownScanner::id_string = "string";
Ident MarkDownScanner::id_cstring = "cstring";
Ident MarkDownScanner::id_char = "char";
Ident MarkDownScanner::id_int = "int";
Ident MarkDownScanner::id_double = "double";

void MarkDownScanner::skipSpace(TextFileBuffer& text)
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
	{   
		if (_in_text)
		{
			if (text.eof())
			{
				break;
			}
			if (text.column() == 1 && text.left() >= 5 && strcmp(text, "```c") == 0 && (text[4] == '\r' || text[4] == '\n'))
			{
				_in_text = false;
			}
			while (!text.eof() && text[0] != '\n')
				text.next();
		}
		else
		{
			if (*text == ' ' || *text == '\t' || text[0] == '\n' || text[0] == '\r')
			{
				text.next();
			}
			else if (cpp_comments && text[0] == '/' && text[1] == '/')
			{
				while (!text.eof() && text[0] != '\n')
					text.next();
			}
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
			if (text.column() == 1 && text.left() >= 4 && strcmp(text, "```") == 0 && (text[3] == '\r' || text[3] == '\n'))
			{
				while (!text.eof() && text[0] != '\n')
					text.next();
				_in_text = true;				
			}
			else
				break;
		}
	}

	/* Fixed part. Do not modify!! */
	_last_space_end_pos = text;
	/* _print_state(); */
}


bool MarkDownScanner::acceptEOF(TextFileBuffer& text)
{
	return text.eof();
}

bool MarkDownScanner::acceptLiteral(TextFileBuffer& text, const char* sym)
{
	if (*sym == '\0')
		return true;

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
		
	text.advance(i);
	skipSpace(text);

	return true;
}

bool MarkDownScanner::acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result)
{
	if (name == id_ident)
		return accept_ident(text, result);
	if (name == id_string)
		return accept_string(text, result, false);
	if (name == id_cstring)
		return accept_string(text, result, true);
	if (name == id_char)
		return accept_char(text, result);
	if (name == id_int)
		return accept_int(text, result);
	if (name == id_double)
		return accept_double(text, result);

	return false;
}

bool MarkDownScanner::acceptWhiteSpace(TextFileBuffer& text, Ident name)
{
	static Ident id_notamp = "notamp";
	if (name == "notamp")
	{
		if (!text.eof() && *text == '&')
			return false;
	}
	return true;
}

bool MarkDownScanner::accept_ident(TextFileBuffer& text, Ident& ident, bool& is_keyword, const char* keyword)
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


bool MarkDownScanner::accept_ident(TextFileBuffer& text, AbstractParseTree& result)
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


bool MarkDownScanner::accept_string(TextFileBuffer& text, AbstractParseTree& result, bool c_string)
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

	text.next();

	String string;
	String::filler filler(string);
	
	for (;;)
	{
		if (text.eof() || *text == '\0' || *text == '\n')
		{   
			break;
		}
		else if (*text == '\\')
		{
			text.next();
			if (!text.eof() && '0' <= *text && *text <= '7')
			{   char d1 = *text,
					 v = *text - '0';
				text.next();
				if (!text.eof() && '0' <= *text && *text <= '7')
				{   v = v*8 + *text - '0';
					text.next();
					if (   !text.eof() && (d1 == '0' || d1 == '1') 
						&& '0' <= *text && *text <= '7')
					{   v = v*8 + *text - '0';
						text.next();
					}
				}
				filler << v;
			}
			else if (!text.eof()) 
			{   if (*text == 'n')
					filler << '\n';
				else if (*text == 'r')
					filler << '\r';
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

bool MarkDownScanner::accept_char(TextFileBuffer& text, AbstractParseTree& result)
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

bool MarkDownScanner::accept_int(TextFileBuffer& text, AbstractParseTree& result)
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

bool MarkDownScanner::accept_double(TextFileBuffer& text, AbstractParseTree& result)
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



