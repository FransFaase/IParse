IParse
------
IParse is an interpreting parser, meaning that it reads
a grammar and interpret this to parse another file. It
also uses a parsing driven scanner approach, where the
parser calls the scanner to see if a certain type of
scanner symbol is found on the input. A number of scanners
are provided, including a raw scanner, which gives access
to the raw input. The grammar allows definition of character
ranges and white space terminals, thus allowing a scanner
to be specified in the grammar.

Several parsing algorithms are provided and can be selected
from the command line. The default parsing algorithm is a
back-tracking parser, which uses memorization, resulting in
a good overall performance.

IParse has a proven track record in many application (including
a commercial application), but it should be noted that some parts
are still under construction, such as the LL1HeapColourParser.
The ParParser, an experimental parallel parser, has poor
performance.

Also RcTransl, an tool for language translation between
Windows resource files, is still under development. 

http://www.iwriteiam.nl/MM.html

Installing
----------

On Linux just compile all_IParse.cpp, which includes all
source files, with g++ into IParse executable.

On Windows use A Visual C++ 2008 Express Edition with
IParse.sln file.