#pragma once

#include <type_traits>

namespace fusca
{
  template<typename slow_clock_t, typename fast_clock_t>
  class fusca
  {
      typedef typename std::result_of<slow_clock_t()>::type slow_val_t;
      typedef typename std::result_of<fast_clock_t()>::type fast_val_t;
      typedef decltype(std::declval<slow_clock_t>()() - std::declval<slow_clock_t>()()) slow_dur_t;
      typedef decltype(std::declval<fast_clock_t>()() - std::declval<fast_clock_t>()()) fast_dur_t;

      slow_clock_t m_sc;
      fast_clock_t m_fc;

      slow_val_t m_sstart;
      fast_val_t m_fstart;

      slow_dur_t m_speriod;
      fast_dur_t m_fperiod;

      inline slow_val_t estimate() const
      {
        fast_val_t fnow = m_fc();

        return m_sstart + m_speriod * (fnow - m_fstart) / m_fperiod;
      }
    public:
      fusca(slow_clock_t sc, fast_clock_t fc) : m_sc(sc), m_fc(fc)
      {
        slow_val_t st1 = m_sc();
        fast_val_t ft1 = m_fc();
        slow_val_t st2 = m_sc();
        fast_val_t ft2 = m_fc();

        m_fperiod = ft2 - ft1;
        m_speriod = st2 - st1;

        m_sstart = st2;
        m_fstart = ft2;
      }

      inline slow_val_t operator()()
      {
        return estimate();
      }
  };

  template<typename slow_clock_t, typename fast_clock_t>
  fusca<slow_clock_t, fast_clock_t> make(slow_clock_t sc, fast_clock_t fc)
  {
    return fusca<slow_clock_t, fast_clock_t>(sc,fc);
  }

}
