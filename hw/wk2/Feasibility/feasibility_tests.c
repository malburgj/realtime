#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0
#define U32_T unsigned int

U32_T ex0_period[] = {2, 10, 15};
U32_T ex0_wcet[] = {1, 1, 2};
uint32_t ex0_numSer = sizeof(ex0_period) / sizeof(U32_T);

U32_T ex1_period[] = {2, 5, 7};
U32_T ex1_wcet[] = {1, 1, 2};
uint32_t ex1_numSer = sizeof(ex1_period) / sizeof(U32_T);

U32_T ex2_period[] = {2, 5, 7, 13};
U32_T ex2_wcet[] = {1, 1, 1, 2};
uint32_t ex2_numSer = sizeof(ex2_period) / sizeof(U32_T);

U32_T ex3_period[] = {3, 5, 15};
U32_T ex3_wcet[] = {1, 2, 3};
uint32_t ex3_numSer = sizeof(ex3_period) / sizeof(U32_T);

U32_T ex4_period[] = {2, 4, 16};
U32_T ex4_wcet[] = {1, 1, 4};
uint32_t ex4_numSer = sizeof(ex4_period) / sizeof(U32_T);

U32_T ex5_period[] = {2, 5, 10};
U32_T ex5_wcet[] = {1, 2, 1};
uint32_t ex5_numSer = sizeof(ex5_period) / sizeof(U32_T);

U32_T ex6_period[] = {2, 5, 7, 13};
U32_T ex6_wcet[] = {1, 1, 1, 2};
uint32_t ex6_numSer = sizeof(ex6_period) / sizeof(U32_T);

U32_T ex7_period[] = {3, 5, 15};
U32_T ex7_wcet[] = {1, 2, 4};
uint32_t ex7_numSer = sizeof(ex7_period) / sizeof(U32_T);

U32_T ex8_period[] = {2, 5, 7, 13};
U32_T ex8_wcet[] = {1, 1, 1, 2};
uint32_t ex8_numSer = sizeof(ex8_period) / sizeof(U32_T);

U32_T ex9_period[] = {6, 8, 12, 24};
U32_T ex9_wcet[] = {1, 2, 4, 6};
uint32_t ex9_numSer = sizeof(ex9_period) / sizeof(U32_T);

U32_T ex10_period[] = {2, 5, 7, 14};
U32_T ex10_wcet[] = {1, 1, 1, 2};
uint32_t ex10_numSer = sizeof(ex10_period) / sizeof(U32_T);

int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
int scheduling_point_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
void print_test(uint32_t numServices, uint32_t *pPeriod, uint32_t *pWcet, uint8_t exNum, 
    int (*testfunc)(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]));

int main(void)
{
  int i;
  U32_T numServices;

  printf("******** Completion Test Feasibility Example\n");

  #define NUM_TST 5
  U32_T *period[] = {  ex0_period, ex1_period, ex2_period, ex3_period, ex4_period, ex5_period, ex6_period, ex7_period, ex8_period, ex9_period, ex10_period};
  U32_T *wcet[]   = {  ex0_wcet,   ex1_wcet,   ex2_wcet,   ex3_wcet,   ex4_wcet,   ex5_wcet,   ex6_wcet,   ex7_wcet,   ex8_wcet,   ex9_wcet,   ex10_wcet};
  uint32_t num[]  = {  ex0_numSer, ex1_numSer, ex2_numSer, ex3_numSer, ex4_numSer, ex5_numSer, ex6_numSer, ex7_numSer, ex8_numSer, ex9_numSer, ex10_numSer};
  for(int testInd = 0; testInd < NUM_TST; ++testInd)
  {
    print_test(num[testInd], period[testInd], wcet[testInd], testInd, completion_time_feasibility);
  }

  printf("\n\n");
  printf("******** Scheduling Point Feasibility Example\n");

    for(int testInd = 0; testInd < NUM_TST; ++testInd)
  {
    print_test(num[testInd], period[testInd], wcet[testInd], testInd, scheduling_point_feasibility);
  }
}

int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
  int i, j;
  U32_T an, anext;

  // assume feasible until we find otherwise
  int set_feasible = TRUE;

  //printf("numServices=%d\n", numServices);

  for (i = 0; i < numServices; i++)
  {
    an = 0;
    anext = 0;

    for (j = 0; j <= i; j++)
    {
      an += wcet[j];
    }

    //printf("i=%d, an=%d\n", i, an);

    while (1)
    {
      anext = wcet[i];

      for (j = 0; j < i; j++)
        anext += ceil(((double)an) / ((double)period[j])) * wcet[j];

      if (anext == an)
        break;
      else
        an = anext;

      //printf("an=%d, anext=%d\n", an, anext);
    }

    //printf("an=%d, deadline[%d]=%d\n", an, i, deadline[i]);

    if (an > deadline[i])
    {
      set_feasible = FALSE;
    }
  }

  return set_feasible;
}

int scheduling_point_feasibility(U32_T numServices, U32_T period[],
                                 U32_T wcet[], U32_T deadline[])
{
  int rc = TRUE, i, j, k, l, status, temp;

  for (i = 0; i < numServices; i++) // iterate from highest to lowest priority
  {
    status = 0;

    for (k = 0; k <= i; k++)
    {
      for (l = 1; l <= (floor((double)period[i] / (double)period[k])); l++)
      {
        temp = 0;

        for (j = 0; j <= i; j++)
          temp += wcet[j] * ceil((double)l * (double)period[k] / (double)period[j]);

        if (temp <= (l * period[k]))
        {
          status = 1;
          break;
        }
      }
      if (status)
        break;
    }
    if (!status)
      rc = FALSE;
  }
  return rc;
}

  void print_test(uint32_t numServices, uint32_t *pPeriod, uint32_t *pWcet, uint8_t exNum, 
    int (*testfunc)(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]))
  {
    float utot = 0.0f;
    for(int ind = 0; ind < numServices; ++ind) {
      utot += (float)pWcet[ind] / (float)pPeriod[ind];
    }
    printf("Ex-%d U=%4.2f (", exNum,
          utot);

    for(int ind = 0; ind < numServices; ++ind) {
      printf("C%d=%d, ", ind + 1, pWcet[ind]);
    }
    for(int ind = 0; ind < numServices; ++ind) {
      printf("T%d=%d", ind + 1, pPeriod[ind]);
      if(ind != numServices - 1) {
        printf(", ");
      }
    }
    printf("; T=D): ");
    
    if (testfunc(numServices, pPeriod, pWcet, pPeriod) == TRUE)
      printf("FEASIBLE\n");
    else
      printf("INFEASIBLE\n");
  }
