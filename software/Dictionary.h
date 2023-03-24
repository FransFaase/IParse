#ifndef _INCLUDED_DICTIONARY_H
#define _INCLUDED_DICTIONARY_H

#include "Ident.h"
#include "String.h"
#include "AbstractParseTree.h"

class Translator
{
public:
	virtual bool translate(const String& from_text, String &to_text) const = 0;
	virtual bool translate(const String& from_text, Ident item, String &to_text) const = 0;
	virtual bool translate(const String& from_text, Ident section, Ident item, String &to_text) const = 0;
	virtual bool translate(const String& from_text, Ident section, int nr, String &to_text) const = 0;
};

class Dictionary : public Translator
{
public:
	Dictionary();
	~Dictionary();
	bool empty() { return _entries == 0; }
	void clear();
	void setLanguages(const String& from_lang, const String& to_lang);
	virtual void add(const String& from_text, const String& to_text);
	virtual void add(const Ident& section, const Ident& item, const String& from_text, const String& to_text);
	void establishMajorVariants();
	void printMultiple(FILE *fout);

	const String& fromLang() const { return _from_lang; }
	const String& toLang() const { return _to_lang; }

	bool translate(const String& from_text, String &to_text) const;
	bool translate(const String& from_text, Ident item, String &to_text) const;
	bool translate(const String& from_text, Ident section, Ident item, String &to_text) const;
	bool translate(const String& from_text, Ident section, int nr, String &to_text) const;

	struct Entry;
	struct Variant;
	struct Example;

	class VariantIterator;
	class EntryIterator
	{
		friend class VariantIterator;
	public:
		EntryIterator(const Dictionary& dictionary);
		bool more();
		void next();
		const String fromText();
		bool hasVariants();
		bool hasMajorVariant();
		const String& majorToText();
	private:
		Entry* _entry;
	};

	class ExampleIterator;
	class VariantIterator
	{
		friend class ExampleIterator;
	public:
		VariantIterator(const EntryIterator& entryIterator, bool include_major = false);
		bool more();
		void next();
		const String& toText();
	private:
		Variant* _major_variant;
		Variant* _variant;
	};

	class ExampleIterator
	{
	public:
		ExampleIterator(const VariantIterator& variantIterator);
		bool more();
		void next();
		const Ident section();
		const Ident item();
	private:
		Example* _example;
	};

private:
	String _from_lang;
	String _to_lang;
	Entry* _entries;
	Example* _examples;
	Example** _ref_example;
};


#endif // _INCLUDED_DICTIONARY_H
