#include "common.h"

namespace bpftrace::test::codegen {

// Trip counts under 10 are usually unrolled automatically.
//
// This tests that the loop isn't unrolled.
TEST(codegen, while_loop_no_unroll)
{
  test("i:s:1 { $a = 0; while ($a <= 10) { @=$a++; }}", NAME);
}

} // namespace bpftrace::test::codegen
