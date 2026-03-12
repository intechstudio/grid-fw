#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>

#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>,
// which is included by <termios.h>
#define BAUDRATE B2000000

char* path = NULL;

void process_args(int argc, char** argv) {
  int idx = 0;
  while (idx < argc) {

    // Input file
    if (strcmp(argv[idx], "-i") == 0 && argc - idx > 1) {
      path = argv[idx + 1];
      ++idx;
    }

    ++idx;
  }
}

void setup_termios(struct termios* tio) {
  // BAUDRATE: bps rate (you could also use cfsetispeed or cfsetospeed
  // CRTSCTS: output hardware flow control (only used if the cable has
  //          all necessary lines
  // CS8: 8n1 (8bit, no parity, 1 stopbit)
  // CLOCAL: local connection, no modem control
  // CREAD: enable receiving characters
  tio->c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;

  // IGNPAR: ignore bytes with parity errors
  // ICRML: map CR to NL (otherwise a CR input on the other computer
  //        will not terminate input)
  // otherwise make device raw (no other input processing)
  tio->c_iflag = IGNPAR /*| ICRNL*/;

  // Raw output
  tio->c_oflag = 0;

  // ICANON: enable canonical input
  // ECHO: disable all echo functionality,
  // and don't send signals to calling program
  tio->c_lflag = 0;

  // Initialize all control characters
  // Default values can be found in /usr/include/termios.h,
  // and are given in the comments, but we don't need them here
  tio->c_cc[VINTR] = 0;    /* Ctrl-c */
  tio->c_cc[VQUIT] = 0;    /* Ctrl-\ */
  tio->c_cc[VERASE] = 0;   /* del */
  tio->c_cc[VKILL] = 0;    /* @ */
  tio->c_cc[VEOF] = 0;     /* Ctrl-d */
  tio->c_cc[VTIME] = 0;    /* inter-character timer unused */
  tio->c_cc[VMIN] = 1;     /* blocking read until 1 character arrives */
  tio->c_cc[VSWTC] = 0;    /* '\0' */
  tio->c_cc[VSTART] = 0;   /* Ctrl-q */
  tio->c_cc[VSTOP] = 0;    /* Ctrl-s */
  tio->c_cc[VSUSP] = 0;    /* Ctrl-z */
  tio->c_cc[VEOL] = 0;     /* '\0' */
  tio->c_cc[VREPRINT] = 0; /* Ctrl-r */
  tio->c_cc[VDISCARD] = 0; /* Ctrl-u */
  tio->c_cc[VWERASE] = 0;  /* Ctrl-w */
  tio->c_cc[VLNEXT] = 0;   /* Ctrl-v */
  tio->c_cc[VEOL2] = 0;    /* '\0' */
}

int fd = -1;
struct termios oldtio;

void terminate() {
  // Restore the old port settings
  int ret = tcsetattr(fd, TCSANOW, &oldtio);
  if (ret < 0) {
    fprintf(stderr, "tcsetattr() failed\n");
    exit(1);
  }

  if (!(fd < 0)) {
    close(fd);
  }

  exit(0);
}

void sigint_handler(int signum) { terminate(); }

bool byte_pattern(char* pattern, int size, char rx, int* pos) {
  assert(size > 0);
  assert(*pos >= 0);
  assert(*pos < size);

  if (pattern[*pos] == rx) {
    *pos += 1;
  } else {
    *pos = pattern[0] == rx;
  }

  return *pos == size;
}

#define SEQ_START "abadcafe"
#define SEQ_CLOSE "abadc0de"

int process_char(char rx) {
  static char* seqs[] = {SEQ_START, SEQ_CLOSE};
  static int lens[] = {sizeof(SEQ_START) - 1, sizeof(SEQ_CLOSE) - 1};

  assert(lens[0] <= lens[1]);

  static char buf[sizeof(SEQ_CLOSE) - 1];
  static int phase = 0;
  static int match = 0;

  // Sequence continues to match
  if (rx == seqs[phase][match]) {

    // Buffer undecided character
    buf[match++] = rx;
  }
  // Sequence stops matching
  else {

    // Flush buffered characters if necessary
    if (phase && match) {
      write(1, buf, match);
    }

    // Reset match length
    match = 0;

    // Sequence start matches
    if (rx == seqs[phase][match]) {

      // Buffer undecided character
      buf[match++] = rx;
    }
    // Sequence start does not match
    else {

      // Write current character if necessary
      if (phase) {
        write(1, &rx, 1);
      }
    }
  }

  // Upon full match
  if (match == lens[phase]) {

    // If the second sequence was matched, return nonzero status
    if (phase && match == lens[phase]) {
      return 1;
    }

    // Otherwise, start matching the next sequence
    ++phase;
    match = 0;
  }

  return 0;
}

enum {
  BUF_SIZE = 0x100,
};

int main(int argc, char** argv) {
  int ret;

  // Set signal handler for SIGINT
  signal(SIGINT, sigint_handler);

  // Process arguments
  process_args(argc, argv);

  // Open modem device for reading and writing and not as controlling tty,
  // because we don't want to get killed upon receiving CTRL-C.
  fd = open(path, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(path);
    exit(1);
  }

  // Save current serial port settings
  ret = tcgetattr(fd, &oldtio);
  if (ret < 0) {
    fprintf(stderr, "tcgetattr() failed\n");
    exit(1);
  }

  // Clear struct, setup new port settings
  struct termios newtio;
  memset(&newtio, 0, sizeof(struct termios));
  setup_termios(&newtio);

  // Clean the modem line
  ret = tcflush(fd, TCIFLUSH);
  if (ret < 0) {
    fprintf(stderr, "tcflush() failed\n");
    exit(1);
  }

  // Activate the settings for the port
  ret = tcsetattr(fd, TCSANOW, &newtio);
  if (ret < 0) {
    fprintf(stderr, "tcsetattr() failed\n");
    exit(1);
  }

  char buf[BUF_SIZE];
  while (true) {

    // Read up to as many bytes as the buffer can hold
    int res = read(fd, buf, BUF_SIZE);
    if (res < 0) {
      fprintf(stderr, "read() returned < 0\n");
      exit(1);
    }

    // Pass bytes to receiver, quit upon nonzero status
    for (int i = 0; i < res; ++i) {
      if (process_char(buf[i])) {
        terminate();
      }
    }
  }

  // Handle termination
  terminate();

  return 0;
}
