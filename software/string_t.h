#ifndef INCLUDED_STRING_T_H
#define INCLUDED_STRING_T_H

struct string_t
{
	string_t(const char *str) : refcount(1)
	{
		value = new char[strlen(str)+1];
		strcpy(value, str);
	}
	string_t(const char *str, const char *till) : refcount(1)
	{
		size_t len = 0;
		while (str + len != till && str[len] != '\0')
			len++;
		value = new char[len+1];
		strncpy(value, str, len);
		value[len] = '\0';
	}
	string_t(int len) : refcount(1)
	{
		value = new char[len+1];
		memset(value, '\0', len+1);
	}
	~string_t() { delete value; }

	long refcount;
	char *value;
};
#endif // INCLUDED_STRING_T_H
