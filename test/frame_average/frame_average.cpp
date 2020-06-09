#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <vector>

#include <signal.h>

void sig_exit(int s) { exit(0); }

void MultiFrameAverage(uint16_t *pIn1_pOut, uint16_t width, uint16_t height,
                       uint8_t frameNumber) {
  uint16_t n;
  uint16_t roundOffset = 0;
  switch (frameNumber) {
  case 16:
    n = 4;
    break;
  case 8:
    n = 3;
    break;
  case 4:
    n = 2;
    break;
  case 2:
    n = 1;
    break;
  default:
    printf("frame number error! %d", frameNumber);
    return;
    break;
  }
  roundOffset = pow(2, n - 1);
  int w, h, m;
  for (h = 0; h < height; h++) {
    m = h * width;
    for (w = 0; w < width; w++) {
      pIn1_pOut[m + w] += roundOffset;
      pIn1_pOut[m + w] = pIn1_pOut[m + w] >> n;
    }
  }
}

void MultiFrameAddition(uint16_t *pIn1_pOut, uint16_t *pIn2, uint16_t width,
                        uint16_t height) {
  int w, h, n;
  for (h = 0; h < height; h++) {
    n = h * width;
    for (w = 0; w < width; w++) {
      pIn1_pOut[n + w] = (pIn1_pOut[n + w] + pIn2[n + w]);
    }
  }
}

void ProcessMultiFrame(int fd_in, int width, int height, int frame_count,
                       int frame_size, int index, uint16_t *pOut0,
                       uint16_t *pOut1, uint16_t *pIn) {
  fprintf(stderr, "index      %d\n", index);
  if (index == 0) {
    read(fd_in, pOut0, frame_size);
  } else if (index > 0 && index < (frame_count >> 1)) {
    read(fd_in, pIn, frame_size);
    MultiFrameAddition(pOut0, pIn, width, height);
  } else if (index == (frame_count >> 1)) {
    read(fd_in, pOut1, frame_size);
  } else if (index > (frame_count >> 1) && index < frame_count) {
    read(fd_in, pIn, frame_size);
    MultiFrameAddition(pOut1, pIn, width, height);
  }

  if (index == ((frame_count >> 1) - 1)) {
    MultiFrameAverage(pOut0, width, height, (frame_count >> 1));
  } else if (index == (frame_count - 1)) {
    MultiFrameAverage(pOut1, width, height, (frame_count >> 1));
  }

  if (index == (frame_count - 1)) {
    MultiFrameAddition(pOut0, pOut1, width, height);
    MultiFrameAverage(pOut0, width, height, 2);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 6) {
    fprintf(stderr, "Usage: ./%s input_file frame_count output_file w h\n",
            argv[0]);
    return 0;
  }

  std::string input_file = argv[1];
  int frame_count = atoi(argv[2]);
  std::string output_file = argv[3];
  int width = atoi(argv[4]);
  int height = atoi(argv[5]);

  fprintf(stderr, "input_file  %s\n", input_file.c_str());
  fprintf(stderr, "frame_count %d\n", frame_count);
  fprintf(stderr, "output_file %s\n", output_file.c_str());
  fprintf(stderr, "width       %d\n", width);
  fprintf(stderr, "height      %d\n", height);

  signal(SIGINT, sig_exit);

  int fd_in = open(input_file.c_str(), O_RDONLY);
  int fd_out = open(output_file.c_str(), O_RDWR | O_CREAT | O_SYNC);

  int one_frame_size = width * height * 2;
  uint16_t *one_frame = (uint16_t *)malloc(one_frame_size);
  memset(one_frame, 0, one_frame_size);
  uint16_t *averge_frame0 = (uint16_t *)malloc(one_frame_size);
  memset(averge_frame0, 0, one_frame_size);
  uint16_t *averge_frame1 = (uint16_t *)malloc(one_frame_size);
  memset(averge_frame1, 0, one_frame_size);

#if 1
  for (int i = 0; i < frame_count; i++) {
    ProcessMultiFrame(fd_in, width, height, frame_count, one_frame_size, i,
                      averge_frame0, averge_frame1, one_frame);
  }
#else
  // get frame 0
  read(fd_in, averge_frame0, one_frame_size);
  // get frame 1 ~ frame (n/2  - 1)
  for (int i = 1; i < (frame_count >> 1); i++) {
    read(fd_in, one_frame, one_frame_size);
    MultiFrameAddition(averge_frame0, one_frame, width, height);
  }
  MultiFrameAverage(averge_frame0, width, height, (frame_count >> 1));

  // get frame (n/2)
  read(fd_in, averge_frame1, one_frame_size);
  // get frame (n/2  + 1) ~ frame (n  - 1)
  for (int i = 1; i < (frame_count >> 1); i++) {
    read(fd_in, one_frame, one_frame_size);
    MultiFrameAddition(averge_frame1, one_frame, width, height);
  }
  MultiFrameAverage(averge_frame1, width, height, (frame_count >> 1));

  MultiFrameAddition(averge_frame0, averge_frame1, width, height);
  MultiFrameAverage(averge_frame0, width, height, 2);
#endif

  write(fd_out, averge_frame0, one_frame_size);

  free(one_frame);
  free(averge_frame0);
  free(averge_frame1);
  close(fd_out);
  close(fd_in);
  sync();

  return 0;
}
