#include <boost/mpi.hpp>
#include <iostream>
#include <string>
#include <functional>
#include <boost/serialization/string.hpp>
namespace mpi = boost::mpi;

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  mpi::communicator world;

  std::string names[10] = { "zero ", "one ", "two ", "three ",
                            "four ", "five ", "six ", "seven ",
                            "eight ", "nine " };

  std::string result;
  reduce(world,
         world.rank() < 10? names[world.rank()]
                          : std::string("many "),
         result, std::plus<std::string>(), 1);

  if (world.rank() == 1)
    std::cout << "The result is " << result << std::endl;

  return 0;
}

