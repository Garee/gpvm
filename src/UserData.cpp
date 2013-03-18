#include "UserData.h"
#include <ctime>

/* Generate a random number between 0 and max. */
int randomNumber(int max) {
return (rand() % (max + 1));
}

void populateData(cl_uint *data) {
  /* Initialise seed for random number generation. */
  srand(1);
  
  int dim = 256; // N rows of a square matrix.

  /* Total number of memory sections allocated. */
  data[0] = 6;

  /* Pointers to allocated memory. */
  data[1] = 256;
  data[2] = data[1] + (dim * dim);
  data[3] = data[2] + (dim * dim);
  data[4] = data[3] + (dim * dim);
  data[5] = dim;
  data[6] = data[4] + (dim * dim);
  
  /* Pointer to scratch memory. */
  data[data[0] + 1] = data[6] + (dim * dim); // Pointer to scratch free/scratch memory.
  
  /* Populate input matrices. */
  for (uint i = data[1]; i < data[2]; i++) {
    data[i] = randomNumber(10);
  }
  
  for (uint i = data[2]; i < data[3]; i++) {
    data[i] = randomNumber(10);
  }
}
