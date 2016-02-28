
#ifndef ETREE_UTIL_H
#define ETREE_UTIL_H


namespace etree {


template<class T>
void Deleter(T *x)
{
    delete x;
}


template<class T>
void dumpVector(const char *s, T v)
{
    int i = 0;
    std::cout << s << ":" << std::endl;
    typename T::const_iterator it = v.begin();
    while(it != v.end()) {
        std::cout << "vector " << i++ << ": " << *it << std::endl;
    }
    std::cout << std::endl;
}


} // namespace

#endif // ETREE_UTIL_H
