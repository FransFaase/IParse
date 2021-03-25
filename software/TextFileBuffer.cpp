#include <string.h>
#include <stdio.h>
#include "TextFileBuffer.h"

TextFileBuffer::TextFileBuffer()
{
	_buffer = 0;
	_utf8encoded = false;
	_tab_size = 4;
}


void TextFileBuffer::assign(const char* str, unsigned long len, bool utf8encoded /* = false */, unsigned int tab_size /* = 4 */)
{
	_buffer = str;
	_len = len;
	_pos = 0;
	_line = 1;
	_column = 1;
	_info = _buffer;
	_utf8encoded = utf8encoded;
	_tab_size = tab_size;
}

void TextFileBuffer::next()
{
    if (_pos < _len)
    {
      switch(*_info)
      {   case '\t':
              _column += _tab_size - (_column - 1) % _tab_size;
              break;
          case '\n':
              _line++;
              _column = 1;
              break;
          default:
			  if (!_utf8encoded || (*_info & 0xC0) != 0x80)
				_column++;
              break;
      }
      _pos++;
      _info++;
    }
}

void TextFileBuffer::advance(unsigned int steps)
{
    _pos += steps;
    if (_pos > _len)
        _pos = _len;
    _info = _buffer + _pos;
    _column += steps;
}

void TextFileBuffer::print_state()
{
    printf("[0,%ld,%ld] = `%c'\n", _pos, _len, *_info);
}

const char* TextFileBuffer::start()
{   static char buff[13];
    int i;
    const char *s;

    for (i = 0, s = _info; i < 10 && *s; s++)
        if (*s == '\n')
        {   buff[i++] = '\\';
            buff[i++] = 'n';
        }
        else if (*s == '\t')
        {   buff[i++] = '\\';
            buff[i++] = 't';
        }
        else
            buff[i++] = *s;

    buff[i] = '\0';
    return buff;
}

