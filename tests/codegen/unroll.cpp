#include "common.h"

namespace bpftrace::test::codegen {

TEST(codegen, unroll)
{
  test("begin { @i = 0; unroll(5) { @i += 1 } }", NAME);
}

} // namespace bpftrace::test::codegen
