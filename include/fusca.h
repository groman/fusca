#pragma once

#include <type_traits>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

namespace fusca
{
  typedef std::nano ns_t;
  typedef std::chrono::duration<double, ns_t> chrono_dur_t;

  using std::chrono::duration_cast;
  using std::chrono::time_point_cast;

  template<typename slow_clock_t, typename fast_clock_t, const uint64_t sync_interval = 100000>
  class fusca
  {
      typedef typename std::result_of<slow_clock_t()>::type slow_val_t;
      typedef typename std::result_of<fast_clock_t()>::type fast_val_t;
      typedef decltype(std::declval<slow_clock_t>()() - std::declval<slow_clock_t>()()) slow_dur_t;
      typedef decltype(std::declval<fast_clock_t>()() - std::declval<fast_clock_t>()()) fast_dur_t;
      typedef double compute_t;

      slow_clock_t m_sc;
      fast_clock_t m_fc;

      slow_val_t m_sstart;
      fast_val_t m_fstart;

      slow_dur_t m_speriod;
      fast_dur_t m_fperiod;

      slow_dur_t m_soverhead;
      fast_dur_t m_foverhead;

      std::atomic<uint64_t> m_seq_update_start;
      std::atomic<uint64_t> m_seq_update_end;

      std::atomic<compute_t> m_c_sstart;
      std::atomic<compute_t> m_c_fstart;

      std::atomic<compute_t> m_c_speriod;
      std::atomic<compute_t> m_c_fperiod;

      std::atomic<compute_t> m_c_speriod_recip;
      std::atomic<compute_t> m_c_fperiod_recip;

      std::mutex m_sync_lock;

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
      static inline auto cast(const from_t & from)
        -> typename std::enable_if<!std::is_arithmetic<to_t>::value, decltype(to_t(duration_cast<typename to_t::duration>(chrono_dur_t(std::declval<from_t>()))))>::type
      {
        return to_t(duration_cast<typename to_t::duration>(chrono_dur_t(from)));
      }

      template<typename pred_t>
      inline auto lockless_crit(pred_t pred) const
        -> decltype(std::declval<pred_t>()())
      {
        uint64_t start_seq1, start_seq2;
        uint64_t end_seq1, end_seq2;
        decltype(std::declval<pred_t>()()) ret;
        // Variation on Bakery algorithm for redoing the predicate until the start and end sequence counters are the same and have not changed
        // during execution
        do
        {
          start_seq1 = m_seq_update_start;
          end_seq1 = m_seq_update_end;

          ret = pred();

          start_seq2 = m_seq_update_start;
          end_seq2 = m_seq_update_end;
        } while( (start_seq1 != start_seq2) || (end_seq1 != end_seq2) || (start_seq1 != end_seq1) );
        return ret;
      }

      inline slow_val_t estimate() const
      {
        return lockless_crit([&] {
          return cast<slow_val_t>(m_c_sstart + m_c_speriod * (cast<compute_t>(m_fc()) - m_c_fstart) * m_c_fperiod_recip);
        });
      }
      void init_clock()
      {
        slow_val_t st1 = m_sc();
        slow_val_t st2 = m_sc();
        fast_val_t ft1 = m_fc();
        fast_val_t ft2 = m_fc();

        m_foverhead = ft2 - ft1;
        m_soverhead = st2 - st1;

        st1 = m_sc();
        ft1 = m_fc();

        std::this_thread::sleep_for(chrono_dur_t(cast<compute_t>(m_foverhead) * sync_interval));

        st2 = m_sc();
        ft2 = m_fc();

        m_fperiod = ft2 - ft1 - (m_foverhead);
        m_speriod = st2 - st1 - (m_soverhead);

        m_sstart = st2;
        m_fstart = ft2;
      }
      void init_sync()
      {
        // Set update sequnce numbers
        m_seq_update_start = 0;
        m_seq_update_end = 0;

        // Pre-compute type casts
        m_c_sstart = cast<compute_t>(m_sstart);
        m_c_fstart = cast<compute_t>(m_fstart);

        m_c_speriod = cast<compute_t>(m_speriod);
        m_c_fperiod = cast<compute_t>(m_fperiod);

        m_c_speriod_recip = 1 / cast<compute_t>(m_speriod);
        m_c_fperiod_recip = 1 / cast<compute_t>(m_fperiod);
      }
      void init()
      {
        init_clock();
        init_sync();
      }
    public:
      fusca(slow_clock_t sc, fast_clock_t fc) : m_sc(sc), m_fc(fc)
      {
        init();
      }

      fusca(const fusca & other) : m_sc(other.m_sc), m_fc(other.m_fc)
      {
        m_foverhead = other.m_foverhead;
        m_soverhead = other.m_soverhead;

        m_fperiod = other.m_fperiod;
        m_speriod = other.m_speriod;

        m_sstart = other.m_sstart;
        m_fstart = other.m_fstart;

        // Initialize and recompute the rest
        init_sync();
      }

      // Synch function that computes error and adjusts values
      void sync()
      {
        // Lock to prevent multiple syncs from stomping on each other
        std::lock_guard<std::mutex> lock(m_sync_lock);

        // Grab all the clocks
        fast_val_t ft1 = m_fc();
        slow_val_t et1 = estimate();
        slow_val_t st1 = m_sc();

        // Estimate the error
        compute_t sdt = cast<compute_t>(st1) - m_c_sstart;
        compute_t edt = cast<compute_t>(et1) - m_c_sstart;

        compute_t period_adj = 1;
        if(edt > 0.0)
        {
          period_adj = sdt / edt;
        }

        // Indicate that we are about to write to the values
        m_seq_update_start++;

        // TODO: Support monotonic clocks by adding a non-restart option
        m_sstart = st1;
        m_fstart = ft1;

        m_c_sstart = cast<compute_t>(m_sstart);
        m_c_fstart = cast<compute_t>(m_fstart);

        m_c_speriod = m_c_speriod * period_adj;
        m_speriod = cast<slow_dur_t>(m_c_speriod);
        m_c_speriod_recip = 1 / cast<compute_t>(m_speriod);

        // Indicate that we are done
        m_seq_update_end++;
      }

      // Const version just does estimate() to try to be fast and fully lockless
      inline slow_val_t operator()() const
      {
        return estimate();
      }

      // This will resync if necessary
      inline slow_val_t operator()()
      {
        bool needSync = lockless_crit([&] {
          return (cast<compute_t>(m_fc() - m_fstart) / cast<compute_t>(m_foverhead) > sync_interval);
        });
        if(needSync)
        {
          sync();
        }
        return estimate();
      }

  };

  template<typename slow_clock_t, typename fast_clock_t>
  fusca<slow_clock_t, fast_clock_t> make(slow_clock_t sc, fast_clock_t fc)
  {
    return fusca<slow_clock_t, fast_clock_t>(sc,fc);
  }

}
