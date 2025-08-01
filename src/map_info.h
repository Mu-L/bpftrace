#pragma once

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "types.h"

namespace libbpf {
#include "libbpf/bpf.h"
} // namespace libbpf

namespace bpftrace {

struct HistogramArgs {
  long bits = -1;
  bool scalar = true;
  bool operator==(const HistogramArgs &other)
  {
    return bits == other.bits && scalar == other.scalar;
  }
  bool operator!=(const HistogramArgs &other)
  {
    return !(*this == other);
  }

private:
  friend class cereal::access;
  template <typename Archive>
  void serialize(Archive &archive)
  {
    archive(bits, scalar);
  }
};

struct LinearHistogramArgs {
  long min = -1;
  long max = -1;
  long step = -1;
  bool scalar = true;

  bool operator==(const LinearHistogramArgs &other)
  {
    return min == other.min && max == other.max && step == other.step &&
           scalar == other.scalar;
  }
  bool operator!=(const LinearHistogramArgs &other)
  {
    return !(*this == other);
  }

private:
  friend class cereal::access;
  template <typename Archive>
  void serialize(Archive &archive)
  {
    archive(min, max, step, scalar);
  }
};

struct TSeriesArgs {
  long interval_ns = -1;
  long num_intervals = -1;
  SizedType value_type;
  TSeriesAggFunc agg;

  bool operator==(const TSeriesArgs &other)
  {
    return interval_ns == other.interval_ns &&
           num_intervals == other.num_intervals &&
           value_type == other.value_type && agg == other.agg;
  }
  bool operator!=(const TSeriesArgs &other)
  {
    return !(*this == other);
  }

private:
  friend class cereal::access;
  template <typename Archive>
  void serialize(Archive &archive)
  {
    archive(interval_ns, num_intervals, value_type, agg);
  }
};

struct MapInfo {
  SizedType key_type;
  SizedType value_type;
  std::variant<std::monostate, HistogramArgs, LinearHistogramArgs, TSeriesArgs>
      detail;
  int id = -1;
  int max_entries = -1;
  libbpf::bpf_map_type bpf_type = libbpf::BPF_MAP_TYPE_HASH;
  bool is_scalar = false;

private:
  friend class cereal::access;
  template <typename Archive>
  void serialize(Archive &archive)
  {
    archive(key_type, value_type, detail, id, max_entries, bpf_type, is_scalar);
  }
};

} // namespace bpftrace
