#include <iostream>

template <typename T>
class RingBuffer
{
public:
    // Constructor (specifies capacity)
    RingBuffer(unsigned int capacity)
    {
        capacity_ = 128;//nextPowerOfTwo(capacity);
        data_ = new T[capacity_];
        head_ = 0;
        tail_ = 0;
        cacapacityMod = capacity_ - 1;
    }

    // Destructor (frees memory)
    ~RingBuffer()
    {
        delete[] data_;
    }

    // Check if the buffer is empty
    bool isEmpty() const
    {
        return head_ == tail_;
    }

    // Check if the buffer is full
    bool isFull() const
    {
        return ((tail_ + 1) & cacapacityMod) == head_;
    }

    // Push an element to the back of the buffer
    bool push(const T &element)
    {
        if (isFull())
        {
            // Handle overflow (optional: throw exception, overwrite oldest element)
            std::cerr << "Ring buffer overflow!" << std::endl;
            return false;
        }

        data_[tail_] = element;
        tail_ = (tail_ + 1) & cacapacityMod;
        return true;
    }

    // Push elements from a pointer with specified length
    bool push(const T *elements, unsigned int length)
    {
        unsigned space_available = availableSize();
        if (space_available < length)
        {
            std::cerr << "Ring buffer overflow!" << std::endl;
            return false;
        }

        unsigned int elements_to_push = std::min(length, space_available);

        // Copy elements from source to the end of the buffer (wrapping around)
        unsigned int copied = 0;
        while (copied < elements_to_push)
        {
            unsigned int remaining = capacity_ - tail_;
            unsigned int to_copy = std::min(elements_to_push - copied, remaining);
            std::memcpy(data_ + tail_, elements + copied, to_copy * sizeof(T));
            tail_ = (tail_ + to_copy) & cacapacityMod;
            copied += to_copy;
        }
        return true;
    }
    T *writePtr()
    {
        return data_ + tail_ & cacapacityMod;
    }
    T *readPtr()
    {
        return data_ + head_ & cacapacityMod;
    }
    unsigned int headToEndSize()
    {
        return capacity_ - head_;
    }
    // to tail size
    unsigned int tailToEndSize()
    {
        return capacity_ - tail_;
    }

    // Pop an element from the front of the buffer
    T pop()
    {
        if (isEmpty())
        {
            // Handle underflow (optional: throw exception, return default value)
            std::cerr << "Ring buffer underflow!" << std::endl;
            return nullptr; // Return default value for type T
        }

        T element = data_[head_];
        head_ = (head_ + 1) & cacapacityMod;
        return element;
    }

    // Pop elements into a provided buffer
    unsigned int pop(T *buff, unsigned int len)
    {
        unsigned int effectSize = size();
        if (effectSize < len)
        {
            std::cerr << "Ring buffer underflow!" << std::endl;
            return 0; // Return 0 elements popped if buffer is empty
        }

        // Calculate the number of elements to pop (min of requested length and available elements)
        unsigned int elements_to_pop = std::min(len, effectSize);

        // Loop to pop elements considering wrapping
        unsigned int copied = 0;
        while (copied < elements_to_pop)
        {
            unsigned int remaining = capacity_ - head_;
            unsigned int to_copy = std::min(elements_to_pop - copied, remaining);

            // Directly copy elements to user-provided buffer (considering wrapping)
            std::memcpy(buff + copied, data_ + head_, to_copy * sizeof(T));

            head_ = (head_ + to_copy) & cacapacityMod;
            copied += to_copy;
        }

        return elements_to_pop; // Return the actual number of elements popped
    }

    // Get the size of the buffer (number of elements)
    unsigned int size() const
    {
        return (tail_ - head_ + capacity_) & cacapacityMod;
    }

    unsigned int availableSize() const
    {
        return capacity_ - size() - 1;
    }

    // Peek at elements (return pointer to the first element within specified length)
    bool peek(T *out, int length)
    {
        int validLen = size();
        if (validLen < length)
        {
            std::cerr << "Ring buffer is empty!" << std::endl;
            return false; // Return nullptr if buffer is empty
        }

        // Calculate the effective length based on buffer size and requested length

        int effective_length = std::min(length, validLen);
        unsigned int copied = 0;
        unsigned int head = head_;
        while (copied < effective_length)
        {
            unsigned int remaining = capacity_ - head;
            unsigned int to_copy = std::min(effective_length - copied, remaining);
            // Directly copy elements to user-provided buffer (considering wrapping)
            std::memcpy(out + copied, data_ + head, to_copy * sizeof(T));

            head = (head + to_copy) & cacapacityMod;
            copied += to_copy;
        }
        return true;
    }

private:
    unsigned int capacity_;     // Maximum capacity of the buffer must 2^n
    unsigned int cacapacityMod; // capacity_ -1
    T *data_;                   // Array to store elements
    unsigned int head_;         // Index of the first element
    unsigned int tail_;         // Index of the element after the last element
};
