

template<class T>
void Deleter(T *x)
{
    delete x;
}


template<class T, void (*F)(T *)>
struct FuncDeleter
{
    void operator()(T *x) const
    {
        F(x);
    }
};
