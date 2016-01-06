all: bin/sss-mon

bin/sss-mon: src/sss-mon.cpp
	$(CXX) -std=c++11 -Wall -Wextra -pedantic -O3 -DNDEBUG -o $@ $<

debug: src/sss-mon.cpp
	$(CXX) -std=c++11 -Wall -Wextra -pedantic -O0 -g3 -o bin/sss-mon $<

clean:
	rm -f bin/sss-mon

.PHONY: clean debug
