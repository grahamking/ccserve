all: ccgen.cpp ccserve.cpp
	clang++ -pthread ccgen.cpp ccserve.cpp -Ofast -Wall -std=c++14 -o ccserve

# comment in the main func in ccgen first
ccgen: ccgen.cpp
	clang++ -pthread ccgen.cpp -Ofast -Wall -std=c++14 -o ccgen

bench:
	./ccserve &
	ab -n 100000 -c 100 localhost:1212/ | grep 'Requests per second'

stop:
	pkill ccserve
