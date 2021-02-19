#ifndef BOMBA_TASKER_H
#define BOMBA_TASKER_H

#include <functional>
#include <vector>

class Tasker {
public:
    explicit Tasker(int core);

    void once(const char *name, std::function<void(void)> op) const;

    void loop(const char *name, std::function<void(void)> op) const;

    void loopEvery(const char *name, int millis, std::function<void(void)> op) const;

    static void sleep(int millis);
    static void yield();

private:
    int core;
};

extern Tasker DefaultTasker;
extern Tasker NetworkTasker;

#endif //BOMBA_TASKER_H
