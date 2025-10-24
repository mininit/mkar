#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define VERSION "development"
#define HEADER_LENGTH 1024
#define PATH_LENGTH 256
#define FILE_SIZE_LENGTH 8 // int64_t
#define PADDING (HEADER_LENGTH - PATH_LENGTH - FILE_SIZE_LENGTH)

void writedata(FILE *destfile, const void *data, size_t bytes)
{
  if (!destfile || !data || bytes == 0)
    return;
  fwrite(data, 1, bytes, destfile);
}

void writefilename(FILE *destfile, const char *filename)
{
  char buffer[PATH_LENGTH] = {0};
  strncpy(buffer, filename, sizeof(buffer) - 1); // leave null termination byte
  writedata(destfile, buffer, sizeof(buffer));
}

void writefilesize(FILE *destfile, int64_t filesize)
{
  int64_t sz = filesize;
  writedata(destfile, &sz, sizeof(sz));
}

// Append a file to destfile
void appendfile(const char *sourcefilename, FILE *destfile)
{
  if (access(sourcefilename, R_OK) != 0)
    return;

  FILE *archive = fopen(sourcefilename, "rb");
  if (!archive)
    return;

  struct stat st;
  if (stat(sourcefilename, &st) != 0)
  {
    return;
  }

  if (S_ISDIR(st.st_mode))
  {
    fprintf(stderr, "%s is a directory. Skipping...\n", sourcefilename);
    return;
  }

  writefilename(destfile, sourcefilename);

  writefilesize(destfile, (int64_t)st.st_size);

  char padding[PADDING] = {0};
  writedata(destfile, padding, sizeof(padding));

  char buffer[8192];
  size_t n;
  while ((n = fread(buffer, 1, sizeof(buffer), archive)) > 0)
  {
    writedata(destfile, buffer, n);
  }

  fclose(archive);
}

void createarchive(int numfiles, const char *files[])
{
  const char *destfilename = files[1];

  FILE *destfile;
  destfile = fopen(destfilename, "wb");

  for (size_t i = 2; i < numfiles; i++)
  {
    if (strcmp(destfilename, files[i]) != 0)
    {
      appendfile(files[i], destfile);
    }
    else
    {
      fprintf(stderr, "Can't add archive to itself. Skipping...\n");
    }
  }

  fclose(destfile);
}

void usage()
{
  printf("usage: mkar <archive> <files...>\n            -x <archive>\n            [-h | --help] [-v | --version]\n");
}

int extractarchive(const char *archivefilename)
{
  FILE *archive = fopen(archivefilename, "rb");
  if (!archive)
  {
    perror("mkar:");
    return 1;
  }

  while (1)
  {
    char filename[PATH_LENGTH];
    int64_t filesize;
    char padding[PADDING];

    // Try to read filename (EOF = end of archive)
    size_t n = fread(filename, 1, sizeof(filename), archive);
    if (n == 0)
      break; // Reached EOF cleanly
    if (n != sizeof(filename))
    {
      fprintf(stderr, "mkar: corrupt archive: incomplete header filename\n");
      break;
    }

    filename[sizeof(filename) - 1] = '\0';

    // Read filesize (exactly one 8-byte int64_t)
    if (fread(&filesize, sizeof(filesize), 1, archive) != 1)
    {
      fprintf(stderr, "mkar: corrupt archive: missing filesize\n");
      break;
    }

    // Read padding to align to 1024 bytes
    if (fread(padding, 1, sizeof(padding), archive) != sizeof(padding))
    {
      fprintf(stderr, "mkar: corrupt archive: truncated file\n");
      break;
    }

    // Validate filesize before allocation
    if (filesize < 0 || filesize > (1LL << 32))
    {
      fprintf(stderr, "mkar: corrupt archive: invalid filesize %lld\n", (long long)filesize);
      break;
    }

    // Allocate and read file contents safely
    char *data = malloc((size_t)filesize);
    if (!data)
    {
      perror("mkar:");
      break;
    }

    size_t read_bytes = fread(data, 1, (size_t)filesize, archive);
    if (read_bytes != (size_t)filesize)
    {
      fprintf(stderr, "mkar: corrupt archive: truncated file\n");
      free(data);
      break;
    }

    // Write output file
    FILE *out = fopen(filename, "wb");
    if (!out)
    {
      perror("mkar:");
      free(data);
      break;
    }

    fwrite(data, 1, (size_t)filesize, out);
    fclose(out);
    free(data);

    printf("Extracted: %s (%lld bytes)\n", filename, (long long)filesize);
  }

  fclose(archive);
  return 0;
}

int main(int argc, char const *argv[])
{
  if (argc > 2)
  {
    if (strcmp("-x", argv[1]) == 0 || strcmp("--extract", argv[1]) == 0)
    {
      printf("Extracting %s...\n", argv[2]);
      if (extractarchive(argv[2]) != 0)
      {
        return 1;
      }
    }
    else
    {
      createarchive(argc, argv);
    }
  }
  else
  {
    if (strcmp("-v", argv[1]) == 0 || strcmp("--version", argv[1]) == 0)
    {
      printf("mkar version %s\n", VERSION);
    }
    else if (strcmp("-h", argv[1]) == 0 || strcmp("--help", argv[1]) == 0)
    {
      usage();
    }
    else
    {
      usage();
      return 1;
    }
  }

  return 0;
}
