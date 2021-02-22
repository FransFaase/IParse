#include <string.h>
#include "String.h"

#include "string_t.h"

String::String(const char *str)
{
	_str = new string_t(str);
}
String::String(const char *str, const char *till)
{
	_str = new string_t(str, till);
}
String::String(string_t *str)
{
	_str = str;
	if (_str != 0)
		_str->refcount++;
}
String::~String()
{
	clear();
}
void String::clear()
{
	if (_str != 0)
	{
		if (--_str->refcount == 0)
			delete _str;
		_str = 0;
	}
}
String& String::operator=(const char *rhs)
{
	if (_str != 0 && --_str->refcount == 0)
		delete _str;
	_str = rhs != 0 ? new string_t(rhs) : 0;
	return *this;
}

String& String::operator=(string_t *rhs)
{	
	string_t *old_str = _str;
	_str = rhs;
	if (_str != 0)
		_str->refcount++;
	if (old_str != 0 && --old_str->refcount == 0)
		delete old_str;
	return *this;
}

String& String::operator=(const String &rhs)
{
	string_t *old_str = _str;
	_str = rhs._str;
	if (_str != 0)
		_str->refcount++;
	if (old_str != 0 && --old_str->refcount == 0)
		delete old_str;
	return *this;
}

int String::compare(const String& rhs) const
{	return strcmp(*this, rhs);
}

bool String::operator==(const String& rhs) const
{	return strcmp(*this, rhs) == 0;
}

bool String::operator==(const char* rhs) const
{	return strcmp(*this, rhs) == 0;
}

bool String::operator!=(const String& rhs) const
{	return strcmp(*this, rhs) != 0;
}

bool String::operator>(const String& rhs) const
{	return strcmp(*this, rhs) > 0;
}

bool String::operator<(const String& rhs) const
{	return strcmp(*this, rhs) < 0;
}

String::operator const char*() const
{	return _str != 0 ? _str->value : "";
}

const char* String::val() const
{	return _str != 0 ? _str->value : 0;
}

bool String::empty() const
{	return _str == 0;
}

String::filler::filler(String &str)
 : _str(str)
{
	_str.clear();
	_alloced = 0;
	_i = 0;
	_closed = false;
	_s = 0;
}
String::filler::~filler() { (*this) << '\0'; }
String::filler& String::filler::operator<<(char ch)
{
	if (_closed)
		return *this;
	if (_i == 1000)
	{
		char *new_s = new char[_alloced+1000];
		if (_alloced > 0)
			strncpy(new_s, _s, _alloced);
		strncpy(new_s + _alloced, _buffer, 1000);
		delete[] _s;
		_s = new_s;
		_alloced += 1000;
		_i = 0;
	}
	_buffer[_i++] = ch;
	if (ch == '\0')
	{
		if (_alloced + _i > 1)
		{
			_str._str = new string_t(_alloced + _i - 1);
			if (_alloced > 0)
			{
				strncpy(_str._str->value, _s, _alloced);
				delete[] _s;
			}
			strncpy(_str._str->value + _alloced, _buffer, _i);
		}
		_closed = true;
	}
	return *this;
}

