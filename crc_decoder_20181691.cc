#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int _INIT_BUFFER_SIZE = 1024;

typedef struct _globalInfo
{
  int index = 0; // size == index
  int codewordsSize;
  char *codewords;
} globalInfo;

globalInfo *db;

int _initGlobalInfo()
{
  db = (globalInfo *)calloc(1, sizeof(globalInfo));
  db->codewordsSize = _INIT_BUFFER_SIZE;
  db->codewords = (char *)calloc(db->codewordsSize + 1, sizeof(char));
  return 0;
}

int _clearGlobalInfo()
{
  free(db->codewords);
  free(db);
  return 0;
}

int _extendOutputBuffer()
{
  if (db->codewords == NULL)
  {
    _initGlobalInfo();
    return 1;
  }
  char *newCodewords = (char *)calloc(db->codewordsSize + _INIT_BUFFER_SIZE + 1, sizeof(char));
  memcpy(newCodewords, db->codewords, db->codewordsSize);
  free(db->codewords);
  db->codewords = newCodewords;
  db->codewordsSize += _INIT_BUFFER_SIZE;
  return 0;
}

int byteToArray8(int8_t byte, char *pointer)
{
  for (int i = 7; i >= 0; i--)
  {
    int bit = (byte >> i) & 1;
    pointer[7 - i] = '0' + bit;
  }
  return 0;
}

int _readCodewords(FILE *in)
{
  int8_t padding;
  int8_t byte;
  fread(&padding, 1, sizeof(padding), in);
  // printf("Padding: %d\n", padding);
  while (fread(&byte, 1, sizeof(byte), in) > 0)
  {
    // printf("byte: %d\n", byte);
    if (db->codewordsSize <= db->index + 10)
    {
      _extendOutputBuffer();
    }
    byteToArray8(byte, db->codewords + db->index);
    db->index += 8;
  }
  db->index = db->index - padding;
  memcpy(db->codewords, db->codewords + padding, db->index);
  db->codewords[db->index] = 0;
  // for (int i = 0; i < db->index; i++)
  // {
    // printf("%c", db->codewords[i]);
  // }
  // printf("\n");
  return 0;
}

int checkCRC(int codewordSize, char *codeword, char *generator)
{
  char *temp = (char *)calloc(codewordSize + 1, sizeof(char));
  int generatorSize = strlen(generator);
  strcpy(temp, codeword);
  for (int i = 0; i < codewordSize - (generatorSize - 1); i++)
  {
    if (temp[i] == '1')
    {
      for (int j = 0; j < generatorSize; j++)
      {
        temp[i + j] = ((temp[i + j] - '0') ^ (generator[j] - '0')) + '0';
      }
    }
  }

  for (int i = 0; i < generatorSize - 1; i++)
  {
    if (temp[codewordSize - (generatorSize - 1) + i] != '0')
    {
      return 1; // Error detected
    }
  }
  return 0; // No error detected
}

int8_t array8ToByte(char *pointer)
{
  int8_t byte = 0;
  for (int i = 0; i < 8; i++)
  {
    byte += ((pointer[i] - '0') << (7 - i));
  }
  return byte;
}

void decodeFile(char *input_file, char *output_file, char *result_file, char *generator, int dataword_size)
{
  FILE *in = fopen(input_file, "rb");
  if (!in)
  {
    printf("input file open error.\n");
    exit(1);
  }

  FILE *out = fopen(output_file, "wb");
  if (!out)
  {
    printf("output file open error.\n");
    fclose(in);
    exit(1);
  }

  FILE *res = fopen(result_file, "w");
  if (!res)
  {
    printf("result file open error.\n");
    fclose(in);
    fclose(out);
    exit(1);
  }
  _initGlobalInfo();
  _readCodewords(in);
  // END OF INIT
  char byteAry[9] = {0};
  int8_t byte;
  int numFrames = 0;
  int numErrors = 0;
  int codewordSize = dataword_size + strlen(generator) - 1;
  // printf("codewordSize: %d\n", codewordSize);
  char *codeword = (char *)calloc(codewordSize + 1, sizeof(char));

  if (dataword_size == 8)
  {
    for (int i = 0; i < db->index; i += codewordSize)
    {
      memcpy(codeword, db->codewords + i, codewordSize);
      numFrames++;
      numErrors += checkCRC(codewordSize, codeword, generator);
      //
      memcpy(byteAry, codeword, dataword_size);
      byte = array8ToByte(byteAry);
      fwrite(&byte, sizeof(int8_t), 1, out);
    }
  }
  else if (dataword_size == 4)
  {
    int flag = 0;
    for (int i = 0; i < db->index; i += codewordSize, flag++)
    { flag %= 2;
      memcpy(codeword, db->codewords + i, codewordSize);
      numFrames++;
      numErrors += checkCRC(codewordSize, codeword, generator);
      //
      memcpy(byteAry + 4 * flag, codeword, dataword_size);
      if(flag){
        byte = array8ToByte(byteAry);
        fwrite(&byte, sizeof(int8_t), 1, out);
      }
    }
  }
  fprintf(res, "%d %d", numFrames, numErrors);
  free(codeword);
  // Dummy implementation of decoding logic for illustration
  _clearGlobalInfo();
  fclose(in);
  fclose(out);
  fclose(res);
}

int main(int argc, char *argv[])
{
  if (argc != 6)
  {
    printf("usage: ./crc_decoder input_file output_file result_file generator dataword_size\n");
    return 1;
  }

  char *input_file = argv[1];
  char *output_file = argv[2];
  char *result_file = argv[3];
  char *generator = argv[4];
  int dataword_size = atoi(argv[5]);

  if (dataword_size != 4 && dataword_size != 8)
  {
    printf("dataword size must be 4 or 8.\n");
    return 1;
  }

  decodeFile(input_file, output_file, result_file, generator, dataword_size);
  return 0;
}