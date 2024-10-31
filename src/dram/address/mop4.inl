/*
 *  author: Suhas Vittal
 *  date:   30 October 2024
 * */

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

constexpr size_t B_LOW = 2;

constexpr size_t SC_OFF = B_LOW;
constexpr size_t CH_OFF = SC_OFF + B_SC;
constexpr size_t BG_OFF = CH_OFF + B_CH;
constexpr size_t BA_OFF = BG_OFF + B_BG;
constexpr size_t RA_OFF = BA_OFF + B_BA;
constexpr size_t HI_OFF = RA_OFF + B_RA;
constexpr size_t RO_OFF = HI_OFF + (B_CO-B_LOW);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline uint64_t CHANNEL(uint64_t x)     { return (x >> CH_OFF) & MASK(B_CH); }
inline uint64_t SUBCHANNEL(uint64_t x)  { return (x >> SC_OFF) & MASK(B_SC); }
inline uint64_t RANK(uint64_t x)        { return (x >> RA_OFF) & MASK(B_RA); }
inline uint64_t BANKGROUP(uint64_t x)   { return (x >> BG_OFF) & MASK(B_BG); }
inline uint64_t BANK(uint64_t x)        { return (x >> BA_OFF) & MASK(B_BA); }
inline uint64_t ROW(uint64_t x)         { return (x >> RO_OFF) & MASK(B_RO); }

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline uint64_t 
COLUMN(uint64_t x) { 
    uint64_t lwr = x & MASK(B_LOW),
             upp = (x >> HI_OFF) & MASK(B_CO-B_LOW);
    return lwr | (upp << B_LOW);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
