#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "Ident.h"
#include "XMLParser.h"


bool XMLParser::parse(const TextFileBuffer& textBuffer, AbstractParseTree& result)
{
	_text = textBuffer;

	result.createTree("XML");

	while (parse_xml(result)) {}

	return _text.eof();
}

void XMLParser::skip_spaces()
{
	while (!_text.eof() && isspace((unsigned char)*_text))
		_text.next();
}

bool XMLParser::parse_xml(AbstractParseTree& parent)
{
	TextFilePos _save_pos = _text;
	skip_spaces();

	if (_text.eof())
		return false;

	if (*_text == '<')
	{
		if (_text[1] == '/')
			return false;
		else if (_text[1] == '?')
		{
			_text.advance(2);
			TextFilePos sp = _text;
			int i = 0;
			while (!_text.eof() && *_text != '?')
			{
				_text.next();
				i++;
			}
			String line;
			String::filler filler(line);
			_text = sp;
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
			skip_spaces();

			AbstractParseTree child;
			child.createTree("?");
			child.setLineColumn(sp.line(), sp.column());
			child.appendChild(line);
			parent.appendChild(child);
		}
		else if (_text[1] == '!')
		{
			_text.advance(2);
			TextFilePos sp = _text;
			int i = 0;
			while (!_text.eof() && *_text != '>')
			{
				_text.next();
				i++;
			}
			String line;
			String::filler filler(line);
			_text = sp;
			while (!_text.eof() && *_text != '>')
			{
				filler << *_text;
				_text.next();
			}
			filler << '\0';
			if (!_text.eof() && *_text == '>')
				_text.next();
			skip_spaces();

			AbstractParseTree child;
			child.createTree("!");
			child.setLineColumn(sp.line(), sp.column());
			child.appendChild(line);
			parent.appendChild(child);
		}
		else
		{
			int line = _text.line();
			int column = _text.column();
			_text.next();
			char name[100];
			int i = 0;
			while (!_text.eof() && !isspace((unsigned char)*_text) && *_text != '>' && *_text != '/')
			{
				if (i < 99)
					name[i++] = *_text;
				_text.next();
			}
			name[i] = '\0';
			skip_spaces();
			if (_text.eof())
				return false;

			AbstractParseTree child;
			child.createTree(name);
			child.setLineColumn(line, column);
			parent.appendChild(child);

			while (!_text.eof() && *_text != '>' && *_text != '/')
			{
				line = _text.line();
				column = _text.column();

				char attr_name[100];
				int i = 0;
				while (!_text.eof() && !isspace((unsigned char)*_text) && *_text != '>' && *_text != '/' && *_text != '=')
				{
					if (i < 99)
						attr_name[i++] = *_text;
					_text.next();
				}
				attr_name[i] = '\0';
				if (!_text.eof() && *_text == '=')
				{
					_text.next();
					if (!_text.eof() && *_text == '"')
					{
						_text.next();
						String attr_value;
						parse_string('"', attr_value);
						if (!_text.eof() && *_text == '"')
							_text.next();
						skip_spaces();

						AbstractParseTree attr;
						attr.createTree("=");
						attr.setLineColumn(line, column);
						child.appendChild(attr);
						attr.appendChild(AbstractParseTree(Ident(attr_name)));
						attr.appendChild(AbstractParseTree(attr_value));
					}
				}
			}
			if (*_text == '/')
			{
				_text.next();
				skip_spaces();
				if (!_text.eof() && *_text == '>')
				{
					_text.next();
					skip_spaces();
				}
			}
			else if (*_text == '>')
			{
				_text.next();
				while (parse_xml(child)) {}

				if (!_text.eof() && *_text == '<')
				{
					_text.next();
					if (!_text.eof() && *_text == '/')
					{
						_text.next();
						while (!_text.eof() && *_text != '>')
							_text.next();
						if (!_text.eof() && *_text == '>')
							_text.next();
						skip_spaces();
					}
				}
			}
		}
	}
	else
	{
		_text = _save_pos;
		String content;
		parse_string('<', content);
		parent.appendChild(AbstractParseTree(content));
	}
	return true;
}

void XMLParser::parse_string(char terminator, String &result)
{
	TextFilePos sp = _text;
	int i = 0;
	while (!_text.eof() && *_text != terminator)
	{
		if (*_text == '&')
		{
			if (strncmp(_text, "&lt;", 4) == 0)
				_text.advance(4);
			else if (strncmp(_text, "&gt;", 4) == 0)
				_text.advance(4);
			else if (strncmp(_text, "&amp;", 5) == 0)
				_text.advance(5);
			else
			{
				_text.next();
				while (!_text.eof() && *_text != ';')
					_text.next();
				if (!_text.eof() && *_text == ';')
					_text.next();
			}
		}
		else
			_text.next();
		i++;
	}
	String::filler filler(result);
	_text = sp;
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
		i++;
	}
	filler << '\0';
}
