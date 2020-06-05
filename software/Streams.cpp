#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "Streams.h"

void CharStreamToFile::emit(char symbol)
{
	if (_text && symbol == '\n')
		fputc('\r', _f);
	fputc(symbol, _f);
}

void UnicodeToUTF8ConverterStream::emit(UnicodeChar symbol)
{
	if (symbol < 0x80)
	{
		_out->emit((char)(unsigned char)symbol);
	}
	else if (symbol <= 0x7FF)
	{
		_out->emit((char)(unsigned char)((symbol >> 6) | 0xC0));
		_out->emit((char)(unsigned char)((symbol & 0x3F) | 0x80));
	}
	else if (symbol <= 0xFFFF)
	{
    	_out->emit((char)(unsigned char)((symbol >> 12) | 0xE0));
    	_out->emit((char)(unsigned char)(((symbol >> 6) & 0x3F) | 0x80));
    	_out->emit((char)(unsigned char)((symbol & 0x3F) + 0x80));
	}
	else if (symbol <= 0x10FFFF)
	{
    	_out->emit((char)(unsigned char)((symbol >> 18) | 0xF0));
    	_out->emit((char)(unsigned char)(((symbol >> 12) & 0x3F) | 0x80));
    	_out->emit((char)(unsigned char)(((symbol >> 6) & 0x3F) | 0x80));
    	_out->emit((char)(unsigned char)((symbol & 0x3F) | 0x80));
	}
}

void UTF16ToUTF8ConverterStream::emit(char symbol)
{
	_count++;
	if (_count % 2 == 1)
	{
		_prev = symbol;
	}
	else if (_count == 2 && ((unsigned char)_prev == 0xFE && (unsigned char)symbol == 0xFF))
	{
		// BOM
	}
	else if (_count == 2 && ((unsigned char)_prev == 0xFF && (unsigned char)symbol == 0xFE))
	{
		// BOM
	}
	else if (_count == 2 && ((unsigned char)_prev == 0x00 && (unsigned char)symbol == 0x00))
	{
		// EOF character
	}
	else
	{
		_unicodeToUTF8ConverterStream.emit((long)(unsigned char)_prev | ((long)(unsigned char)symbol << 8));
	}
}

void CodePageToUnicodeConverterStream::emit(char symbol)
{
	UnicodeChar code_point;
	if (_codePage.from(symbol, code_point))
		_out->emit(code_point);
}

void UTF8ToUnicodeConverterStream::emit(char symbol)
{
	unsigned char value = (unsigned char)symbol;
	if ((value & 0xC0) == 0x80)
	{
		if (_following == 0)
		{
			// error: out of place following char
			return;
		}
		_code_point = (_code_point << 6) | (value & 0x7F);
		if (--_following == 0)
			_out->emit(_code_point);
		return;
	}
	if (_following > 0)
	{
		// error: expecting more following chars
		_following = 0;
		return;
	}
	if ((value & 0x80) == 0x00)
	{
		_out->emit(value);
	}
	else if ((value & 0xE0) == 0xC0)
	{
		_code_point = value & 0x1F;
		_following = 1;
	}
	else if ((value & 0xF0) == 0xE0)
	{
		_code_point = value & 0x0F;
		_following = 2;
	}
	else if ((value & 0xF8) == 0xF0)
	{
		_code_point = value & 0x07;
		_following = 3;
	}
	else if ((value & 0xFC) == 0xF8)
	{
		_code_point = value & 0x03;
		_following = 4;
	}
	else
	{
		// error: incorrect start character
	}
}

void UnicodeToCodePageConverterStream::emit(UnicodeChar symbol)
{
	char ch;
	if (_codePage.to(symbol, ch))
		_out->emit(ch);
}

void UnicodeToUTF16ConverterStream::emit(UnicodeChar symbol)
{
	_out->emit((char)(unsigned char)(symbol & 0xFF));
	_out->emit((char)(unsigned char)((symbol >> 8) & 0xFF));
}

