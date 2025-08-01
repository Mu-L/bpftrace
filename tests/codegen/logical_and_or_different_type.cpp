#include "common.h"

namespace bpftrace::test::codegen {

TEST(codegen, logical_and_or_different_type)
{
  test("struct Foo { int m; }"
       "begin"
       "{"
       "  $foo = *(struct Foo*)0;"
       "  printf(\"%d %d %d %d\", $foo.m && 0, 1 && $foo.m, $foo.m || 0, 0 || "
       "$foo.m)"
       "}",
       NAME);
}

} // namespace bpftrace::test::codegen
