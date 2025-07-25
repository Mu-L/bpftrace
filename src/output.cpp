#include <algorithm>
#include <bpf/libbpf.h>
#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "ast/async_event_types.h"
#include "bpfmap.h"
#include "bpftrace.h"
#include "log.h"
#include "map_info.h"
#include "output.h"
#include "required_resources.h"
#include "types.h"
#include "util/stats.h"
#include "util/strings.h"
#include "util/time.h"
#include "util/tseries.h"

using namespace std::chrono_literals;

namespace bpftrace {

namespace {
bool is_quoted_type(const SizedType &ty)
{
  switch (ty.GetTy()) {
    case Type::buffer:
    case Type::cgroup_path_t:
    case Type::inet:
    case Type::kstack_t:
    case Type::ksym_t:
    case Type::none:
    case Type::strerror_t:
    case Type::string:
    case Type::timestamp:
    case Type::username:
    case Type::ustack_t:
    case Type::usym_t:
      return true;
    case Type::array:
    case Type::avg_t:
    case Type::count_t:
    case Type::hist_t:
    case Type::integer:
    case Type::lhist_t:
    case Type::tseries_t:
    case Type::mac_address:
    case Type::max_t:
    case Type::min_t:
    case Type::pointer:
    case Type::record:
    case Type::stack_mode:
    case Type::stats_t:
    case Type::sum_t:
    case Type::timestamp_mode:
    case Type::tuple:
    case Type::voidtype:
    case Type::boolean:
      return false;
  }
  return false;
}
} // namespace

std::ostream &operator<<(std::ostream &out, MessageType type)
{
  switch (type) {
    case MessageType::map:
      out << "map";
      break;
    case MessageType::value:
      out << "value";
      break;
    case MessageType::hist:
      out << "hist";
      break;
    case MessageType::tseries:
      out << "tseries";
      break;
    case MessageType::stats:
      out << "stats";
      break;
    case MessageType::printf:
      out << "printf";
      break;
    case MessageType::time:
      out << "time";
      break;
    case MessageType::cat:
      out << "cat";
      break;
    case MessageType::join:
      out << "join";
      break;
    case MessageType::syscall:
      out << "syscall";
      break;
    case MessageType::attached_probes:
      out << "attached_probes";
      break;
    case MessageType::lost_events:
      out << "lost_events";
      break;
    default:
      out << "?";
  }
  return out;
}

// Translate the index into the starting value for the corresponding interval.
// Each power of 2 is mapped into N = 2**k intervals, each of size
// S = 1 << ((index >> k) - 1), and starting at S * N.
// The last k bits of index indicate which interval we want.
//
// For example, if k = 2 and index = 0b11011 (27) we have:
// - N = 2**2 = 4;
// - interval size S is 1 << ((0b11011 >> 2) - 1) = 1 << (6 - 1) = 32
// - starting value is S * N = 128
// - the last 2 bits 11 indicate the third interval so the
//   starting value is 128 + 32*3 = 224

std::string TextOutput::hist_index_label(uint32_t index, uint32_t k)
{
  const uint32_t n = (1 << k);
  const uint32_t interval = index & (n - 1);
  assert(index >= n);
  uint32_t power = (index >> k) - 1;
  // Choose the suffix for the largest power of 2^10
  const uint32_t decade = power / 10;
  const char suffix = "\0KMGTPE"[decade];
  power -= 10 * decade;

  std::ostringstream label;
  label << (1 << power) * (n + interval);
  if (suffix)
    label << suffix;
  return label.str();
}

std::string TextOutput::lhist_index_label(int number, int step)
{
  constexpr int kilo = 1024;
  constexpr int mega = 1024 * 1024;

  if (step % kilo != 0)
    return std::to_string(number);

  std::ostringstream label;

  if (number == 0) {
    label << number;
  } else if (number % mega == 0) {
    label << number / mega << 'M';
  } else if (number % kilo == 0) {
    label << number / kilo << 'K';
  } else {
    label << number;
  }

  return label.str();
}

void Output::hist_prepare(const std::vector<uint64_t> &values,
                          int &min_index,
                          int &max_index,
                          int &max_value) const
{
  min_index = -1;
  max_index = -1;
  max_value = 0;

  for (size_t i = 0; i < values.size(); i++) {
    int v = values.at(i);
    if (v > 0) {
      if (min_index == -1)
        min_index = i;
      max_index = i;
    }
    max_value = std::max(v, max_value);
  }
}

void Output::lhist_prepare(const std::vector<uint64_t> &values,
                           int min,
                           int max,
                           int step,
                           int &max_index,
                           int &max_value,
                           int &buckets,
                           int &start_value,
                           int &end_value) const
{
  max_index = -1;
  max_value = 0;
  buckets = (max - min) / step; // excluding lt and gt buckets

  for (size_t i = 0; i < values.size(); i++) {
    int v = values.at(i);
    if (v != 0)
      max_index = i;
    max_value = std::max(v, max_value);
  }

  if (max_index == -1)
    return;

  // trim empty values
  start_value = -1;
  end_value = 0;

  for (unsigned int i = 0; i <= static_cast<unsigned int>(buckets) + 1; i++) {
    if (values.at(i) > 0) {
      if (start_value == -1) {
        start_value = i;
      }
      end_value = i;
    }
  }

  if (start_value == -1) {
    start_value = 0;
  }
}

std::string Output::get_helper_error_msg(libbpf::bpf_func_id func_id,
                                         int retcode) const
{
  std::string msg;
  if (func_id == libbpf::BPF_FUNC_map_update_elem && retcode == -E2BIG) {
    msg = "Map full; can't update element. Try increasing max_map_keys config "
          "or manually setting the max entries in a map declaration e.g. `let "
          "@a = hash(5000)`";
  } else if (func_id == libbpf::BPF_FUNC_map_delete_elem &&
             retcode == -ENOENT) {
    msg = "Can't delete map element because it does not exist.";
  }
  // bpftrace sets the return code to 0 for map_lookup_elem failures
  // which is why we're not also checking the retcode
  else if (func_id == libbpf::BPF_FUNC_map_lookup_elem) {
    msg = "Can't lookup map element because it does not exist.";
  } else {
    msg = strerror(-retcode);
  }
  return msg;
}

std::string Output::value_to_str(BPFtrace &bpftrace,
                                 const SizedType &type,
                                 const std::vector<uint8_t> &value,
                                 bool is_per_cpu,
                                 uint32_t div,
                                 bool is_map_key) const
{
  uint32_t nvalues = is_per_cpu ? bpftrace.ncpus_ : 1;
  switch (type.GetTy()) {
    case Type::kstack_t: {
      return bpftrace.get_stack(util::read_data<uint64_t>(value.data()),
                                util::read_data<uint64_t>(value.data() + 8),
                                -1,
                                -1,
                                false,
                                type.stack_type,
                                8);
    }
    case Type::ustack_t: {
      return bpftrace.get_stack(util::read_data<uint64_t>(value.data()),
                                util::read_data<uint64_t>(value.data() + 8),
                                util::read_data<int32_t>(value.data() + 16),
                                util::read_data<int32_t>(value.data() + 20),
                                true,
                                type.stack_type,
                                8);
    }
    case Type::ksym_t: {
      return bpftrace.resolve_ksym(util::read_data<uint64_t>(value.data()));
    }
    case Type::usym_t: {
      return bpftrace.resolve_usym(util::read_data<uint64_t>(value.data()),
                                   util::read_data<uint32_t>(value.data() + 8),
                                   util::read_data<uint32_t>(value.data() +
                                                             12));
    }
    case Type::inet: {
      return bpftrace.resolve_inet(util::read_data<uint64_t>(value.data()),
                                   static_cast<const uint8_t *>(value.data() +
                                                                8));
    }
    case Type::username: {
      return bpftrace.resolve_uid(util::read_data<uint64_t>(value.data()));
    }
    case Type::buffer: {
      return bpftrace.resolve_buf(
          reinterpret_cast<const AsyncEvent::Buf *>(value.data())->content,
          reinterpret_cast<const AsyncEvent::Buf *>(value.data())->length);
    }
    case Type::string: {
      const auto *p = reinterpret_cast<const char *>(value.data());
      return { p, strnlen(p, type.GetSize()) };
    }
    case Type::array: {
      size_t elem_size = type.GetElementTy()->GetSize();
      std::vector<std::string> elems;
      for (size_t i = 0; i < type.GetNumElements(); i++) {
        std::vector<uint8_t> elem_data(value.begin() + i * elem_size,
                                       value.begin() + (i + 1) * elem_size);
        elems.push_back(value_to_str(bpftrace,
                                     *type.GetElementTy(),
                                     elem_data,
                                     is_per_cpu,
                                     div,
                                     is_map_key));
      }
      return array_to_str(elems);
    }
    case Type::record: {
      std::vector<std::string> elems;
      for (auto &field : type.GetFields()) {
        std::vector<uint8_t> elem_data(value.begin() + field.offset,
                                       value.begin() + field.offset +
                                           field.type.GetSize());
        elems.push_back(field_to_str(
            field.name,
            value_to_str(
                bpftrace, field.type, elem_data, is_per_cpu, div, is_map_key)));
      }
      return struct_to_str(elems);
    }
    case Type::tuple: {
      std::vector<std::string> elems;
      for (auto &field : type.GetFields()) {
        std::vector<uint8_t> elem_data(value.begin() + field.offset,
                                       value.begin() + field.offset +
                                           field.type.GetSize());
        elems.push_back(value_to_str(
            bpftrace, field.type, elem_data, is_per_cpu, div, false));
      }
      return tuple_to_str(elems, false);
    }
    case Type::count_t: {
      return std::to_string(util::reduce_value<uint64_t>(value, nvalues) / div);
    }
    case Type::avg_t: {
      // on this code path, avg is calculated in the kernel while
      // printing the entire map is handled in a different function
      // which shouldn't call this
      assert(!is_per_cpu);
      if (type.IsSigned()) {
        return std::to_string(util::read_data<int64_t>(value.data()) / div);
      }
      return std::to_string(util::read_data<uint64_t>(value.data()) / div);
    }
    case Type::integer: {
      auto sign = type.IsSigned();
      switch (type.GetIntBitWidth()) {
          // clang-format off
          case 64:
            if (sign)
              return std::to_string(util::reduce_value<int64_t>(value, nvalues) / static_cast<int64_t>(div));
            return std::to_string(util::reduce_value<uint64_t>(value, nvalues) / div);
          case 32:
            if (sign)
              return std::to_string(
                  util::reduce_value<int32_t>(value, nvalues) / static_cast<int32_t>(div));
            return std::to_string(util::reduce_value<uint32_t>(value, nvalues) / div);
          case 16:
            if (sign)
              return std::to_string(
                  util::reduce_value<int16_t>(value, nvalues) / static_cast<int16_t>(div));
            return std::to_string(util::reduce_value<uint16_t>(value, nvalues) / div);
          case 8:
            if (sign)
              return std::to_string(
                  util::reduce_value<int8_t>(value, nvalues) / static_cast<int8_t>(div));
            return std::to_string(util::reduce_value<uint8_t>(value, nvalues) / div);
            // clang-format on
        default:
          LOG(BUG) << "value_to_str: Invalid int bitwidth: "
                   << type.GetIntBitWidth() << "provided";
          return {};
      }
    }
    case Type::boolean: {
      return util::read_data<uint8_t>(value.data()) ? "true" : "false";
    }
    case Type::sum_t: {
      if (type.IsSigned())
        return std::to_string(util::reduce_value<int64_t>(value, nvalues) /
                              div);

      return std::to_string(util::reduce_value<uint64_t>(value, nvalues) / div);
    }
    case Type::max_t:
    case Type::min_t: {
      if (is_per_cpu) {
        if (type.IsSigned()) {
          return std::to_string(
              util::min_max_value<int64_t>(value, nvalues, type.IsMaxTy()) /
              div);
        }
        return std::to_string(
            util::min_max_value<uint64_t>(value, nvalues, type.IsMaxTy()) /
            div);
      }
      if (type.IsSigned()) {
        return std::to_string(util::read_data<int64_t>(value.data()) / div);
      }
      return std::to_string(util::read_data<uint64_t>(value.data()) / div);
    }
    case Type::timestamp: {
      return bpftrace.resolve_timestamp(
          reinterpret_cast<const AsyncEvent::Strftime *>(value.data())->mode,
          reinterpret_cast<const AsyncEvent::Strftime *>(value.data())
              ->strftime_id,
          reinterpret_cast<const AsyncEvent::Strftime *>(value.data())->nsecs);
    }
    case Type::mac_address: {
      return bpftrace.resolve_mac_address(value.data());
    }
    case Type::cgroup_path_t: {
      return bpftrace.resolve_cgroup_path(
          reinterpret_cast<const AsyncEvent::CgroupPath *>(value.data())
              ->cgroup_path_id,
          reinterpret_cast<const AsyncEvent::CgroupPath *>(value.data())
              ->cgroup_id);
    }
    case Type::strerror_t: {
      return strerror(util::read_data<uint64_t>(value.data()));
    }
    case Type::none: {
      return "";
    }
    case Type::voidtype:
    case Type::hist_t:
    case Type::lhist_t:
    case Type::tseries_t:
    case Type::stack_mode:
    case Type::pointer:
    case Type::stats_t:
    case Type::timestamp_mode: {
      LOG(BUG) << "Invalid value type: " << type;
    }
  }
  return "";
}

std::string Output::map_key_str(BPFtrace &bpftrace,
                                const SizedType &arg,
                                const std::vector<uint8_t> &data) const
{
  std::ostringstream ptr;
  switch (arg.GetTy()) {
    case Type::integer:
    case Type::boolean:
    case Type::kstack_t:
    case Type::ustack_t:
    case Type::timestamp:
    case Type::ksym_t:
    case Type::usym_t:
    case Type::inet:
    case Type::username:
    case Type::string:
    case Type::buffer:
    case Type::pointer:
    case Type::array:
    case Type::mac_address:
    case Type::record:
    case Type::count_t:
    case Type::avg_t:
    case Type::max_t:
    case Type::min_t:
    case Type::sum_t:
      return value_to_str(bpftrace, arg, data, false, 1, true);
    case Type::tuple: {
      std::vector<std::string> elems;
      for (auto &field : arg.GetFields()) {
        std::vector<uint8_t> elem_data(data.begin() + field.offset,
                                       data.begin() + field.offset +
                                           field.type.GetSize());
        elems.push_back(
            value_to_str(bpftrace, field.type, elem_data, false, 1, true));
      }
      return tuple_to_str(elems, true);
    }
    case Type::cgroup_path_t:
    case Type::strerror_t:
    case Type::hist_t:
    case Type::lhist_t:
    case Type::tseries_t:
    case Type::none:
    case Type::stack_mode:
    case Type::stats_t:
    case Type::timestamp_mode:
    case Type::voidtype:
      LOG(BUG) << "Invalid mapkey argument type: " << arg;
  }
  return "";
}

std::string Output::array_to_str(const std::vector<std::string> &elems) const
{
  return "[" + util::str_join(elems, ",") + "]";
}

std::string Output::struct_to_str(const std::vector<std::string> &elems) const
{
  return "{ " + util::str_join(elems, ", ") + " }";
}

void Output::map_contents(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
        &values_by_key) const
{
  uint32_t i = 0;
  size_t total = values_by_key.size();
  const auto &map_type = bpftrace.resources.maps_info.at(map.name()).value_type;

  bool first = true;
  for (const auto &pair : values_by_key) {
    auto key = pair.first;
    auto value = pair.second;

    if (top) {
      if (total > top && i++ < (total - top))
        continue;
    }

    if (first)
      first = false;
    else
      map_elem_delim(map_type);

    auto key_str = map_key_to_str(bpftrace, map, key);
    auto value_str = value_to_str(
        bpftrace, map_type, value, map.is_per_cpu_type(), div);
    map_key_val(map_type, key_str, value_str);
  }
}

void Output::map_hist_contents(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::map<std::vector<uint8_t>, std::vector<uint64_t>> &values_by_key,
    const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
        &total_counts_by_key) const
{
  uint32_t i = 0;
  const auto &map_info = bpftrace.resources.maps_info.at(map.name());
  const auto &map_type = map_info.value_type;
  bool first = true;
  for (const auto &key_count : total_counts_by_key) {
    const auto &key = key_count.first;
    const auto &value = values_by_key.at(key);

    if (top && values_by_key.size() > top && i++ < (values_by_key.size() - top))
      continue;

    if (first)
      first = false;
    else
      map_elem_delim(map_type);

    auto key_str = map_key_to_str(bpftrace, map, key);
    std::string val_str;
    if (map_type.IsHistTy()) {
      if (!std::holds_alternative<HistogramArgs>(map_info.detail))
        LOG(BUG) << "call to hist with missing \"bits\" argument";
      val_str = hist_to_str(value,
                            div,
                            std::get<HistogramArgs>(map_info.detail).bits);
    } else {
      if (!std::holds_alternative<LinearHistogramArgs>(map_info.detail))
        LOG(BUG) << "call to lhist with missing arguments";
      const auto &args = std::get<LinearHistogramArgs>(map_info.detail);
      val_str = lhist_to_str(value, args.min, args.max, args.step);
    }
    map_key_val(map_type, key_str, val_str);
  }
}

void Output::map_tseries_contents(
    BPFtrace &bpftrace,
    const BpfMap &map,
    const TSeriesMap &values_by_key,
    const std::vector<std::pair<KeyType, EpochType>> &latest_epoch_by_key) const
{
  const auto &map_info = bpftrace.resources.maps_info.at(map.name());
  const auto &map_type = map_info.value_type;
  EpochType last_epoch = util::tseries_last_epoch(values_by_key);
  bool first = true;
  for (const auto &key_epoch : latest_epoch_by_key) {
    const auto &key = key_epoch.first;
    const auto &value = values_by_key.at(key);

    if (first)
      first = false;
    else
      map_elem_delim(map_type);

    if (!std::holds_alternative<TSeriesArgs>(map_info.detail))
      LOG(BUG) << "call to tseries with missing arguments";
    const auto &args = std::get<TSeriesArgs>(map_info.detail);
    auto key_str = map_key_to_str(bpftrace, map, key);
    std::string val_str = tseries_to_str(bpftrace,
                                         value,
                                         last_epoch,
                                         args.interval_ns,
                                         args.num_intervals,
                                         args.value_type);
    map_key_val(map_type, key_str, val_str);
  }
}

void Output::map_stats_contents(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
        &values_by_key) const
{
  const auto &map_type = bpftrace.resources.maps_info.at(map.name()).value_type;
  uint32_t i = 0;
  size_t total = values_by_key.size();
  bool first = true;

  for (const auto &[key, value] : values_by_key) {
    if (top && map_type.IsAvgTy()) {
      if (total > top && i++ < (total - top))
        continue;
    }

    if (first)
      first = false;
    else
      map_elem_delim(map_type);

    auto key_str = map_key_to_str(bpftrace, map, key);

    std::string total_str;
    std::string count_str;
    std::string avg_str;

    if (map_type.IsSigned()) {
      auto stats = util::stats_value<int64_t>(value, bpftrace.ncpus_);
      avg_str = std::to_string(stats.avg / div);
      total_str = std::to_string(stats.total);
      count_str = std::to_string(stats.count);
    } else {
      auto stats = util::stats_value<uint64_t>(value, bpftrace.ncpus_);
      avg_str = std::to_string(stats.avg / div);
      total_str = std::to_string(stats.total);
      count_str = std::to_string(stats.count);
    }

    std::string value_str;
    if (map_type.IsStatsTy()) {
      std::vector<std::pair<std::string, std::string>> stats = {
        { "count", std::move(count_str) },
        { "average", std::move(avg_str) },
        { "total", std::move(total_str) }
      };
      value_str = key_value_pairs_to_str(stats);
    } else {
      value_str = std::move(avg_str);
    }

    map_key_val(map_type, key_str, value_str);
  }
}

void TextOutput::map(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
        &values_by_key) const
{
  map_contents(bpftrace, map, top, div, values_by_key);
  out_ << std::endl;
}

std::string TextOutput::hist_to_str(const std::vector<uint64_t> &values,
                                    uint32_t div,
                                    uint32_t k) const
{
  int min_index, max_index, max_value;
  hist_prepare(values, min_index, max_index, max_value);
  if (max_index == -1)
    return "";

  std::ostringstream res;
  for (int i = min_index; i <= max_index; i++) {
    std::ostringstream header;

    // Index 0 is for negative values. Following that, each sequence
    // of N = 1 << k indexes represents one power of 2.
    // In particular:
    // - the first set of N indexes is for values 0..N-1
    //   (one value per index)
    // - the second and following sets of N indexes each contain
    //   <1, 2, 4 .. and subsequent powers of 2> values per index.
    //
    // Since the first and second set are closed intervals and the value
    // of each interval equals "index - 1", we print it directly.
    // Higher indexes contain multiple values and we use helpers to print
    // the range as open intervals.

    if (i == 0) {
      header << "(..., 0)";
    } else if (i <= (2 << k)) {
      header << "[" << (i - 1) << "]";
    } else {
      // Use a helper function to print the interval boundaries.
      header << "[" << hist_index_label(i - 1, k);
      header << ", " << hist_index_label(i, k) << ")";
    }

    int max_width = 52;
    int bar_width = values.at(i) / static_cast<float>(max_value) * max_width;
    std::string bar(bar_width, '@');

    res << std::setw(16) << std::left << header.str() << std::setw(8)
        << std::right << (values.at(i) / div) << " |" << std::setw(max_width)
        << std::left << bar << "|" << std::endl;
  }
  return res.str();
}

std::string TextOutput::lhist_to_str(const std::vector<uint64_t> &values,
                                     int min,
                                     int max,
                                     int step) const
{
  int max_index, max_value, buckets, start_value, end_value;
  lhist_prepare(values,
                min,
                max,
                step,
                max_index,
                max_value,
                buckets,
                start_value,
                end_value);
  if (max_index == -1)
    return "";

  std::ostringstream res;
  for (int i = start_value; i <= end_value; i++) {
    int max_width = 52;
    int bar_width = values.at(i) / static_cast<float>(max_value) * max_width;
    std::ostringstream header;
    if (i == 0) {
      header << "(..., " << lhist_index_label(min, step) << ")";
    } else if (i == (buckets + 1)) {
      header << "[" << lhist_index_label(max, step) << ", ...)";
    } else {
      header << "[" << lhist_index_label(((i - 1) * step) + min, step);
      header << ", " << lhist_index_label((i * step) + min, step) << ")";
    }

    std::string bar(bar_width, '@');

    res << std::setw(16) << std::left << header.str() << std::setw(8)
        << std::right << values.at(i) << " |" << std::setw(max_width)
        << std::left << bar << "|" << std::endl;
  }
  return res.str();
}

void TextOutput::map_hist(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::map<std::vector<uint8_t>, std::vector<uint64_t>> &values_by_key,
    const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
        &total_counts_by_key) const
{
  map_hist_contents(
      bpftrace, map, top, div, values_by_key, total_counts_by_key);
  out_ << std::endl;
}

void TextOutput::map_tseries(
    BPFtrace &bpftrace,
    const BpfMap &map,
    const TSeriesMap &values_by_key,
    const std::vector<std::pair<KeyType, EpochType>> &latest_epoch_by_key) const
{
  map_tseries_contents(bpftrace, map, values_by_key, latest_epoch_by_key);
  out_ << std::endl;
}

template <typename T>
std::string tseries_to_str_text(BPFtrace &bpftrace,
                                const TSeries &values,
                                EpochType last_epoch,
                                uint64_t interval_ns,
                                uint64_t buckets)
{
  constexpr int graph_width = 53;
  std::ostringstream res;

  // We have no reference point from which to render the rest of the graph.
  // Just print <no data> instead.
  if (!last_epoch) {
    res << "<no data>" << std::endl;
    return res.str();
  }

  auto min_max = util::tseries_bounds<T>(values,
                                         last_epoch - buckets + 1,
                                         last_epoch);
  T min_value = min_max.first;
  T max_value = min_max.second;

  // Ensure a range of at least three, so that we have concrete values for the
  if (min_value == max_value) {
    // Generally prefer if we can add some buffer on both sides of the points
    // in the graph, but don't overflow if our min or max is at the upper
    // bound of the range.
    if (min_value > std::numeric_limits<T>::min()) {
      min_value -= 1;
    }
    if (max_value < std::numeric_limits<T>::max()) {
      max_value += 1;
    }
  }

  std::string time_fmt = "%H:%M:%S";
  std::string time_legend = "hh:mm:ss";
  if (interval_ns <
      static_cast<uint64_t>(std::chrono::nanoseconds(1us).count())) {
    time_fmt += ".%k";
    time_legend += ".ns       ";
  } else if (interval_ns <
             static_cast<uint64_t>(std::chrono::nanoseconds(1ms).count())) {
    time_fmt += ".%f";
    time_legend += ".us    ";
  } else if (interval_ns <
             static_cast<uint64_t>(std::chrono::nanoseconds(1s).count())) {
    time_fmt += ".%l";
    time_legend += ".ms ";
  }

  constexpr int padding = 21;
  res << std::setw(time_legend.length()) << "" << " " << std::setw(padding)
      << std::left << min_value << std::setw(graph_width - padding)
      << std::right << max_value << std::endl;
  std::string top(graph_width - 2, '_');
  res << time_legend << " |" << top << "|" << std::endl;

  int zero_offset = 0;
  if (min_value < 0 && max_value > 0) {
    zero_offset = -1 * min_value / static_cast<float>(max_value - min_value) *
                  graph_width;
  }

  // First bucket that actually has data.
  EpochType first_epoch = util::tseries_first_epoch(values,
                                                    last_epoch,
                                                    buckets);

  for (EpochType epoch = last_epoch - buckets + 1; epoch <= last_epoch;
       epoch++) {
    const auto &v = values.find(epoch);
    auto ts = bpftrace.resolve_timestamp(static_cast<uint32_t>(
                                             TimestampMode::tai),
                                         epoch * interval_ns,
                                         time_fmt,
                                         false);
    std::string line(graph_width, ' ');

    res << ts << " ";

    line[0] = '|';
    line[graph_width - 1] = '|';
    if (zero_offset > 0) {
      line[zero_offset] = '.';
    }

    if (v != values.end() && epoch >= first_epoch) {
      T val = 0;
      if (v != values.end()) {
        val = util::read_data<T>(v->second.data());
      }

      int point_offset = (val - min_value) /
                         static_cast<float>(max_value - min_value) *
                         (graph_width - 1);
      line[point_offset] = '*';
      res << line << " " << val;
    } else {
      res << line << " -";
    }

    res << std::endl;
  }

  std::string bottom(graph_width, '_');
  bottom[0] = 'v';
  bottom[graph_width - 1] = 'v';
  res << std::setw(time_legend.length()) << "" << " " << bottom << std::endl;
  res << std::setw(time_legend.length()) << "" << " " << std::setw(padding)
      << std::left << min_value << std::setw(graph_width - padding)
      << std::right << max_value << std::endl;

  return res.str();
}

std::string TextOutput::tseries_to_str(BPFtrace &bpftrace,
                                       const TSeries &values,
                                       EpochType last_epoch,
                                       uint64_t interval_ns,
                                       uint64_t num_intervals,
                                       const SizedType &value_type) const
{
  if (value_type.IsSigned()) {
    return tseries_to_str_text<int64_t>(
        bpftrace, values, last_epoch, interval_ns, num_intervals);
  } else {
    return tseries_to_str_text<uint64_t>(
        bpftrace, values, last_epoch, interval_ns, num_intervals);
  }
}

void TextOutput::map_stats(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
        &values_by_key) const
{
  map_stats_contents(bpftrace, map, top, div, values_by_key);
  out_ << std::endl << std::endl;
}

void TextOutput::value(BPFtrace &bpftrace,
                       const SizedType &ty,
                       std::vector<uint8_t> &value) const
{
  out_ << value_to_str(bpftrace, ty, value, false, 1) << std::endl;
}

std::string TextOutput::value_to_str(BPFtrace &bpftrace,
                                     const SizedType &type,
                                     const std::vector<uint8_t> &value,
                                     bool is_per_cpu,
                                     uint32_t div,
                                     bool is_map_key) const
{
  switch (type.GetTy()) {
    case Type::pointer: {
      std::ostringstream res;
      res << "0x" << std::hex << util::read_data<uint64_t>(value.data());
      return res.str();
    }
    case Type::integer: {
      if (type.IsEnumTy() && div == 1) {
        assert(!is_per_cpu);

        const auto *data = value.data();
        const auto &enum_name = type.GetName();
        uint64_t enum_val;
        switch (type.GetIntBitWidth()) {
          case 64:
            enum_val = util::read_data<uint64_t>(data);
            break;
          case 32:
            enum_val = util::read_data<uint32_t>(data);
            break;
          case 16:
            enum_val = util::read_data<uint16_t>(data);
            break;
          case 8:
            enum_val = util::read_data<uint8_t>(data);
            break;
          default:
            LOG(BUG) << "value_to_str: Invalid int bitwidth: "
                     << type.GetIntBitWidth() << "provided";
            return {};
        }

        if (c_definitions_.enum_defs.contains(enum_name) &&
            c_definitions_.enum_defs[enum_name].contains(enum_val)) {
          return c_definitions_.enum_defs[enum_name][enum_val];
        } else {
          // Fall back to something comprehensible in case user somehow
          // tricked the type system into accepting an invalid enum.
          return std::to_string(enum_val);
        }
      }
      [[fallthrough]];
    }
    default: {
      return Output::value_to_str(
          bpftrace, type, value, is_per_cpu, div, is_map_key);
    }
  };
}

void TextOutput::message(MessageType type __attribute__((unused)),
                         const std::string &msg,
                         bool nl) const
{
  out_ << msg;
  if (nl)
    out_ << std::endl;
}

void TextOutput::lost_events(uint64_t lost) const
{
  out_ << "Lost " << lost << " events" << std::endl;
}

void TextOutput::attached_probes(uint64_t num_probes) const
{
  if (num_probes == 1)
    out_ << "Attached " << num_probes << " probe" << std::endl;
  else
    out_ << "Attached " << num_probes << " probes" << std::endl;
}

void TextOutput::helper_error(int retcode, const HelperErrorInfo &info) const
{
  LOG(WARNING,
      std::string(info.source_location),
      std::vector(info.source_context),
      out_)
      << get_helper_error_msg(info.func_id, retcode)
      << "\nAdditional Info - helper: " << info.func_id
      << ", retcode: " << retcode;
}

void TextOutput::benchmark_results(
    const std::map<std::string, uint32_t> &results) const
{
  const std::string BENCHMARK = "BENCHMARK";
  const std::string AVERAGE_TIME = "AVERAGE TIME";
  size_t longest_name = BENCHMARK.size();

  for (const auto &benchmark : results) {
    longest_name = std::max(longest_name, benchmark.first.size());
  }

  auto sep = [&]() {
    out_ << "+" << std::setw(longest_name + 2) << std::setfill('-') << ""
         << "+" << std::setw(AVERAGE_TIME.size() + 2) << std::setfill('-') << ""
         << "+" << std::endl;
  };

  sep();
  out_ << "| " << std::left << std::setw(longest_name) << std::setfill(' ')
       << BENCHMARK << " | " << AVERAGE_TIME << " |" << std::endl;
  sep();

  for (const auto &benchmark : results) {
    auto [unit, scale] = util::duration_str(
        std::chrono::nanoseconds(benchmark.second));
    std::stringstream val;
    val << benchmark.second / scale << unit;
    out_ << "| " << std::left << std::setw(longest_name) << std::setfill(' ')
         << benchmark.first << " | " << std::left
         << std::setw(AVERAGE_TIME.size()) << std::setfill(' ') << val.str()
         << " |" << std::endl;
  }

  sep();
  out_ << std::endl;
}

std::string TextOutput::field_to_str(const std::string &name,
                                     const std::string &value) const
{
  return "." + name + " = " + value;
}

std::string TextOutput::tuple_to_str(const std::vector<std::string> &elems,
                                     bool is_map_key) const
{
  if (!is_map_key) {
    return "(" + util::str_join(elems, ", ") + ")";
  }
  return util::str_join(elems, ", ");
}

std::string TextOutput::map_key_to_str(BPFtrace &bpftrace,
                                       const BpfMap &map,
                                       const std::vector<uint8_t> &key) const
{
  const auto &map_info = bpftrace.resources.maps_info.at(map.name());
  const auto &key_type = map_info.key_type;
  if (map_info.is_scalar)
    return map.name();

  return map.name() + "[" + map_key_str(bpftrace, key_type, key) + "]";
}

void TextOutput::map_key_val(const SizedType &map_type,
                             const std::string &key,
                             const std::string &val) const
{
  out_ << key;
  if (map_type.IsHistTy() || map_type.IsLhistTy() || map_type.IsTSeriesTy())
    out_ << ":\n";
  else
    out_ << ": ";
  out_ << val;
}

void TextOutput::map_elem_delim(const SizedType &map_type) const
{
  if (!map_type.IsKstackTy() && !map_type.IsUstackTy() &&
      !map_type.IsKsymTy() && !map_type.IsUsymTy() && !map_type.IsInetTy())
    out_ << "\n";
}

std::string TextOutput::key_value_pairs_to_str(
    std::vector<std::pair<std::string, std::string>> &keyvals) const
{
  std::vector<std::string> elems;
  for (auto &e : keyvals)
    elems.push_back(e.first + " " + e.second);
  return util::str_join(elems, ", ");
}

std::string JsonOutput::json_escape(const std::string &str) const
{
  std::ostringstream escaped;
  for (const char &c : str) {
    switch (c) {
      case '"':
        escaped << "\\\"";
        break;

      case '\\':
        escaped << "\\\\";
        break;

      case '\n':
        escaped << "\\n";
        break;

      case '\r':
        escaped << "\\r";
        break;

      case '\t':
        escaped << "\\t";
        break;

      default:
        // c always >= '\x00'
        if (c <= '\x1f') {
          escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                  << static_cast<int>(c);
        } else {
          escaped << c;
        }
    }
  }
  return escaped.str();
}

void JsonOutput::map(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
        &values_by_key) const
{
  if (values_by_key.empty())
    return;

  const auto &map_info = bpftrace.resources.maps_info.at(map.name());

  out_ << R"({"type": ")" << MessageType::map << R"(", "data": {)";
  out_ << "\"" << json_escape(map.name()) << "\": ";
  if (!map_info.is_scalar)
    out_ << "{";

  map_contents(bpftrace, map, top, div, values_by_key);

  if (!map_info.is_scalar)
    out_ << "}";
  out_ << "}}" << std::endl;
}

std::string JsonOutput::hist_to_str(const std::vector<uint64_t> &values,
                                    uint32_t div,
                                    uint32_t k) const
{
  int min_index, max_index, max_value;
  hist_prepare(values, min_index, max_index, max_value);
  if (max_index == -1)
    return "[]";

  std::ostringstream res;
  res << "[";
  for (int i = min_index; i <= max_index; i++) {
    if (i > min_index)
      res << ", ";

    res << "{";
    // See description in TextOutput::hist_to_str():
    // first index is for negative values, the next 2 sets of
    // N = 2^k indexes have one value each (equal to i -1)
    // and remaining sets of N indexes each cover one power of 2,
    // whose ranges are computed as described in hist_index_label()
    if (i == 0) {
      res << "\"max\": -1, ";
    } else if (i <= (2 << k)) {
      res << "\"min\": " << i - 1 << ", \"max\": " << i - 1 << ", ";
    } else {
      const uint32_t n = 1 << k;
      uint32_t power = ((i - 1) >> k) - 1;
      uint32_t bucket = (i - 1) & (n - 1);
      const long low = (1ULL << power) * (n + bucket);
      power = (i >> k) - 1;
      bucket = i & (n - 1);
      const long high = ((1ULL << power) * (n + bucket)) - 1;
      res << "\"min\": " << low << ", \"max\": " << high << ", ";
    }
    res << "\"count\": " << values.at(i) / div;
    res << "}";
  }
  res << "]";

  return res.str();
}

std::string JsonOutput::lhist_to_str(const std::vector<uint64_t> &values,
                                     int min,
                                     int max,
                                     int step) const
{
  int max_index, max_value, buckets, start_value, end_value;
  lhist_prepare(values,
                min,
                max,
                step,
                max_index,
                max_value,
                buckets,
                start_value,
                end_value);
  if (max_index == -1)
    return "[]";

  std::ostringstream res;
  res << "[";
  for (int i = start_value; i <= end_value; i++) {
    if (i > start_value)
      res << ", ";

    res << "{";
    if (i == 0) {
      res << "\"max\": " << min - 1 << ", ";
    } else if (i == (buckets + 1)) {
      res << "\"min\": " << max << ", ";
    } else {
      long low = ((i - 1) * step) + min;
      long high = (i * step) + min - 1;
      res << "\"min\": " << low << ", \"max\": " << high << ", ";
    }
    res << "\"count\": " << values.at(i);
    res << "}";
  }
  res << "]";

  return res.str();
}

void JsonOutput::map_hist(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::map<std::vector<uint8_t>, std::vector<uint64_t>> &values_by_key,
    const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
        &total_counts_by_key) const
{
  if (total_counts_by_key.empty())
    return;

  const auto &map_info = bpftrace.resources.maps_info.at(map.name());

  out_ << R"({"type": ")" << MessageType::hist << R"(", "data": {)";
  out_ << "\"" << json_escape(map.name()) << "\": ";
  if (!map_info.is_scalar)
    out_ << "{";

  map_hist_contents(
      bpftrace, map, top, div, values_by_key, total_counts_by_key);

  if (!map_info.is_scalar)
    out_ << "}";
  out_ << "}}" << std::endl;
}

void JsonOutput::map_tseries(
    BPFtrace &bpftrace,
    const BpfMap &map,
    const TSeriesMap &values_by_key,
    const std::vector<std::pair<KeyType, EpochType>> &latest_epoch_by_key) const
{
  const auto &map_info = bpftrace.resources.maps_info.at(map.name());

  out_ << R"({"type": ")" << MessageType::tseries << R"(", "data": {)";
  out_ << "\"" << json_escape(map.name()) << "\": ";
  if (!map_info.is_scalar) // check if this map has keys
    out_ << "{";

  map_tseries_contents(bpftrace, map, values_by_key, latest_epoch_by_key);

  if (!map_info.is_scalar) // check if this map has keys
    out_ << "}";
  out_ << "}}" << std::endl;
}

template <typename T>
std::string tseries_to_str_json(BPFtrace &bpftrace,
                                const TSeries &values,
                                EpochType last_epoch,
                                uint64_t interval_ns,
                                uint64_t num_intervals)
{
  EpochType first_epoch = util::tseries_first_epoch(values,
                                                    last_epoch,
                                                    num_intervals);
  std::ostringstream res;

  if (!first_epoch) {
    return "[]";
  }

  bool first = true;
  res << "[";
  for (EpochType epoch = first_epoch; epoch <= last_epoch; epoch++) {
    const auto &v = values.find(epoch);
    if (v == values.end()) {
      continue;
    }

    if (!first) {
      res << ",";
    } else {
      first = false;
    }

    T val = util::read_data<T>(v->second.data());

    auto ts = bpftrace.resolve_timestamp(static_cast<uint32_t>(
                                             TimestampMode::tai),
                                         epoch * interval_ns,
                                         "%FT%H:%M:%S.%kZ",
                                         true);

    res << "{";
    res << "\"interval_start\"" << ":" << "\"" << ts << "\"," << "\"value\""
        << ":" << val;
    res << "}";
  }
  res << "]";

  return res.str();
}

std::string JsonOutput::tseries_to_str(BPFtrace &bpftrace,
                                       const TSeries &values,
                                       EpochType last_epoch,
                                       uint64_t interval_ns,
                                       uint64_t num_intervals,
                                       const SizedType &value_type) const
{
  if (value_type.IsSigned()) {
    return tseries_to_str_json<int64_t>(
        bpftrace, values, last_epoch, interval_ns, num_intervals);
  } else {
    return tseries_to_str_json<uint64_t>(
        bpftrace, values, last_epoch, interval_ns, num_intervals);
  }
}

void JsonOutput::map_stats(
    BPFtrace &bpftrace,
    const BpfMap &map,
    uint32_t top,
    uint32_t div,
    const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
        &values_by_key) const
{
  if (values_by_key.empty())
    return;

  const auto &map_info = bpftrace.resources.maps_info.at(map.name());

  out_ << R"({"type": ")" << MessageType::stats << R"(", "data": {)";
  out_ << "\"" << json_escape(map.name()) << "\": ";
  if (!map_info.is_scalar)
    out_ << "{";

  map_stats_contents(bpftrace, map, top, div, values_by_key);

  if (!map_info.is_scalar)
    out_ << "}";
  out_ << "}}" << std::endl;
}

void JsonOutput::value(BPFtrace &bpftrace,
                       const SizedType &ty,
                       std::vector<uint8_t> &value) const
{
  out_ << R"({"type": ")" << MessageType::value << R"(", "data": )"
       << value_to_str(bpftrace, ty, value, false, 1, false) << "}"
       << std::endl;
}

void JsonOutput::message(MessageType type,
                         const std::string &msg,
                         bool nl __attribute__((unused))) const
{
  out_ << R"({"type": ")" << type << R"(", "data": ")" << json_escape(msg)
       << "\"}" << std::endl;
}

void JsonOutput::message(MessageType type,
                         const std::string &field,
                         uint64_t value) const
{
  out_ << R"({"type": ")" << type << R"(", "data": )" << "{\"" << field
       << "\": " << value << "}" << "}" << std::endl;
}

void JsonOutput::lost_events(uint64_t lost) const
{
  message(MessageType::lost_events, "events", lost);
}

void JsonOutput::attached_probes(uint64_t num_probes) const
{
  message(MessageType::attached_probes, "probes", num_probes);
}

void JsonOutput::helper_error(int retcode, const HelperErrorInfo &info) const
{
  out_ << R"({"type": "helper_error", "msg": ")"
       << get_helper_error_msg(info.func_id, retcode) << R"(", "helper": ")"
       << info.func_id << R"(", "retcode": )" << retcode << R"(, "filename": ")"
       << info.filename << R"(", "line": )" << info.line << R"(, "col": )"
       << info.column << "}" << std::endl;
}

void JsonOutput::benchmark_results(
    const std::map<std::string, uint32_t> &results) const
{
  if (results.empty()) {
    return;
  }

  out_ << R"({"type": ")" << "benchmark_results" << R"(", "data": {)";
  bool first = true;
  for (const auto &benchmark : results) {
    if (!first) {
      out_ << ",";
    } else {
      first = false;
    }
    out_ << "\"" << benchmark.first << "\":" << benchmark.second;
  }
  out_ << "}}" << std::endl;
}

std::string JsonOutput::field_to_str(const std::string &name,
                                     const std::string &value) const
{
  return "\"" + name + "\": " + value;
}

std::string JsonOutput::tuple_to_str(const std::vector<std::string> &elems,
                                     bool is_map_key) const
{
  if (!is_map_key) {
    return "[" + util::str_join(elems, ",") + "]";
  }
  return util::str_join(elems, ",");
}

std::string JsonOutput::value_to_str(BPFtrace &bpftrace,
                                     const SizedType &type,
                                     const std::vector<uint8_t> &value,
                                     bool is_per_cpu,
                                     uint32_t div,
                                     bool is_map_key) const
{
  std::string str;

  switch (type.GetTy()) {
    case Type::pointer:
      str = std::to_string(util::read_data<uint64_t>(value.data()));
      break;
    default:
      str = Output::value_to_str(
          bpftrace, type, value, is_per_cpu, div, is_map_key);
  };

  if (is_quoted_type(type)) {
    if (is_map_key) {
      return json_escape(str);
    } else {
      return "\"" + json_escape(str) + "\"";
    }
  }

  return str;
}

std::string JsonOutput::map_key_to_str(BPFtrace &bpftrace,
                                       const BpfMap &map,
                                       const std::vector<uint8_t> &key) const
{
  const auto &map_info = bpftrace.resources.maps_info.at(map.name());
  if (map_info.is_scalar)
    return "";

  return "\"" + json_escape(map_key_str(bpftrace, map_info.key_type, key)) +
         "\"";
}

void JsonOutput::map_key_val(const SizedType &map_type __attribute__((unused)),
                             const std::string &key,
                             const std::string &val) const
{
  out_ << (key.empty() ? val : key + ": " + val);
}

void JsonOutput::map_elem_delim(const SizedType &map
                                __attribute__((unused))) const
{
  out_ << ", ";
}

std::string JsonOutput::key_value_pairs_to_str(
    std::vector<std::pair<std::string, std::string>> &keyvals) const
{
  std::vector<std::string> elems;
  for (auto &e : keyvals)
    elems.push_back("\"" + e.first + "\": " + e.second);
  return "{" + util::str_join(elems, ", ") + "}";
}

} // namespace bpftrace
