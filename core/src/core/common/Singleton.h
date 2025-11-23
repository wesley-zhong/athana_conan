
#ifndef SRC_UTILS_SINGLETON_H_
#define SRC_UTILS_SINGLETON_H_

template <class T>
class Singleton
{
public:
    static T *Instance()
    {
        return _instance;
    }

protected:
    static T* _instance;
    // Disable compiler generated code to prevent default construction/destruction
    // and copy/assignment semantics.
    Singleton() {}
    ~Singleton() {}
    Singleton(Singleton const &) {}
    Singleton &operator=(Singleton const &) {}
};
template <class T> T* Singleton<T> ::_instance = new T();
#endif // SRC_UTILS_SINGLETON_H_

