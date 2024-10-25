#ifndef UTILS_BITCOUNT_h
#define UTILS_BITCOUNT_h

/*
 * Counts bits in a value.
 * */
template <size_t X>
struct Log2 {
    constexpr static size_t value = Log2< X/2 >::value + 1;
};

template <> struct Log2<1> { constexpr static size_t value = 0; };

#endif
