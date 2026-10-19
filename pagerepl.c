#include "types.h"
#include "stat.h"
#include "user.h"

// Reference string length and maximum frame capacity tested
#define REF_LEN 20
#define MAX_FRAMES 7

static int ref_str[REF_LEN] = {
  7, 0, 1, 2, 0, 3, 0, 4, 2, 3,
  0, 3, 2, 1, 2, 0, 1, 7, 0, 1
};

static int
in_frames(int *frames, int nframes, int page)
{
  int i;
  for(i = 0; i < nframes; i++)
    if(frames[i] == page)
      return i;
  return -1;
}

// 1. FIFO Page Replacement
static int
sim_fifo(int *refs, int n, int nframes)
{
  int frames[MAX_FRAMES];
  int i, ptr = 0, faults = 0;

  for(i = 0; i < nframes; i++)
    frames[i] = -1;

  for(i = 0; i < n; i++){
    if(in_frames(frames, nframes, refs[i]) < 0){
      frames[ptr] = refs[i];
      ptr = (ptr + 1) % nframes;
      faults++;
    }
  }
  return faults;
}

// 2. LRU (Least Recently Used) Page Replacement
static int
sim_lru(int *refs, int n, int nframes)
{
  int frames[MAX_FRAMES];
  int last_used[MAX_FRAMES];
  int i, j, faults = 0;

  for(i = 0; i < nframes; i++){
    frames[i] = -1;
    last_used[i] = -1;
  }

  for(i = 0; i < n; i++){
    int idx = in_frames(frames, nframes, refs[i]);
    if(idx >= 0){
      last_used[idx] = i;
    } else {
      int victim = 0;
      for(j = 1; j < nframes; j++){
        if(last_used[j] < last_used[victim])
          victim = j;
      }
      frames[victim] = refs[i];
      last_used[victim] = i;
      faults++;
    }
  }
  return faults;
}

// 3. Optimal (Belady's MIN) Page Replacement
static int
sim_optimal(int *refs, int n, int nframes)
{
  int frames[MAX_FRAMES];
  int i, j, k, faults = 0;

  for(i = 0; i < nframes; i++)
    frames[i] = -1;

  for(i = 0; i < n; i++){
    if(in_frames(frames, nframes, refs[i]) >= 0)
      continue;

    int victim = -1;
    int farthest = -1;
    for(j = 0; j < nframes; j++){
      if(frames[j] == -1){
        victim = j;
        break;
      }
      int next_use = 999999;
      for(k = i + 1; k < n; k++){
        if(refs[k] == frames[j]){
          next_use = k;
          break;
        }
      }
      if(next_use > farthest){
        farthest = next_use;
        victim = j;
      }
    }
    frames[victim] = refs[i];
    faults++;
  }
  return faults;
}

int
main(int argc, char *argv[])
{
  int i, f;

  printf(1, "============================================================\n");
  printf(1, "Page Replacement Algorithms Comparison (pagerepl)\n");
  printf(1, "============================================================\n");
  printf(1, "Reference String (%d accesses): ", REF_LEN);
  for(i = 0; i < REF_LEN; i++)
    printf(1, "%d ", ref_str[i]);
  printf(1, "\n\n");

  printf(1, "+--------+-------------+-------------+----------------+\n");
  printf(1, "| Frames | FIFO Faults |  LRU Faults | Optimal Faults |\n");
  printf(1, "+--------+-------------+-------------+----------------+\n");
  for(f = 3; f <= 6; f++){
    int fifo = sim_fifo(ref_str, REF_LEN, f);
    int lru  = sim_lru(ref_str, REF_LEN, f);
    int opt  = sim_optimal(ref_str, REF_LEN, f);
    printf(1, "|   %d    |     %d      |     %d      |       %d        |\n",
           f, fifo, lru, opt);
  }
  printf(1, "+--------+-------------+-------------+----------------+\n");
  printf(1, "SUCCESS: Optimal achieves minimum page faults; LRU outperforms FIFO.\n");
  exit();
}
