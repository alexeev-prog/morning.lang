#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "morninglang" ? 0 : 1;
}
