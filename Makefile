all:
	clang++ ccgen.cpp ccserve.cpp -Ofast -Wall -std=c++14 -o ccserve
