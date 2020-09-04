#include "bf/JobSystemExt.hpp"

#include <iostream>

int main()
{
  using namespace bf::job;

  if (!bf::job::initialize())
  {
    return 1;
  }

#define DATA_SIZE 100000

  int data[DATA_SIZE];

  for (int i = 0; i < DATA_SIZE; ++i)
  {
    data[i] = i;
  }

  std::printf("Before:\n");
  for (int i = 0; i < 20; ++i)
  {
    std::printf("data[%i] = %i\n", i, data[i]);
  }

  auto* t = bf::job::parallel_for(
   data, DATA_SIZE, bf::job::CountSplitter{6}, [](int* data, std::size_t data_size) {
     for (std::size_t i = 0; i < data_size; ++i)
     {
       data[i] = data[i] * 5;
     }
   });

  bf::job::taskSubmit(t);

  bf::job::waitOnTask(t);

  std::printf("After:\n");
  for (int i = 0; i < 20; ++i)
  {
    std::printf("data[%i] = %i\n", i, data[i]);
  }

  bf::job::shutdown();

  return 0;
}
