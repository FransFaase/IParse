#ifndef INCLUDED_STRING_H
#define INCLUDED_STRING_H

struct string_t;

class AbstractParseTree;

class String
{
	friend class AbstractParseTree;
public:
	String() : _str(0) {}
	String(const String &str);
	String(const char *str);
	String(const char *str, const char *till);
	String(string_t *str);
	~String();
	String &operator=(const char *rhs);
	String &operator=(string_t *rhs);
	String &operator=(const String &rhs);
	void clear();
	int compare(const String& rhs) const;
	bool operator==(const String& rhs) const;
	bool operator==(const char *rhs) const;
	bool operator!=(const String& rhs) const;
	bool operator<(const String& rhs) const;
	bool operator>(const String& rhs) const;
	operator const char*() const;
	const char *val() const;
	bool empty() const;
	class filler
	{
	public:
		filler(String &str);
		~filler();
		filler& operator <<(char ch);
	private:
		char _buffer[1000];
		int _alloced;
		int _i;
		char *_s;
		bool _closed;
		String &_str;
	};
	friend class filler;
private:
	string_t *_str;
};


#endif // INCLUDED_STRING_H
