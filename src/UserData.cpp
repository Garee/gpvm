#include "UserData.h"
#include <ctime>
#include <iostream>

/* Generate a random number between 0 and max. */
int randomNumber(int max) {
  return (rand() % (max + 1));
}

void populateData2(cl_uint *data) {
  /* Initialise seed for random number generation. */
  srand(1);
  
  int dim = 2; // N rows of a square matrix.
  
  /* Total number of memory sections allocated. */
  data[0] = 3;
  
  /* Pointers to allocated memory. */
  data[1] = 256;
  data[2] = data[1] + (dim * dim);
  data[3] = data[2] + (dim * dim);
  
  /* Pointer to scratch memory. */
  data[data[0] + 1] = data[3] + (dim * dim); // Pointer to scratch free/scratch memory.
  
  /* Populate input matrices. */
  /*
    for (uint i = data[1]; i < data[2]; i++) {
    data[i] = randomNumber(10);
    }
  
    for (uint i = data[2]; i < data[3]; i++) {
    data[i] = randomNumber(10);
    }
  */
  
  data[data[1]] = 1;
  data[data[1] + 1] = 2;
  data[data[1] + 2] = -3;
  data[data[1] + 3] = 11;
  
  data[data[2]] = -2;
  data[data[2] + 1] = 4;
  data[data[2] + 2] = 7;
  data[data[2] + 3] = 1;
}

void populateData(cl_uint *data) {
  /* Initialise seed for random number generation. */
  srand(1);

  int dim = 2; // N rows of a square matrix.

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
  /*
  for (uint i = data[1]; i < data[2]; i++) {
    data[i] = randomNumber(10);
  }
  
  for (uint i = data[2]; i < data[3]; i++) {
    data[i] = randomNumber(10);
    }
  */

  data[data[1]] = 1;
  data[data[1] + 1] = 2;
  data[data[1] + 2] = -3;
  data[data[1] + 3] = 11;
  
  data[data[2]] = -2;
  data[data[2] + 1] = 4;
  data[data[2] + 2] = 7;
  data[data[2] + 3] = 1;
}


