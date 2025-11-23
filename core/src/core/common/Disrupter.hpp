#include <atomic>
#include <iostream>
#include <memory> // For std::unique_ptr
#include <thread> // For std::thread

//  SequenceBarrier implementation (Single)

class SingleSequenceBarrier
{
public:
    SingleSequenceBarrier(){

    }

    void signal(size_t sequence)
    {
        // No explicit action needed for single barrier type
    }

    void wait(size_t sequence)
    {
        while (sequence_.load() < sequence)
        {
            // Spin-wait or alternative strategy
            // std::this_thread::yield(); // Optional spin-wait
        }
    }
    size_t load() const{
        return sequence_.load();
    }

private:
    std::atomic<size_t> sequence_;
};

enum class SequenceBarrierType { SINGLE, BATCH }; // Options for barrier type
template <typename T>
class Disruptor {
public:
    // Constructor (specifies buffer size and sequence barrier type)
    Disruptor(size_t buffer_size)
        : buffer_(new T[buffer_size]), capacity_(buffer_size), sequence_(0){}

    // Destructor (frees memory)
    ~Disruptor() {
        delete[] buffer_;
    }

    // Push an element to the ring buffer (thread-safe)
    bool push(const T& element) {
        if (is_full()) {
            std::cerr << "Disruptor overflow!" << std::endl;
            return false;
        }

        size_t expected = sequence_.load();
        while (!sequence_.compare_exchange_weak(expected, expected + 1, std::memory_order_acq_rel)); // Acquire current, release updated

        buffer_[expected % capacity_] = element;
        barrier_.signal(expected);
        return true;
    }

    // Pop an element from the ring buffer (thread-safe)
    T pop() {
        while (is_empty()) {
            // Handle empty buffer (spin-wait or alternative strategy)
            // std::this_thread::yield(); // Optional spin-wait
        }

        size_t next_sequence = sequence_.load();
        barrier_.wait(next_sequence);
        T element = buffer_[next_sequence % capacity_];
        sequence_.store(next_sequence + 1, std::memory_order_release); // Release updated sequence
        return element;
    }

private:
    std::unique_ptr<T[]> buffer_;
    size_t capacity_;
    std::atomic<size_t> sequence_; // Sequence counter (cursor for producers)
    SingleSequenceBarrier barrier_; // Handles synchronization and memory ordering

    bool is_full() const {
        return (sequence_.load() - 1) % capacity_ == sequence_.load();
    }

    bool is_empty() const {
        return sequence_.load() == barrier_.load();
    }
};