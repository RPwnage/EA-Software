#include <windows.h>

template <class T>
class AutoLocalHeap
{
public:
    AutoLocalHeap() : _ptr(0) { }
    AutoLocalHeap(T p) : _ptr(p) { }
    ~AutoLocalHeap()
    {
        if (_ptr)
        {
            LocalFree(_ptr);
        }
    }

    T* operator&()
    {
        return &_ptr;
    }

    operator T()
    {
        return _ptr;
    }

    T data()
    {
        return _ptr;
    }

    AutoLocalHeap& operator=(const T rhs)
    {
        _ptr = rhs;
        return *this;
    }

    T release()
    {
        T tmp = _ptr;
        _ptr = 0;
        return tmp;
    }
private:
    T _ptr;
};

template <class T>
class AutoBuffer
{
public:
    AutoBuffer() : _ptr(NULL) { }
    AutoBuffer(size_t size) : _ptr(NULL)
    {
        _ptr = (T*) new char[size];
        if (_ptr)
        {
            ZeroMemory(_ptr, size);
        }
    }
    ~AutoBuffer()
    {
        if (_ptr)
        {
            delete [] _ptr;
        }
    }

    operator T*()
    {
        return _ptr;
    }

    T* operator->()
    {
        return _ptr;
    }

    T* data()
    {
        return _ptr;
    }

    AutoBuffer& operator=(const T rhs)
    {
        _ptr = rhs;
        return *this;
    }

    T release()
    {
        T tmp = _ptr;
        _ptr = 0;
        return tmp;
    }
private:
    T* _ptr;
};

class Auto_MUTEX
{
public:
    Auto_MUTEX(LPCWSTR mutexName) : _handle(0), _acquired(false)
    {
        _handle = CreateMutex(NULL, FALSE, mutexName);
    }
    ~Auto_MUTEX() 
    {
        if (_acquired)
        {
            ReleaseMutex(_handle);
            _acquired = false;
        }
        if (_handle)
        {
            CloseHandle(_handle);
        }
    }

    operator HANDLE()
    {
        return _handle;
    }

    bool valid()
    {
        return _handle != NULL;
    }

    void acquire(DWORD timeout)
    {
        if (_acquired)
            return;

        if (!valid())
            return;

        WaitForSingleObject(_handle, timeout);

        _acquired = true;
    }

    void release()
    {
        if (!valid())
            return;

        if (_acquired)
        {
            ReleaseMutex(_handle);
            _acquired = false;
        }
    }

private:
    HANDLE _handle;
    bool _acquired;
};

class Auto_HANDLE
{
public:
    Auto_HANDLE() : _handle(0)
    {
    }
    Auto_HANDLE(HANDLE handle) : _handle(handle) 
    { 
    }
    ~Auto_HANDLE() 
    {
        if (_handle)
        {
            CloseHandle(_handle);
        }
    }

    operator HANDLE()
    {
        return _handle;
    }

    Auto_HANDLE& operator=(const HANDLE rhs)
    {
        _handle = rhs;
        return *this;
    }

    HANDLE* operator&()
    {
        return &_handle;
    }

    HANDLE release()
    {
        HANDLE tmp = _handle;
        _handle = 0;
        return tmp;
    }

    void close()
    {
        if (_handle)
        {
            CloseHandle(_handle);
            _handle = 0;
        }
    }

private:
    HANDLE _handle;
};

class Auto_FileLock
{
public:
    Auto_FileLock(LPCWSTR filename)
    {
        // Open file handle with read-only sharing (prevents other processes from modifying the file
        HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            _hdl = fileHandle;
        }
    }
    
    void release()
    {
        _hdl.close();
    }

    bool acquired()
    {
        return ((HANDLE)_hdl != NULL);
    }

private:
    Auto_HANDLE _hdl;
};