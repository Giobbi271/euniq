#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t interrupted = 0;

static void handleSigint(int sig) {
  (void)sig;
  interrupted = 1;
}

static size_t nextPrime(size_t n);
static size_t hash(const char *v, size_t M);
static char **resizeHash(char **table, size_t *hashSize);
static size_t probing(size_t i, size_t M);
static void freeAll(char **table, size_t hashSize, char *buf, FILE *rp,
                    FILE *wp);
static void printUsage(FILE *out, const char *progName);

int main(int argc, char *argv[]) {
  if (argc == 2 &&
      (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    printUsage(stdout, argv[0]);
    return 0;
  }

  if (argc > 3) {
    printUsage(stderr, argv[0]);
    return 1;
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handleSigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; // Leave SA_RESTART unset so SIGINT interrupts blocking I/O
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  FILE *rp = stdin;
  FILE *wp = stdout;

  if (argc > 1) {
    rp = fopen(argv[1], "r");
    if (rp == NULL) {
      perror(argv[1]);
      return 1;
    }
  }

  if (argc == 3) {
    wp = fopen(argv[2], "w");
    if (wp == NULL) {
      perror(argv[2]);
      if (rp != stdin)
        fclose(rp);
      return 1;
    }
  }

  char *buf = NULL;
  size_t maxN = 64;
  size_t fullCnt = 0;
  double fillHash = 0.5; // magic number for fill ratio
  size_t hashSize = nextPrime(maxN);
  size_t size = 0;
  ssize_t nRead;

  char **table = calloc(hashSize, sizeof(char *));
  if (table == NULL) {
    perror("calloc");
    freeAll(NULL, 0, NULL, rp, wp);
    return 1;
  }

  while (!interrupted && (nRead = getline(&buf, &size, rp)) != -1) {
    buf[strcspn(buf, "\n")] = '\0';

    size_t i = hash(buf, hashSize);
    while (table[i] != NULL && strcmp(table[i], buf) != 0)
      i = probing(i, hashSize);

    if (table[i] == NULL) {
      char *copy = strdup(buf);
      if (copy == NULL) {
        perror("strdup");
        freeAll(table, hashSize, buf, rp, wp);
        return 1;
      }
      table[i] = copy;
      fprintf(wp, "%s\n", buf);
      fullCnt++;
    }

    if ((double)fullCnt / (double)hashSize > fillHash) {
      char **newTable = resizeHash(table, &hashSize);
      if (newTable == NULL) {
        perror("resizeHash");
        freeAll(table, hashSize, buf, rp, wp);
        return 1;
      }
      table = newTable;
    }
  }

  if (!interrupted && ferror(rp)) {
    perror("getline");
    freeAll(table, hashSize, buf, rp, wp);
    return 1;
  }

  freeAll(table, hashSize, buf, rp, wp);

  if (interrupted)
    return 130; // Standard exit code for termination by SIGINT (128 + 2)

  return 0;
}

static char **resizeHash(char **table, size_t *hashSize) {
  size_t newSize = nextPrime(*hashSize * 2);
  char **newTable = calloc(newSize, sizeof(char *));
  if (newTable == NULL)
    return NULL;

  for (size_t i = 0; i < *hashSize; i++) {
    if (table[i] != NULL) {
      size_t j = hash(table[i], newSize);
      while (newTable[j] != NULL)
        j = probing(j, newSize);
      newTable[j] = table[i];
    }
  }
  free(table);
  *hashSize = newSize;
  return newTable;
}

// Horner's method for polynomial rolling hash
static size_t hash(const char *v, size_t M) {
  size_t h = 0;
  const size_t base = 127;
  for (; *v != '\0'; v++)
    h = (base * h + (unsigned char)*v) % M;
  return h;
}

static size_t nextPrime(size_t n) {
  if (n < 2)
    return 2;
  while (1) {
    size_t d;
    for (d = 2; d * d <= n; d++) {
      if (n % d == 0)
        break;
    }
    if (d * d > n)
      return n;
    n++;
  }
}
// linear probing
static size_t probing(size_t i, size_t M) { return (i + 1) % M; }

static void freeAll(char **table, size_t hashSize, char *buf, FILE *rp,
                    FILE *wp) {
  if (table != NULL) {
    for (size_t i = 0; i < hashSize; i++)
      free(table[i]);
    free(table);
  }
  free(buf);
  if (rp != NULL && rp != stdin)
    fclose(rp);
  if (wp != NULL && wp != stdout)
    fclose(wp);
}

static void printUsage(FILE *out, const char *progName) {
  fprintf(out, "Usage: %s [INPUT [OUTPUT]]\n", progName);
  fprintf(out, "Filter out duplicate lines across the entire input without "
               "requiring sorting.\n");
}
