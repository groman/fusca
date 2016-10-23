#pragma once

#include <type_traits>


template<typename slow_clock_t, typename fast_clock_t>
class fusca
{
    slow_clock_t m_sc;
    fast_clock_t m_fc;
  public:
    fusca(slow_clock_t sc, fast_clock_t fc) : m_sc(sc), m_fc(fc)
    {
    }

    inline auto operator()() -> typename std::result_of<slow_clock_t()>::type
    {
      return m_fc();
    }
};

inline template<typename slow_clock_t, typename fast_clock_t>
fusca<slow_clock_t, fast_clock_t> make_fusca(slow_clock_t sc, fast_clock_t fc)
{
  return fusca<slow_clock_t, fast_clock_t>(sc,fc);
}
