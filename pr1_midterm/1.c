#include <stdio.h>
#include <matheval.h>
#include <stdlib.h>
#include <assert.h>

double calculateIntegration(char *str,double up,double down,double step){

  double rate = (up-down)/step;
  void *func = evaluator_create(str);


  if(func == 0){
    fprintf(stderr,"Syntax error!\n");
    exit(0);
  }

  double xi = rate; // xi = x1

  int i=1;
  double total=0;
  for(i=1;i<step;++i){
    total+= evaluator_evaluate_x(func,xi);
    xi+=rate;
  }

  double firstValue = evaluator_evaluate_x (func,down);
  double lastValue = evaluator_evaluate_x(func,up);
  return (up-down)*(firstValue+ 2*total+lastValue )/(2.0*step);
}

int main(){


  char str[100]="0.2+25*x-200*x*x+675*x*x*x-900*x*x*x*x+400*x*x*x*x*x";
  char str2[100] = "sin (  s )*s*s*s + cos(5*t)";
  printf("Result  : %f\n",calculateIntegration(str2,0.8,0,8));




  return 0;
  }
