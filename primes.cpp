#include <atomic>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <deque>

template<typename IntType>
constexpr bool isDivisible(IntType dividend, IntType divisor) {
    return dividend % divisor == 0;
}
template<typename IntType>
constexpr bool internalNotIsPrime(IntType n, IntType testNumber, IntType top) {
  return testNumber <= top ? isDivisible(n, testNumber)
                                 ? true
                                 : internalNotIsPrime(n, testNumber + 1, top) : false;
}
template<typename IntType>
constexpr bool isPrime(IntType n) {
    return n == 2 ? true : !internalNotIsPrime<IntType>(n, 2, static_cast<IntType>(sqrt(static_cast<double>(n))));
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

class primeOutputBuffer {
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
           void sort () {
             lock.acquire();
             std::sort(theQueue.begin(), theQueue.end());
             lock.release();
           }
};

struct WorkerContext {
    int start;
  int increment;
  int foundPrimes = 0;
  primeOutputBuffer* outputBuffer;
  bool running = false;
  bool shouldBeRunning = true;
};

int primeFinderWorker(volatile WorkerContext* context) {
  context->running = true;
  for (unsigned long long i = context->start; context->running; i += context->increment) {
    if (isPrime(i)) {
      context->outputBuffer->push(i);
      context->foundPrimes++;
    }
    if (!context->shouldBeRunning) {
      context->running = false;
    }
}
return 0;
}

int main (int argc, char** argv) {
  auto numWorkers = thread::hardware_concurrency();
  volatile WorkerContext contexts[numWorkers];
  primeOutputBuffer outputBuffer;
  cout << "Starting calculation..." << endl;
  outputBuffer.push(2); // Only prime not found by the program
  for (int i = 0; i < numWorkers; i++) {
    volatile WorkerContext& context = contexts[i];
    context.start = i * 2 + 3; // Only odd numbers can be prime (except 2)
    context.increment = numWorkers * 2;
    context.outputBuffer = &outputBuffer;
    thread workerThread(primeFinderWorker, &contexts[i]);
    workerThread.detach();
  }
  int targetNumberOfPrimes = atoi(argv[1]);
  while (outputBuffer.size() < targetNumberOfPrimes) {
    cout << "\r" << outputBuffer.size() << ": " << outputBuffer.peekBack() << "\t";
    this_thread::sleep_for(chrono::milliseconds(100));
  }
  cout << "Stopping worker threads" << endl;
  for (int i = 0; i < numWorkers; i ++) {
    contexts[i].shouldBeRunning = false;
    cout << "Stopping worker thread " << i << ": ";
    while (contexts[i].running)
    ;
    cout << "Done (it found " << contexts[i].foundPrimes << " primes)" << endl;
  }
    cout << "Found " << outputBuffer.size() << " primes" << endl;
    cout << "Sorting..." << endl;
    outputBuffer.sort();
    cout << "Last prime: " << outputBuffer.peekBack() << endl;
    cout << "Building textual representation..." << endl;
    string outputString = "";
    for (int i = 0; outputBuffer.size() > 0; i ++) {
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
