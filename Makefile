all: ccgen.cpp ccserve.cpp
	clang++ ccgen.cpp ccserve.cpp -Ofast -Wall -std=c++14 -o ccserve

# comment in the main func in ccgen first
ccgen: ccgen.cpp
	clang++ ccgen.cpp -Ofast -Wall -std=c++14 -o ccgen
