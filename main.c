#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
// -lpthread

typedef struct {
  int dimension;
  int* entries;
} squareMatrix;

squareMatrix* newSquareMatrix(int dimension) {
  squareMatrix* ret = (squareMatrix*)malloc(sizeof(squareMatrix));
  if (dimension < 1) {
    return ret;
  }
  int d = dimension;
  ret->dimension = d;
  ret->entries = (int*)malloc(d*d*sizeof(int));
  return ret;
}

void printMatrix(squareMatrix* m) {
  for (int i=0; i<m->dimension; ++i) {
    for (int j=0; j<m->dimension; ++j) {
      printf("%d ", m->entries[j + i*m->dimension]); 
    } 
    printf("\n");
  }
  printf("\n");
}

void freeMatrix(squareMatrix* m) {
  free(m->entries);
  free(m);
}

squareMatrix* cofactorMatrix(squareMatrix* m, int idx) {
  int d = m->dimension;
  int dd=d-1;
  squareMatrix* ret = newSquareMatrix(d-1);
  int ii=0;
  int jIdx = idx%d;
  for (int i=1; i<d; ++i) { // note this starts at i=1 to skip first row
    for (int j=0; j<d; ++j) {
      if (j==jIdx) {
        continue;
      }
      ret->entries[ii] = m->entries[j + i*d]; 
      ii++;
    }
  }
  return ret;
}

int determinant_(squareMatrix* m); // declare

int smartCofactorMul(int a, squareMatrix* m, int idx) {
  if (a==0) { // so many zeros so we can speed up
    return 0;
  }
  squareMatrix* c = cofactorMatrix(m, idx);
  if (c->dimension == 1) {
    int product = a * c->entries[0];
    freeMatrix(c);
    return product; 
  }
  int product = a * determinant_(c);
  freeMatrix(c);
  return product;
}

int polarity(int idx, int dimension) {
  int i = idx/dimension;
  int j = idx%dimension;
  if ((i+j)%2==0) {
    return 1;
  }
  return -1;
}

typedef struct {
  squareMatrix* m;
  int idx;
  int result;
} threadPacket;

threadPacket* newThreadPacket(squareMatrix* m, int idx) { 
  threadPacket* ret = (threadPacket*)malloc(sizeof(threadPacket));
  ret->m = m;
  ret->idx = idx;
  ret->result = 0; // initialize
  return ret;
}

int determinant_(squareMatrix* m) { // lower level 
  int ret=0;
  for (int i=0; i<m->dimension; i++) { // only the top row
    ret += polarity(i, m->dimension) * smartCofactorMul(m->entries[i], m, i); 
  }
  return ret;
}

void* computeSummand(void* ptr) {
  threadPacket* tp = (threadPacket*)ptr;
  int i = tp->idx;
  squareMatrix* m = tp->m;
  int ret = polarity(i, m->dimension) * smartCofactorMul(m->entries[i], m, i); 
  tp->result = ret;
}

int determinant(squareMatrix* m) { // top level with concurrency
  int ret=0;
  pthread_t threads[m->dimension];
  threadPacket* tps[m->dimension];
  for (int i=0; i<m->dimension; ++i) { // only the top row
    tps[i] = newThreadPacket(m, i);
    pthread_t t;
    threads[i] = t;
    pthread_create(&threads[i], NULL, computeSummand, (void*)tps[i]);
  }
  for (int i = 0; i<m->dimension; ++i) {
    pthread_join(threads[i], NULL); 
    ret += tps[i]->result;
    free(tps[i]);
  }
  return ret;
}

int main() {

  // easy test
  printf("debug test correctness\n");
  squareMatrix* m2 = newSquareMatrix(3);  
  m2->entries[0] = 83;
  m2->entries[1] = 86;
  m2->entries[2] = 77;
  m2->entries[3] = 15;
  m2->entries[4] = 93;
  m2->entries[5] = 35;
  m2->entries[6] = 86;
  m2->entries[7] = 92;
  m2->entries[8] = 49;
  printf("%d\n", determinant(m2));
  assert(determinant(m2) == -202965); // according to Wolfram Alpha

  // stress test 1
  printf("\ndebug stress test 1\n");
  
  int dimension = 6;
  int d2 = dimension*dimension;
  squareMatrix* m = newSquareMatrix(dimension);
  for (int i=0; i<d2; i++) {
    m->entries[i] = rand()%100000; 
  }

  printMatrix(m);

  int d = determinant(m);
  printf("debug %d\n", d);

  // stress test 2
  printf("\ndebug stress test 2\n");

  dimension = 10; 
  d2 = dimension*dimension;
  m = newSquareMatrix(dimension);
  for (int i=0; i<d2; i++) {
    m->entries[i] = rand()%10000000; 
  }

  printMatrix(m);

  // haven't tested if this value is correct
  // but I suspect I should make math -> safeMath
  d = determinant(m);
  printf("debug %d\n", d);

  return 0;
}
