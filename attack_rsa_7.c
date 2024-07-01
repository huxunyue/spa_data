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

#define NB_SAMPLES 535000000
#define NUM_TRACE 1
#define LEN_PATTERN 227
#define LEN_BASE 7  // 7 limbs

const char* path = "traces.raw";
const char* pattern_path = "patterns7.raw";

float * traces[NUM_TRACE]; // to be adapted if needed
float patterns[2][LEN_PATTERN];

#define min(x, y) ((x) < (y) ? (x) : (y))


int parsing(int lst[4096][17], float pattern[LEN_PATTERN], float pattern0[LEN_PATTERN],float trace[NB_SAMPLES]){
  int c = 0;
  int n_limb =0;
  for (int i = 0; i < (NB_SAMPLES - LEN_PATTERN);  i++) {
    double diff1 = 0;
    double diff2 = 0;
    for (int j = 0; j < LEN_PATTERN;  j++) {
      diff1 += fabs(pattern[j] - trace[i+j]);
      diff2 += fabs(pattern0[j] - trace[i+j]);
    }
    if((diff2/LEN_PATTERN < 0.0068) || diff1/LEN_PATTERN < 0.0155){  //0.012550 
      assert(c < 4096);
      lst[c][0] = i + 30;
      if((lst[c][0]-lst[c-1][0])>250000){
        printf("[%d:%d]\n", lst[c-1][0], lst[c][0]);
      }
      //printf("%f, %f, ", trace[poi[i][0]], trace[poi[i][1]]);
      for (int j = 1; j < 16; j ++){
        n_limb = LEN_BASE*(1+(j-1)*2);  
        if(n_limb < 64){
          lst[c][j] = lst[c][j-1] + 89+13*n_limb;
        }else{
          lst[c][j] = lst[c][j-1] + 921;   // 921 = 89 + 13*64  // max_n_limb = 64
        }
        //printf("%f, ", trace[poi[i][j]]);
      }
      lst[c][16] = lst[c][15] + 916;
      //printf("%f\n", trace[poi[i][16]]);
      c++;
      i += 30000;
    }
  }

  // double diff0 = 0;
  // double diff1 = 0;
  // for (int j = 0; j < LEN_PATTERN;  j++) {
  //   diff1 += fabs(pattern[j] - trace[524593475+223335-30+j]);
  //   diff0 += fabs(pattern0[j] - trace[524593475+223335-30+j]);
  // }
  // printf("diff0: %f\n", diff0/LEN_PATTERN);
  // printf("diff1: %f\n", diff1/LEN_PATTERN);

  return c;
}

int print_bin(int b[5], int s){ // ecrit dans b[5] le binaire de s, retourne la taille de binaire
  int i=1;
  b[4] = 1;

  while(s!=0){
    i++;
    b[5-i] = s&1;
    s >>= 1;
  }

  return i;
}

int recover_key(int key_found[2048], int key[1024]){ // traduit les secrets trouvés en binaire et retourne le nombre de bits qu'on a trouvé
  key_found[0] = 1;
  int c = 1;
  for(int i=0; i<512; i++){
    c += key[i*2+1];
    int b[5] = {0};
    int len = print_bin(b,key[i*2]);
    for(int j=0; j<len; j++){
      key_found[c-1-j] = b[4-j];
    }
  }
  return c;
}

//float ^
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
  qsort(cons,LEN_PATTERN,sizeof(float),cmp);
  for(int j = 0; j < n-1; j++){
    if((cons[j+1] - cons[j]) > max_diff){
      max_diff = cons[j+1] - cons[j];
      tmp_threshold = (max_diff/2) + cons[j];
      }
  }
  (*threshold) = tmp_threshold;
}



int main(int argc, char const *argv[]) {
  clock_t begin,end;
  int poi[4096][17] = {0};
  int key[1024] = {0};
  int key_found[2048] = {0};
  int num_bit = 0;
  int lst[3000] = {0};
  float poi_threshold[17];

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

  int nb = parsing(poi, patterns[1], patterns[0], trace);

  for(int j = 0; j < 17; j++){
    for (int i = 0; i < nb; i ++) {
      lst[i] = poi[i][j];
    }
    find_poi_threshold(nb, lst, trace, &poi_threshold[j]);
    printf("threshold %d: %f\n", j,poi_threshold[j]);

  }

  for (int i = 0; i < nb-1; i ++) {
    printf("%d, ", poi[i][0]);
  }
  printf("%d\n", poi[nb-1][0]);
  

  // for (int i = 0; i < 40; i ++) {
  //   for (int j = 0; j < 16; j ++){
  //     printf("%f, ", trace[poi[i][j]]);
  //   }
  //   printf("%f\n", trace[poi[i][16]]);
  // }

  int c = 0;
  int index = 0;
  for (int i = 0; i < nb; i ++) {
    if((trace[poi[i][16]] > poi_threshold[16])){ //&& (trace[poi[i][16]] != 0)
      for (int j = 0; j < 16; j ++){
        if(trace[poi[i][j]] < poi_threshold[j]){
          // printf("%f\n", trace[poi[i][j]]);
          // printf("%d\n", c);
          key[index] = j;
          key[index+1] = c;
          index += 2;
          c=0;
          break;
        }
      }
    }else{
      c++;
    }
  }

  num_bit = recover_key(key_found,key);
  // printf("num_bit: %d\n", num_bit);

  end = clock();
  printf("Time: %lu s\n ",((end-begin)/CLOCKS_PER_SEC));

  printf("secret found:    ");
  for (int i = 0; i < 1023; i ++) {
    printf("%x, ",key[i]);
  }
  printf("%x\n",key[1023]);


  printf("key found:       0b");
  for (int i = 0; i < num_bit; i ++) {
    printf("%d",key_found[i]);
  }
  return 0;
}