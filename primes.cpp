#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

template <typename IntType>
static constexpr inline bool isDivisible(IntType dividend, IntType divisor) {
  return dividend % divisor == 0;
}
template <typename IntType>
static constexpr inline bool internalNotIsPrime(IntType n, IntType testNumber,
                                                IntType top) {
  return testNumber <= top ? isDivisible(n, testNumber)
                                 ? true
                                 : internalNotIsPrime(n, testNumber + 1, top)
                           : false;
}
template <typename IntType> static constexpr inline bool isPrime(IntType n) {
  return n == 2 ? true
                : !internalNotIsPrime<IntType>(
                      n, 2, static_cast<IntType>(sqrt(static_cast<double>(n))));
}

using namespace std;

class PrimeOutputBuffer {
private:
  uint64_t *data;
  atomic_int64_t numElements;
  uint64_t maxElementCount;

public:
  typedef uint64_t *iterator;
  PrimeOutputBuffer(uint64_t maxElements)
      : data(new uint64_t[maxElements]), maxElementCount(maxElements) {
    numElements = 0;
  }

  void push(uint64_t value) { data[atomic_fetch_add(&numElements, 1)] = value; }
  iterator begin() { return data; }
  iterator end() { return data + maxElementCount; }

  void sort() { std::sort(begin(), end()); }

  uint64_t size() { return numElements; }
};

struct WorkerContext {
  int start;
  int increment;
  int numPrimes;
  PrimeOutputBuffer *outputBuffer;
};

int primeFinderWorker(WorkerContext *context) {
  int increment = context->increment;
  int numPrimes = context->numPrimes;
  PrimeOutputBuffer *outputBuffer = context->outputBuffer;
  int foundPrimes = 0;
  for (unsigned long long i = context->start; foundPrimes < numPrimes;
       i += increment) {
    if (isPrime(i)) {
      outputBuffer->push(i);
      foundPrimes++;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  auto numWorkers = thread::hardware_concurrency();
  WorkerContext contexts[numWorkers];
  int targetNumberOfPrimes = atoi(argv[1]);
  int numPrimesPerWorker = (targetNumberOfPrimes + numWorkers - 1) / numWorkers;
  int numPrimes =
      numPrimesPerWorker * numWorkers + 1; // Don't forget we already added 2
  PrimeOutputBuffer outputBuffer(numPrimes);
  cout << "Starting calculation..." << endl;
  outputBuffer.push(2); // Only prime not found by the program
  for (int i = 0; i < numWorkers; i++) {
    WorkerContext &context = contexts[i];
    context.start = i * 2 + 3; // Only odd numbers can be prime (except 2)
    context.increment = numWorkers * 2;
    context.numPrimes = numPrimesPerWorker;
    context.outputBuffer = &outputBuffer;
    thread workerThread(primeFinderWorker, &contexts[i]);
    workerThread.detach();
  }
  while (outputBuffer.size() < numPrimes) {
    cout << outputBuffer.size() << "\t\r";
    cout.flush();
    this_thread::sleep_for(chrono::milliseconds(500));
  }
  cout << "Sorting..." << endl;
  outputBuffer.sort();
  cout << "Building textual representation..." << endl;
  string outputString = "";
  for (PrimeOutputBuffer::iterator iterator = outputBuffer.begin();
       iterator != outputBuffer.end(); iterator++) {
    outputString += to_string(*iterator);
    outputString += '\n';
  }
  cout << "Writing to 'primes.txt'..." << endl;
  ofstream output("primes.txt");
  output << outputString;
  output.close();
  cout << "Done!" << endl;
  return 0;
}
