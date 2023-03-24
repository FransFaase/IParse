#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "Ident.h"
#include "XMLIterator.h"

XMLIterator::XMLIterator(const TextFileBuffer& textBuffer)
{
	_text = textBuffer;
	_state = ' ';
	next();
}

void XMLIterator::skip_spaces()
{
	while (!_text.eof() && isspace((unsigned char)*_text))
		_text.next();
}


void XMLIterator::next()
{
	if (_state == '\0' || _state == 'e')
		return;
	
	skip_spaces();

	if (_text.eof())
	{
		_state = '\0';
		return;
	}
	
	if (_state == 'o' || _state == 'a')
	{
		if (*_text == '/')
		{
			_text.next();
			skip_spaces();
			if (*_text != '>')
			{
				_state = 'e';
				return;
			}
			_text.next();
			_state = 'c';
			return;
		}
		else if (*_text == '>')
		{
			_text.next();
			skip_spaces();
			if (_text.eof())
			{
				_state = '\0';
				return;
			}
		}
		else
		{
			line = _text.line();
			column = _text.column();

			int i = 0;
			while (!_text.eof() && !isspace((unsigned char)*_text) && *_text != '>' && *_text != '/' && *_text != '=')
			{
				if (i < 99)
					attr[i++] = *_text;
				_text.next();
			}
			attr[i] = '\0';
			if (_text.eof() || *_text != '=')
			{
				_state = 'e';
				return;
			}
			_text.next();
			if (_text.eof() || *_text != '"')
			{
				_state = 'e';
				return;
			}
			_text.next();
			parse_string('"');
			if (_text.eof() || *_text != '"')
			{
				_state = 'e';
				return;
			}
			_text.next();
			_state = 'a';
			return;
		}
	}
	
	line = _text.line();
	column = _text.column();

	if (*_text == '<')
	{
		if (_text[1] == '/')
		{
			_text.advance(2);
			int i = 0;
			while (!_text.eof() && !isspace((unsigned char)*_text) && *_text != '>')
			{
				if (i < 99)
					tag[i++] = *_text;
				_text.next();
			}
			tag[i] = '\0';
			if (*_text != '>')
			{
				_state = 'e';
				return;
			}
			_text.next();
			_state = 'c';
			return;
		}
		if (_text[1] == '?')
		{
			_text.advance(2);
			String::filler filler(value);
			while (!_text.eof() && *_text != '?')
			{
				filler << *_text;
				_text.next();
			}
			filler << '\0';
			if (!_text.eof() && *_text == '?')
				_text.next();
			if (!_text.eof() && *_text == '>')
				_text.next();
			_state = 'm';
			return;
		}
		if (_text[1] == '!')
		{
			_text.advance(2);
			String::filler filler(value);
			while (!_text.eof() && *_text != '>')
			{
				filler << *_text;
				_text.next();
			}
			filler << '\0';
			if (!_text.eof() && *_text == '>')
				_text.next();
			_state = '!';
			return;
		}
		_text.next();
		char name[100];
		int i = 0;
		while (!_text.eof() && !isspace((unsigned char)*_text) && *_text != '>' && *_text != '/')
		{
			if (i < 99)
				tag[i++] = *_text;
			_text.next();
		}
		tag[i] = '\0';
		skip_spaces();
		_state = 'o';
		return;
	}
	
	parse_string('<');
	_state = 't';
}

void XMLIterator::parse_string(char terminator)
{
	String::filler filler(value);
	while (!_text.eof() && *_text != terminator)
	{
		if (*_text == '&')
		{
			if (strncmp(_text, "&lt;", 4) == 0)
			{
				filler << '<';
				_text.advance(4);
			}
			else if (strncmp(_text, "&gt;", 4) == 0)
			{
				filler << '>';
				_text.advance(4);
			}
			else if (strncmp(_text, "&amp;", 5) == 0)
			{
				filler << '&';
				_text.advance(5);
			}
			else
			{
				filler << '?';
				_text.next();
				while (!_text.eof() && *_text != ';')
					_text.next();
				if (!_text.eof() && *_text == ';')
					_text.next();
			}
		}
		else
		{
			filler << *_text;
			_text.next();
		}
	}
	filler << '\0';
}

