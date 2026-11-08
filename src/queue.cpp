#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

template <typename T>
class MessageQueue {
public:
    MessageQueue() = default;

    void push (const T& msg) {
        {
            std::lock_guard lock (_mutex);
            _queue.push (msg);
        }
        _cond.notify_one();
    }

    T pop() {
        std::unique_lock lock (_mutex);
        _cond.wait (lock, [this] { return ! _queue.empty(); });
        auto msg = std::move (_queue.front());
        _queue.pop();
        return msg;
    }

    bool empty() const {
        std::lock_guard lock (_mutex);
        return _queue.empty();
    }

private:
    std::queue<T> _queue;
    mutable std::mutex _mutex;
    std::condition_variable _cond;
};

struct Message {
    int num { 0 };
};

std::unique_ptr<MessageQueue<Message>> _queue;

void process() {
    while (! _queue->empty()) {
        const auto msg = _queue->pop();
        std::cout << "msg num = " << msg.num << std::endl;
        std::this_thread::sleep_for (std::chrono::milliseconds { 10 });
    }
};

int main() {
    _queue = std::make_unique<MessageQueue<Message>>();

    for (int i = 1; i <= 100; ++i) {
        _queue->push ({ i });
    }

    std::thread t { process };
    t.join();

    _queue.reset();
    return 0;
}
