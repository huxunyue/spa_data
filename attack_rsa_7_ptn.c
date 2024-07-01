#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h> 
#include <string.h>
#include <assert.h>

#define NB_SAMPLES 557000000
#define NUM_TRACE 1
#define LEN_PATTERN 13711
#define LEN_BASE 7  // 7 limbs

const char* path = "traces.raw";
const char* pattern_path = "pattern.raw";

float * traces[NUM_TRACE]; // to be adapted if needed


#define min(x, y) ((x) < (y) ? (x) : (y))


int parsing(int lst[4096][17], float pattern[LEN_PATTERN],float trace[NB_SAMPLES]){
  // int fp;
  int c = 0;
  int n_limb =0;
  // int missing = 0;
  for (int i = 0; i < (NB_SAMPLES - LEN_PATTERN);  i++) {
    double diff = 0;
    for (int j = 0; j < LEN_PATTERN;  j++) {
      diff += fabs(pattern[j] - trace[i+j]);
    }
    if( (diff/LEN_PATTERN) < 0.01){  //0.012550 

      // printf("diff: %f\n", diff/LEN_PATTERN);
      assert(c < 4096);
      lst[c][0] = i + 30;
      if( (c > 0) && (lst[c][0]-lst[c-1][0]) > 250000){
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
    }
  }

  // double diff = 0;
  // for (int j = 0; j < LEN_PATTERN;  j++) {
  //   diff += fabs(pattern[j] - trace[55605234-30+j]);
  // }
  // printf("diff: %f\n", diff/LEN_PATTERN);

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

int main(int argc, char const *argv[]) {
  int poi[4096][17] = {0};
  int key[1024] = {0};
  int key_found[2048] = {0};
  int num_bit = 0;
  float pattern[LEN_PATTERN];
  int fd;
  
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
      uint64_t nb_read_samples = min(step, offset + step - NB_SAMPLES);
      
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
    if (read(fd, pattern, sizeof(float) * LEN_PATTERN) == -1) {
      perror("read");
      return -1;
    }
  }

  float *trace = traces[0];

  int nb = parsing(poi, pattern, trace);
  // for (int i = 0; i < nb-1; i ++) {
  //   printf("%d, ", poi[i][0]);
  // }
  // printf("%d\n", poi[nb-1][0]);

  // exit(0);
  int c = 0;
  int index = 0;
  for (int i = 0; i < nb; i ++) {
    if((trace[poi[i][16]] > -0.02) && (trace[poi[i][16]] != 0)){
      for (int j = 0; j < 16; j ++){
        if(trace[poi[i][j]] < -0.03){
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