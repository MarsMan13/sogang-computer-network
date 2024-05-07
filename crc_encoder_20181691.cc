#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int _INIT_BUFFER_SIZE = 1024;

typedef struct _globalInfo {
  int index = 0;        //size == index
  int outputBufferSize;
  char *outputBuffer;
} globalInfo;

globalInfo *db;

int _initGlobalInfo(){
  db = (globalInfo*)calloc(1, sizeof(globalInfo));
  db->outputBufferSize = _INIT_BUFFER_SIZE;
  db->outputBuffer = (char*)calloc(db->outputBufferSize, sizeof(char));
  return 0;
}

int _clearGlobalInfo(){
  free(db->outputBuffer); free(db);
  return 0;
}

int _doubleOutputBuffer(){
  if(db->outputBuffer == NULL){
    _initGlobalInfo();
    return 1;
  }
  char*newOutputBuffer = (char*)calloc(db->outputBufferSize * 2, sizeof(char));
  memcpy(newOutputBuffer, db->outputBuffer, db->outputBufferSize);
  free(db->outputBuffer);
  db->outputBuffer = newOutputBuffer;
  db->outputBufferSize *= 2;
  return 0;
}

int _appendCodeword(char* codeword){
  if(db->outputBufferSize < db->index * 2){
    _doubleOutputBuffer();
  }
  int total_len = strlen(codeword);
  memcpy(db->outputBuffer + db->index, codeword, total_len);
  db->index += total_len;
  return 0;
}

int calculateCRC(char *dataword, char *generator){
  int dataword_len = strlen(dataword);
  int generator_len = strlen(generator);
  int total_len = dataword_len + generator_len - 1;
  char temp[total_len + 1];

  strcpy(temp, dataword);
  memset(temp + dataword_len, '0', generator_len - 1);
  temp[total_len] = '\0';
  for (int i = 0; i < dataword_len; i++)
  {
    if (temp[i] == '1')
    {
      for (int j = 0; j < generator_len; j++)
      {
        temp[i + j] = ((temp[i + j] - '0') ^ (generator[j] - '0')) + '0';
      }
    }
  }
  memcpy(temp, dataword, dataword_len);
  _appendCodeword(temp);
  return 0;
}

int8_t array8ToByte(char *pointer){
  int8_t byte = 0;
  for(int i = 0; i < 8; i++){
    byte += ((pointer[i] - '0') << (7-i));
  }
  return byte;
}

void processFile(char *input_file, char *output_file, char *generator, int dataword_size)
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
  _initGlobalInfo();
  // END INIT ========================
  int byte;
  int total_bits = 0;
  char byteAry[9] = {0};
  while ((byte = fgetc(in)) != EOF){
    if(byte == 10)  break;   // TODO: DELETE IT LATER @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    for (int i = 7; i >= 0; i--)
    {
      int bit = (byte >> i) & 1;
      byteAry[7 - i] = '0' + bit;
    }
    // END OF GET BYTE
    //////
    if(dataword_size == 4){
      char dataword[5] = {0};
      for(int j = 0; j<4; j++){
        dataword[j] = byteAry[j];
      }
      calculateCRC(dataword, generator);
      for(int j = 4; j<8; j++){
        dataword[j-4] = byteAry[j];
      }
      calculateCRC(dataword, generator);
    }
    if(dataword_size == 8){
      char dataword[9] = {0};
      for(int j = 0; j<8; j++){
        dataword[j] = byteAry[j];
      }
      calculateCRC(dataword, generator);
    }
  }
  for(int i = 0; db->outputBuffer[i] != 0; i++){
    printf("%c", db->outputBuffer[i]);
  }
  printf("\n");
  // Add Padding
  int padding = (8 - (db->index % 8)) % 8;
  int8_t paddingSizeByte = padding;
  memcpy(db->outputBuffer+padding, db->outputBuffer, db->outputBufferSize);
  memset(db->outputBuffer, '0', padding);
  printf("%s\n", db->outputBuffer);
  //////
  // OUTPUT
  printf("OUTPUT\n");
  fwrite(&paddingSizeByte, sizeof(int8_t), 1, out);
  for(int i = 0; i<db->index; i+=8){
    int8_t temp = array8ToByte(db->outputBuffer + i);
    printf("%d\n", temp);
    fwrite(&temp, sizeof(int8_t), 1, out);
  }
  ////
  _clearGlobalInfo();
  fclose(in);
  fclose(out);
}

int main(int argc, char *argv[])
{
  if (argc != 5)
  {
    printf("usage: ./crc_encoder input_file output_file generator dataword_size\n");
    return 1;
  }

  char *input_file = argv[1];        // datastream.tx
  char *output_file = argv[2];       // codedstream.tx
  char *generator = argv[3];         // 1101
  int dataword_size = atoi(argv[4]); // 4 or 8

  if (dataword_size != 4 && dataword_size != 8)
  {
    printf("dataword size must be 4 or 8.\n");
    return 1;
  }

  processFile(input_file, output_file, generator, dataword_size);
  return 0;
}
