#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"
#include "TextFileBuffer.h"
#include "Scanner.h"
#include "ParserGrammar.h"


#define IDENT_START_CHAR(X) (isalpha(X)||X=='_')
#define IDENT_FOLLOW_CHAR(X) (isalpha(X)||isdigit(X)||X=='_')

void BasicScanner::initScanning(Grammar* grammar)
{
	AbstractScanner::initScanning(grammar);
	_last_space_pos.clear();
}

Ident BasicScanner::id_ident = "ident";
Ident BasicScanner::id_string = "string";
Ident BasicScanner::id_cstring = "cstring";
Ident BasicScanner::id_char = "char";
Ident BasicScanner::id_int = "int";
Ident BasicScanner::id_double = "double";
Ident BareScanner::id_ws = "ws";
Ident BareScanner::id_nows = "nows";

void BasicScanner::skipSpace(TextFileBuffer& text)
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
	{   if (*text == ' ' || *text == '\t' || text[0] == '\n')
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
		else
			break;
	}

	/* Fixed part. Do not modify!! */
	_last_space_end_pos = text;
	/* _print_state(); */
}


bool BasicScanner::acceptEOF(TextFileBuffer& text)
{
	return text.eof();
}

bool BasicScanner::acceptLiteral(TextFileBuffer& text, const char* sym)
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

bool BasicScanner::acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result)
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

bool BasicScanner::acceptWhiteSpace(TextFileBuffer& text, Ident name)
{
	static Ident id_notamp = "notamp";
	if (name == "notamp")
	{
		if (!text.eof() && *text == '&')
			return false;
	}
	return true;
}

bool BasicScanner::accept_ident(TextFileBuffer& text, Ident& ident, bool& is_keyword, const char* keyword)
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


bool BasicScanner::accept_ident(TextFileBuffer& text, AbstractParseTree& result)
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


bool BasicScanner::accept_string(TextFileBuffer& text, AbstractParseTree& result, bool c_string)
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

bool BasicScanner::accept_char(TextFileBuffer& text, AbstractParseTree& result)
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

bool BasicScanner::accept_int(TextFileBuffer& text, AbstractParseTree& result)
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

bool BasicScanner::accept_double(TextFileBuffer& text, AbstractParseTree& result)
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


Ident WhiteSpaceScanner::id_ws = "ws";

void WhiteSpaceScanner::skipSpace(TextFileBuffer& text)
{
}

bool WhiteSpaceScanner::acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result)
{
	if (name == id_ws)
		return accept_whitespace(text, result);
	
	return BasicScanner::acceptTerminal(text, name, result);
}

bool WhiteSpaceScanner::accept_whitespace(TextFileBuffer& text, AbstractParseTree& result)
{
	bool cpp_comments = true;
	bool nested_comments = true;

	/* Fixed part. Do not modify!! */
	if (text == _last_space_pos)
	{   text = _last_space_end_pos;
		result = ws;
		return true;
	}
	_last_space_pos = text;
	ws.clear();
	String::filler filler(ws);

	/* Flexible part. 
	   The code below increments scans the white space 
	   characters.
	*/
	for(;;)
	{   if (*text == ' ' || *text == '\t' || text[0] == '\n')
		{
			filler << *text;
			text.next();
		}
		else if (cpp_comments && text[0] == '/' && text[1] == '/')
		{
			while (!text.eof() && text[0] != '\n')
			{
				filler << *text;
				text.next();
			}
		}
		else if (text[0] == '/' && text[1] == '*')
		{   int nesting_depth = 1;
			TextFilePos sp = text;
 
 			filler << *text;
 			text.next();
 			filler << *text;
 			text.next();
			while (!text.eof() && !(text[0] == '*' && text[1] == '/') && nesting_depth > 0)
			{
				if (nested_comments)
				{
					if (text[0] == '/' && text[1] == '*')
					{	nesting_depth++;
						filler << *text;
						text.next();
					}
					else if (text[0] == '*' && text[1] == '/')
					{	nesting_depth--;
						filler << *text;
						text.next();
					}
				}
				filler << *text;
				text.next();
			}
			if (text.eof())
			{   //error("Warning: %d.%d comment not terminated\n");
			}
			filler << *text;
			text.next();
			filler << *text;
			text.next();
		}
		else
			break;
	}

	/* Fixed part. Do not modify!! */
	filler << '\0';
	result = ws;
	_last_space_end_pos = text;
	/* _print_state(); */
	
	return true;
}

Ident ColourCodingScanner::id_colourcommand = "colourcommand";

bool
ColourCodingScanner::acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result)
{
	if (name == id_colourcommand)
		return accept_colourcommand(text, result);
	
	return BasicScanner::acceptTerminal(text, name, result);
}

bool ColourCodingScanner::accept_colourcommand(TextFileBuffer& text, AbstractParseTree& result)
{
	if (*text != '$')
		return false;
	text.next();

	char type = ' ';
	if (*text == '<' || *text == '>' || *text == '!')
	{
		type = *text;
		text.next();
	}

	long fgcolour = 0;
	accept_colourcode(text, fgcolour);
	long bgcolour = 0;
	if (*text == ',')
	{
		text.next();
		accept_colourcode(text, bgcolour);
	}
	
	result.createTree("colourcommand");
	result.appendChild(AbstractParseTree(type));
	result.appendChild(AbstractParseTree(fgcolour));
	result.appendChild(AbstractParseTree(bgcolour));

	return true;
}

void ColourCodingScanner::accept_colourcode(TextFileBuffer& text, long &code)
{
	TextFilePos sp = text;
	code = 0;
	for (int i = 0; i < 6; i++)
	{
		if (text.eof())
		{
			code = -1;
			text = sp;
			return;
		}
		if ('0' <= *text && *text <= '9')
			code = code * 16 + *text - '0';
		else if ('a' <= *text && *text <= 'f')
			code = code * 16 + 10 + *text - 'a';
		else if ('A' <= *text && *text <= 'F')
			code = code * 16 + 10 + *text - 'F';
		else
		{
			code = -1;
			text = sp;
			return;
		}
		text.next();
	}
	return;
}




void RawScanner::skipSpace(TextFileBuffer& text)
{
}

bool RawScanner::acceptEOF(TextFileBuffer& text)
{
	return text.eof();
}

bool RawScanner::acceptLiteral(TextFileBuffer& text, const char* sym)
{
	if (*sym == '\0')
		return true;

	int i;
	const char *s;
	for (i = 0, s = sym; *s != '\0' && text[i] == *s; i++, s++);

	if (*s != '\0')
		return false;
		
	for (; i > 0; i--)
		text.next();
	return true;
}

bool RawScanner::acceptTerminal(TextFileBuffer& text, Ident name, AbstractParseTree& result)
{
	return false;
}

bool RawScanner::acceptWhiteSpace(TextFileBuffer& text, Ident name)
{
	return true;
}


void BareScanner::initScanning(Grammar* grammar)
{
	RawScanner::initScanning(grammar);
	_last_space_pos.clear();
}

void BareScanner::skipSpace(TextFileBuffer& text)
{
	if (text == _last_space_pos)
	{   
		printf("repeate skip space %d.%d to %d.%d\n",
			_last_space_pos.line(), _last_space_pos.column(),
			_last_space_end_pos.line(), _last_space_end_pos.column());
		text = _last_space_end_pos;
		return;
	}
	/*if (text == _last_space_end_pos)
	{
		printf("At last skip space pos\n");
		return;
	}*/

	_last_space_pos = text;

	while (!text.eof() && (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r'))
		text.next();

	_last_space_end_pos = text;
	printf("first skip space %d.%d to %d.%d\n",
			_last_space_pos.line(), _last_space_pos.column(),
			_last_space_end_pos.line(), _last_space_end_pos.column());
	if (_last_space_end_pos.line() == 345 && _last_space_end_pos.column() == 10)
		printf("Break\n");
}

bool BareScanner::acceptLiteral(TextFileBuffer& text, const char* sym)
{
	if (*sym == '\0')
		return true;

	if (!RawScanner::acceptLiteral(text, sym))
		return false;
	skipSpace(text);
	return true;
}

bool BareScanner::acceptWhiteSpace(TextFileBuffer& text, Ident name)
{
	if (name == id_nows)
	{
		if (text == _last_space_end_pos)
		{
			printf("\\nows back to %d.%d\n", _last_space_pos.line(), _last_space_pos.column());
			text = _last_space_pos;
		}
		else
			printf("\\nows no action\n");
	}
	else if (name == id_ws)
		skipSpace(text);

	return true;
}









#if 0

// Below follow some scanning procedures from the original C program
// which have not been converted to this C++ version.



/*** Transact-SQL scanning procedures ***/

#define ADD_CHAR(C) { if (string) string[i] = C; i++; }

bool try_accept_sql_string(TextFileBuffer& text, String &string)
{
	char quote = text[0];
	TextFilePos sp = text;

	String::filler filler(string);

	text.next();
	for (;;)
	{
		if (text.eof() || *text == '\0' || *text == '\n')
		{   
			printf("Error: %d.%d incorrectly terminated string\n",
				   sp.cur_line(), sp.cur_column());
			return false;
		}
		else if (*text == quote)
		{
			text.next();
			if (!text.eof() && *text == quote)
			{
				filler << quote;
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

	return true;
}

bool accept_both_sql_string(TextFileBuffer& text, String &string, char quote)
{	
	if (text.eof() || (text[0] != quote))
	{   expected_string(text, "<string>", 0);
		DEBUG_F_P3("%d.%d: accept_sql_string() failed for: `%s'\n",
				 text.cur_line(), text.cur_column(), text.start());
		return false;
	}

	TextFilePos sp = text;

	if (!try_accept_sql_string(string, &i))
		return false;

	skip_space(text);

	DEBUG_P4("%d.%d: accept_sql_string(\"%s\") %d\n",
			 sp.cur_line(), sp.cur_column(), *string, cur_column);
	return true;
}

bool accept_single_sql_string(TextFileBuffer& text, String &string)
{
	return accept_both_sql_string(string, '\'');
}

bool accept_double_sql_string(TextFileBuffer& text, String &string)
{
	return accept_both_sql_string(string, '"');
}

int last_ident_ats;
bool last_ident_is_label;

bool try_accept_sql_ident(TextFileBuffer& text, const char *what, int nr_ats, bool is_label, Ident& ident, bool (*is_keyword)(Ident))
{
	int i, j;
	int cnt_ats;
	long start_line;
	long start_column;

	if (file_pos == last_ident_pos)
	{ 
		if (last_ident_is_keyword || nr_ats != last_ident_ats || is_label != last_ident_is_label)
		{   expected_string(what, 0);
			DEBUG_F_P3("%d.%d: accept_sql_ident() failed for `%s'\n",
					   text.cur_line(), text.cur_column(), text.start());
			return false;
		}
		start_column = cur_column;
		ident = last_ident;
		text = last_ident_end_pos;
	}
	else
	{   longword start_pos = file_pos; 
		char buffer[1000];

		start_line = cur_line;
		start_column = cur_column;

		if (text.eof() || (!isalpha(*text) && *text != '_' && *text != '#' 
			&& *text != '$' && *text != '@'))
		{   expected_string(what, 0);
			DEBUG_F_P3("%d.%d: accept_sql_ident() failed for `%s'\n",
					 cur_line, start_column, start_info());
			return false;
		}
  
		i = 0;
		cnt_ats = 0;
		while (text[i] == '@')
		{
			i++;
			cnt_ats++;
		}
		if ((!isalpha(text[i]) && !isdigit(text[i]) && text[i] != '_'
			&& *text != '#' && *text != '$' && *text != '.')
			|| cnt_ats != nr_ats)
		{   expected_string(what, 0);
			DEBUG_F_P3("%d.%d: accept_sql_ident() failed for `%s'\n",
					 cur_line, start_column, start_info());
			return false;
		}
		j = 0;
		while (isalpha(text[i]) || isdigit(text[i]) || text[i] == '_')
		{
			if (j < 999)
			   buffer[j++] = tolower(text[i]);
			i++;
		}
		buffer[j] = '\0';
		ident = buffer;
		if (_grammar->isLiteral(ident))
		{   expected_string(what, ident.val());
			DEBUG_F_P3("%d.%d: accept_sql_ident() failed, because `%s' is a keyword\n",
					 cur_line, start_column, ident.val());
			return false;
		}
		if (text[i] == ':' && !is_label)
		{   expected_string(what, ident.val());
			DEBUG_F_P3("%d.%d: accept_sql_ident() failed, because `%s' is not a label\n",
					 cur_line, start_column, ident.val());
			return false;
		}
		if (text[i] != ':' && is_label)
		{   expected_string(what, ident.val());
			DEBUG_F_P3("%d.%d: accept_sql_ident() failed, because `%s' is a label\n",
					 cur_line, start_column, ident.val());
			return false;
		}
		
		if (text[i] == ':')
		{
			i++;
		}

		_advance(i);

		last_ident_pos = start_pos;
		last_ident = ident.val();
		last_ident_ats = nr_ats;
		last_ident_is_label = is_label;
		last_ident_is_keyword = false;

		skip_space(text);

		last_ident_end_pos = text;
	}
	
	DEBUG_P3("%d.%d: accept_sql_ident(%s)\n", start_line, start_column, ident.val());
	return true; 
}

bool accept_sql_ident(TextFileBuffer& text, Ident& ident, bool (*is_keyword)(Ident))
{
	return try_accept_sql_ident(text, "<sql_ident>", 0, false, ident, is_keyword);
}
bool accept_sql_var_ident(TextFileBuffer& text, Ident& ident, bool (*is_keyword)(Ident))
{
	return try_accept_sql_ident(text, "<sql_var_ident>", 1, false, ident, is_keyword);
}
bool accept_sql_sysvar_ident(TextFileBuffer& text, Ident& ident, bool (*is_keyword)(Ident))
{
	return try_accept_sql_ident(text, "<sql_sysvar_ident>", 2, false, ident, is_keyword);
}
bool accept_sql_label_ident(TextFileBuffer& text, Ident& ident, bool (*is_keyword)(Ident))
{
	return try_accept_sql_ident(text, "<sql_label_ident>", 0, true, ident, is_keyword);
}



#endif

