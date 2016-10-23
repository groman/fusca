#pragma once

#include <type_traits>
#include <chrono>
#include <thread>
namespace fusca
{
  typedef std::nano ns_t;
  typedef std::chrono::duration<double, ns_t> chrono_dur_t;

  using std::chrono::duration_cast;
  using std::chrono::time_point_cast;

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

      slow_dur_t m_soverhead;
      fast_dur_t m_foverhead;

      template<typename to_t, typename from_t>
      inline auto cast(const from_t & from) const
        -> typename std::enable_if<std::is_arithmetic<from_t>::value && std::is_arithmetic<to_t>::value, to_t>::type
      {
        return static_cast<to_t>(from);
      }

      template<typename to_t, typename from_t>
      inline auto cast(const from_t & from) const
        -> decltype(duration_cast<chrono_dur_t>(std::declval<from_t>()).count())
      {
        return static_cast<to_t>(duration_cast<chrono_dur_t>(from).count());
      }

      template<typename to_t, typename from_t>
      inline auto cast(const from_t & from) const
        -> decltype(static_cast<to_t>(duration_cast<chrono_dur_t>(std::declval<from_t>().time_since_epoch()).count()))
      {
        return static_cast<to_t>(duration_cast<chrono_dur_t>(from.time_since_epoch()).count());
      }

      template<typename to_t, typename from_t>
      inline auto cast(const from_t & from) const
        -> typename std::enable_if<!std::is_arithmetic<to_t>::value, decltype(to_t(duration_cast<typename to_t::duration>(chrono_dur_t(std::declval<from_t>()))))>::type
      {
        return to_t(duration_cast<typename to_t::duration>(chrono_dur_t(from)));
      }

      inline slow_val_t estimate() const
      {
        fast_val_t fnow = m_fc();

        return cast<slow_val_t>(cast<double>(m_sstart) + cast<double>(m_speriod) * (cast<double>(fnow) - cast<double>(m_fstart)) / cast<double>(m_fperiod));
      }
    public:
      fusca(slow_clock_t sc, fast_clock_t fc) : m_sc(sc), m_fc(fc)
      {

        slow_val_t st1 = m_sc();
        slow_val_t st2 = m_sc();
        fast_val_t ft1 = m_fc();
        fast_val_t ft2 = m_fc();

        m_foverhead = ft2 - ft1;
        m_soverhead = st2 - st1;

        st1 = m_sc();
        ft1 = m_fc();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        st2 = m_sc();
        ft2 = m_fc();

        m_fperiod = ft2 - ft1 - (m_foverhead);
        m_speriod = st2 - st1 - (m_soverhead);

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
