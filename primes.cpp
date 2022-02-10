#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <deque>
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

class SpinLock {
private:
  atomic_flag flag = ATOMIC_FLAG_INIT;

public:
  void acquire() {
    while (flag.test_and_set(memory_order_acquire))
      ;
  }
  void release() { flag.clear(memory_order_release); }
};

class PrimeOutputBuffer {
private:
  SpinLock lock;
  deque<unsigned long long> theQueue;

public:
  void push(unsigned long long value) {
    lock.acquire();
    theQueue.push_back(value);
    lock.release();
  }
  unsigned long long pop() {
    lock.acquire();
    unsigned long long result = theQueue.front();
    theQueue.pop_front();
    lock.release();
    return result;
  }
  int size() {
    lock.acquire();
    int result = theQueue.size();
    lock.release();
    return result;
  }
  unsigned long long peekBack() {
    lock.acquire();
    unsigned long long result = theQueue.back();
    lock.release();
    return result;
  }
  void sort() {
    lock.acquire();
    std::sort(theQueue.begin(), theQueue.end());
    lock.release();
  }
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
  PrimeOutputBuffer outputBuffer;
  cout << "Starting calculation..." << endl;
  outputBuffer.push(2); // Only prime not found by the program
  int targetNumberOfPrimes = atoi(argv[1]);
  int numPrimesPerWorker = (targetNumberOfPrimes + numWorkers - 1) / numWorkers;
  int numPrimes =
      numPrimesPerWorker * numWorkers + 1; // Don't forget we already added 2
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
    cout << outputBuffer.size() << ": " << outputBuffer.peekBack() << "\t\r";
    cout.flush();
    this_thread::sleep_for(chrono::milliseconds(500));
  }
  cout << "Found " << outputBuffer.size() << " primes" << endl;
  cout << "Sorting..." << endl;
  outputBuffer.sort();
  cout << "Last prime: " << outputBuffer.peekBack() << endl;
  cout << "Building textual representation..." << endl;
  string outputString = "";
  for (int i = 0; outputBuffer.size() > 0; i++) {
    outputString += to_string(outputBuffer.pop());
    outputString += '\n';
  }
  cout << "Writing to 'primes.txt'..." << endl;
  ofstream output("primes.txt");
  output << outputString;
  output.close();
  cout << "Done!" << endl;
  return 0;
}
