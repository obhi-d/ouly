
#include "catch2/catch_all.hpp"

// NOLINTBEGIN
int main(int argc, char* argv[])
{
  // your setup ...

  int result = Catch::Session().run(argc, argv);

  // your clean-up...

  return result;
}
// NOLINTEND