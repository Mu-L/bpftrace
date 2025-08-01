#include "common.h"

namespace bpftrace::test::codegen::call_len {
constexpr auto PROG = "begin { @x[1] = 1; } kprobe:f { $s = len(@x); }";

TEST_F(codegen_btf, call_len_for_each_map_elem)
{
  auto bpftrace = get_mock_bpftrace();
  auto feature = std::make_unique<MockBPFfeature>();
  feature->add_to_available_kernel_funcs(Kfunc::bpf_map_sum_elem_count, false);
  bpftrace->feature_ = std::move(feature);
  test(*bpftrace, PROG, NAME);
}

TEST_F(codegen_btf, call_len_map_sum_elem_count)
{
  auto bpftrace = get_mock_bpftrace();
  auto feature = std::make_unique<MockBPFfeature>();
  feature->add_to_available_kernel_funcs(Kfunc::bpf_map_sum_elem_count, true);
  bpftrace->feature_ = std::move(feature);
  test(*bpftrace, PROG, NAME);
}

TEST_F(codegen_btf, call_len_ustack_kstack)
{
  test("kprobe:f { @x = len(ustack); @y = len(kstack); }", NAME);
}

} // namespace bpftrace::test::codegen::call_len
