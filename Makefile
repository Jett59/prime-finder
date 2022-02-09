CXXFLAGS+=-Ofast -pthread -std=c++14
CXXFLAGS+=-fno-math-errno

a.out: primes.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) primes.cpp -o a.out

clean:
	rm a.out
