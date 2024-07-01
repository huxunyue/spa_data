#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h> 
#include <time.h> 
#include <string.h>
#include <assert.h>

#define NB_SAMPLES 490000000
#define NUM_TRACE 1
#define LEN_PATTERN 30 //1692
#define OFFSET_POI 2  //14

const char* path = "traces.raw";
const char* pattern_path = "patterns_simple.raw";
// const char* pattern_path = "patterns_mean_simple.raw";

float * traces[NUM_TRACE]; // to be adapted if needed
float patterns[2][LEN_PATTERN];

#define min(x, y) ((x) < (y) ? (x) : (y))

float mean(float lst[], int size){//calcul la moyenne
    float average = 0;
    for (int i = 0; i < size; i++) {
        average = average + (lst[i] - average) / (i + 1);
    }
    return average;
}

int cmp(const void * e1, const void * e2)
{
    if((*(float*)e1-*(float*)e2)>0)
    {
        return 1;
    }
    return 0;
}

void find_poi_threshold(int n, int lst[2048], float trace[NB_SAMPLES], float * threshold){ // n is number of poi
  float cons[n];
  float max_diff = 0;
  float tmp_threshold = 0;
  for(int j = 0; j < n; j++){
    cons[j] = trace[lst[j]];
  }
  qsort(cons,n,sizeof(float),cmp);
  for(int j = 0; j < n-1; j++){
    if((cons[j+1] - cons[j]) > max_diff){
      max_diff = cons[j+1] - cons[j];
      tmp_threshold = (max_diff/2) + cons[j];
      // printf("cons[j+1]: %f, cons[j]: %f\n",cons[j+1],cons[j]);
      // printf("max_diff: %f, tmp_threshold: %f\n",max_diff,tmp_threshold);
      }
  }
  (*threshold) = tmp_threshold;
}

double HW(uint8_t n){ // calculate the hamming weight of un data uint8_t 
    double nb=0;
	while(n>0){
		if((n&1) == 1) nb++;
		n=n>>1;
    }
	return nb;
}


int parsing(int lst[2048], float pattern0[LEN_PATTERN], float pattern1[LEN_PATTERN],float trace[NB_SAMPLES]){

  int c = 0;
  double diff0max = 0;
  double diff1max = 0;

  for (int i = 0; i < (NB_SAMPLES - LEN_PATTERN);  i++) {
    double diff0 = 0;
    double diff1 = 0;
    for (int j = 0; j < LEN_PATTERN;  j++) {
      diff0 += fabs(pattern0[j] - trace[i+j]);
      diff1 += fabs(pattern1[j] - trace[i+j]);
    }
    if( c < 3 ){
      if((diff0/LEN_PATTERN < 0.01) || (diff1/LEN_PATTERN < 0.01)){
        lst[c] = i + OFFSET_POI; 
        c++;
      }
    }
    else if((diff0/LEN_PATTERN < 0.0085) || (diff1/LEN_PATTERN < 0.0087)){  //0.012550 
    // if((diff0/LEN_PATTERN < 0.011) || (diff1/LEN_PATTERN < 0.011)){  //0.012550 
      assert(c < 2048);
      if(diff0/LEN_PATTERN < 0.0085){
        if((diff0 > diff0max)){
          diff0max = diff0;
        }
      } else if(diff1 > diff1max){
        diff1max = diff1;
      }
      lst[c] = i + OFFSET_POI; 
      if( (lst[c]-lst[c-1])>250000){
        printf("[%d:%d]\n", lst[c-1], lst[c]);
      }
      c++;
      if(c > 3){i += 230000;}
    }
  }
  printf("diff0max: %f\n",diff0max/LEN_PATTERN);
  printf("diff1max: %f\n",diff1max/LEN_PATTERN);


  // double diff0 = 0;
  // double diff1 = 0;
  // for (int j = 0; j < LEN_PATTERN;  j++) {
  //   diff0 += fabs(pattern0[j] - trace[410534488-2+j]);
  //   diff1 += fabs(pattern1[j] - trace[410534488-2+j]);
  // }
  // printf("diff0: %f\n", diff0/LEN_PATTERN);
  // printf("diff1: %f\n", diff1/LEN_PATTERN);
  printf("c = %d\n", c);
  return c;
}

int main(int argc, char const *argv[]) {
  clock_t begin,end;
  int poi[2048] = {0};
  int key_found[2048] = {0};
  float poi_threshold;
  int fd;

  begin = clock();

  for (int i = 0; i < NUM_TRACE; i += 1) {
    traces[i] = malloc(sizeof(float) * NB_SAMPLES);
  }

  if ((fd = open(path, O_RDONLY)) == -1) {
    perror("open");
    return -1;
  }

  for(int i=0; i<NUM_TRACE;i++) {
    uint64_t offset = 0;
    uint64_t step = 100000000;
    while (offset < NB_SAMPLES) {
      // uint64_t nb_read_samples = min(step, offset + step - NB_SAMPLES);
      uint64_t nb_read_samples = min(step, NB_SAMPLES - offset);
      if (read(fd, &traces[i][offset], sizeof(float) * nb_read_samples) == -1) {
        perror("read");
        return -1;
      }
      offset += step;
    }
  }

  // for (int i = 0; i < 10; i += 1) {
  //   printf("%f\n", traces[0][i]);
  // }

  if ((fd = open(pattern_path, O_RDONLY)) == -1) {
    perror("open");
    return -1;
  }

  for(int i=0; i<2;i++){
    float *pattern = patterns[i];
    if (read(fd, pattern, sizeof(float) * LEN_PATTERN) == -1) {
      perror("read");
      return -1;
    }
  }

  float *trace = traces[0];

  int nb = parsing(poi, patterns[0], patterns[1], trace);
  for (int i = 0; i < nb-1; i ++) {
    printf("%d, ", poi[i]);
  }
  printf("%d\n", poi[nb-1]);

  find_poi_threshold(nb, poi, trace, &poi_threshold);
  printf("poi_threshold: %f\n",poi_threshold);
  // exit(0);
  key_found[0] = 1;
  int c = 1;
  for (int i = 0; i < nb; i ++) {
    // printf("%f, ",trace[poi[i]]);
    if((trace[poi[i]] < poi_threshold)){
    // if((trace[poi[i]] < -0.11) && (trace[poi[i]] != 0)){
    // if((trace[poi[i]] < trace[poi[i]+1]) && (trace[poi[i]] != 0)){
      key_found[c] = 1;
    }
    else {
      key_found[c] = 0;
    }
    c++;
  }

  end = clock();
  printf("Time: %lu clocks\n ",(end-begin));
  printf("%lu CLOCKS PER SEC\n", CLOCKS_PER_SEC);

  printf("key found:       0b");
  for (int i = 0; i < nb+1; i ++) {
    printf("%d",key_found[i]);
  }
  return 0;
}