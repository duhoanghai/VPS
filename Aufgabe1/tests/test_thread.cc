#include "thread/Thread.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>

// Die verzögerte Aufräumroutine macht sichtbar, ob cancel() tatsächlich wartet.
class TestThread : public Thread {
public:
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    static void cleanup(void* argument) {
        TestThread* self = static_cast<TestThread*>(argument);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        self->finished.store(true);
    }
    void run() override {
        pthread_cleanup_push(cleanup, this);
        started.store(true);
        for (;;) {
            testCancel();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        pthread_cleanup_pop(1);
    }
};

int main() {
    TestThread worker;
    worker.join(); // Vor create() muss join() wirkungslos sein.
    worker.create();
    while (!worker.started.load()) std::this_thread::yield();
    worker.cancel();
    const bool waited = worker.finished.load();
    if (!waited) {
        // Auch im Fehlerfall das Testobjekt bis zum Thread-Ende am Leben halten.
        while (!worker.finished.load()) std::this_thread::yield();
        std::cerr << "FAIL: cancel() hat nicht auf das Thread-Ende gewartet\n";
        return 1;
    }
    worker.cancel(); // Wiederholtes Beenden darf keinen zweiten Join auslösen.
    std::cout << "PASS: cancel() wartet auf die Aufräumroutine des Threads\n";
}
