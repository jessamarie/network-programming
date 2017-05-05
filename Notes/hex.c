#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG
#define BUFFER_SIZE 1024



/**
 * get_MD5 returns the MD5 checksum of a file
 * 
 * @param filename the file to be hashed
 * @return the hash of the file
*/
unsigned char * get_MD5(char * filename) {

    FILE * file = fopen(filename, "rb");
    
    MD5_CTX context;
    
    unsigned char * hash = (unsigned char *) malloc (MD5_DIGEST_LENGTH * sizeof(unsigned char));
    
    int bytes;
    
    unsigned char data[1024];

    MD5_Init (&context);
    
    while ((bytes = fread (data, sizeof(char), BUFFER_SIZE, file)) != 0)
        MD5_Update (&context, data, bytes);
        
    MD5_Final (hash,&context);

#ifdef DEBUG_POPULATE
    int i;

    for(i = 0; i < MD5_DIGEST_LENGTH; i++) 
    	printf("%02x", hash[i]);
    printf (" %s\n", filename);
#endif
    
    fclose (file);
    
    return hash;
	
}


int compare_hashes(unsigned char * hash1, unsigned char * hash2) {
      
      
      int isEqual = memcmp(hash1, hash2, MD5_DIGEST_LENGTH);
      
      if (isEqual == 0) {
            printf("They are equal!\n");
      } else {
            printf("They are not equal :( \n");
      }

      return isEqual;
}


void print_line(unsigned char * hash, char * filename) {
      
      int i;

      for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
            printf("%02x", hash[i]);
            fflush (stdout);
      }
          
      printf (" %s\n", filename);
      fflush (stdout);
}

void put_line(FILE * file, unsigned char * hash, char * filename) {
      
      printf("Writing %s: ", filename);
     
     int i;
     
      for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
            
            char tmp[2];
            tmp[0] = '\0';
            tmp[1] = '\0';
            
            sprintf(tmp, "%x", hash[i]);
            
            char c = tmp[0];
            int k = 0;
            while ( k < 2 && c != '\0' ) {
                  printf("%c", c);
                  fputc(c, file); k++; 
                  c = tmp[k];
            }
            
      }
      
      fwrite("    ", 4, 1, file); 
      
      for(i = 0; i < strlen(filename); i++)
            fputc(filename[i], file);
            
      fputc('\n', file);
      printf("\n");
}

void make_file(char ** filenames) {
      
      int k = 0;
      
      printf("Writing to the file...\n");
            
      FILE * file = fopen("newfile.txt", "w+");
      
      while (k < 2 ) {
      
            unsigned char * hash = get_MD5(filenames[k]);
            
            print_line(hash, filenames[k]);
            
            put_line(file, hash, filenames[k]);
            
            k++;
      }

      
      fclose(file);
}

void get_line(FILE * fd, char * hash, char * filename) {
      char c;
      unsigned char u;
      int i;
      
      printf("Reading %s: ", filename);

      
      for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
            
            u = fgetc(fd);
            
            hash[i] = u;

           printf("%c", u);

      }
      
      hash[i] = '\0';
      
      for(i = 0; i < 4; i++) {
            c = fgetc(fd);
            printf("%c", c);
      }
      
      for(i = 0; i < 255; i++) {
            if ((c = fgetc(fd)) == '\n') {
                 printf("%c", c);
                  break;   
            }
                
            printf("%c", c);
      }
      
      int isEqual = memcmp(hash,hash, MD5_DIGEST_LENGTH);
      
      if (isEqual == 0) {
            printf("They are equal!\n");
      } else {
            printf("They are not equal :( \n");
      }
}

void read_file() {
      
      printf("Reading from file...\n");
      
      FILE * fd = fopen("newfile.txt", "r+");

      char hash[MD5_DIGEST_LENGTH];
      char filen[255];
      
      get_line(fd, hash, filen);
      get_line(fd, hash, filen);


      fclose(fd);
      
      

}

int main() {
      
      char ** filenames = (char **) calloc (2, sizeof(char *));
      
      filenames[0] = (char *) malloc (20);
      sprintf(filenames[0], "mouse.txt");
      filenames[1] = (char *) malloc (20);
      sprintf(filenames[1], "legend.txt");


      unsigned char * hex1 = get_MD5("mouse.txt");
      unsigned char * hex2 = get_MD5("legend.txt");

      print_line(hex1, "mouse.txt");
      print_line(hex2, "legend.txt");
      
      make_file(filenames);
      read_file();
      
     // compare_hashes(hex1, hex2); // not equal
     // compare_hashes(hex1, hex1); // equal

      

      return EXIT_SUCCESS;
}