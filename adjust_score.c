#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500

#include "curdle.h"

/** \file adjust_score.c
 * \brief Functions for safely amending a player's score in the
 * `/var/lib/curdle/scores` file.
 *
 * Contains the \ref adjust_score_file function, plus supporting data
 * structures and functions used by that function.
 *
 * ### Known bugs
 *
 * \bug The \ref adjust_score_file function does not yet work.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// for off_t
#include <sys/types.h>

#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
/** Size of a field (name or score) in a line of the `scores`
  * file.
  */
const size_t FIELD_SIZE = FIELD_SIZE_;

/** Size of a line in the `scores` file.
  */
const size_t REC_SIZE   = REC_SIZE_;

//global error number for error messages.
int error_number = 0;

struct {
  int number;
  int line;
} error;

char*** error_message = NULL;

//actual
//#define FILEPATH  "/var/lib/curdle/scores"

//for testing
#define FILEPATH "/home/vagrant/Project/curdle/src/test.txt"
//good test files
//#define FILEPATH "/home/vagrant/Project/curdle/tests/test-files/good/file0"

//Minimum value score can have before it exceeds 10 bytes.
#define MIN_SCORE -999999999
#define MESSAGE_LEN 1000
#define SUCCESS 1
#define FAILURE 0
/** Initialize a \ref score_record struct.
  *
  * \param rec pointer to a \ref score_record struct to initialize.
  * \param name player name to be inserted into `rec`.
  * \param score player score to be inserted into `rec`.
  */
void score_record_init(struct score_record *rec, const char *name, int score) {
  // this function is needed to initialize a score_record,
  // because you can't *assign* to the name member -- it's an array.
  // so we must copy the name in.
  memset(rec->name, 0, FIELD_SIZE);
  strncpy(rec->name, name, FIELD_SIZE);
  rec->name[FIELD_SIZE-1] = '\0';
  rec->score = score;
}


/**
  * This function checks the return value of other functions
  * to see if they experienced an error.
  * If there was an error, memory for the error message is allocated
  * and it returns true, to signify an error has occured, and false
  * otherwise.
  * The overall purpose of the function is to reduce code clutter
  * and segment the code into more readable chunks.
  *
  * \param returnValue the value returned by the function to check.
  * \return true if an error in that function occured, false otherwise.
  */
bool hasError(int returnValue)
{
  if(returnValue == -1)
  {
    *error_message = malloc(sizeof(char));
    **error_message = malloc(sizeof(char)*MESSAGE_LEN);
    return true;
  } else {
    return false;
  }
}

/**
  * This function sets the error_message to the message specified.
  * This function assumes hasError has been called and has already 
  * allocated memory to the message pointer.
  * It makes sure all messages have the same format.
  */
void setMessage(const char* message, int lineNo)
{
  snprintf(**error_message, MESSAGE_LEN, 
      message, lineNo, strerror(errno));
}


/** Return the size of the open file with file descriptor `fd`.
  * If the size cannot be determined, the function may abort program
  * execution (optionally after printing an error message).
  *
  * `filename` is used for diagnostic purposes only, and may be `NULL`.
  * If non-NULL, it represent the name of the file path from which
  * `fd` was obtained.
  *
  * \param filename name of the file path from which `fd` was obtained.
  * \param fd file descriptor for an open file.
  * \return the size of the file described by `fd`.
  */
size_t file_size(const char * filename, int fd) {
  struct stat file_info;
  int error;

  error = fstat(fd,&file_info);

  if(error)
  {
      setMessage("%d file_size: Failed to determine the size of the score file. %s\n", 
              __LINE__);
      exit(EXIT_FAILURE);
  } else {
      return file_info.st_size;
  }
}


/** Parse a \ref score_record struct from an
  * array of size \ref REC_SIZE.
  *
  * If a name and score cannot be found in `rec_buf`,
  * the function may abort program
  * execution (optionally after printing an error message).
  *
  * \param rec_buf an array of size \ref REC_SIZE.
  * \return a parsed \ref score_record.
  */

struct score_record parse_record(char rec_buf[REC_SIZE]) {
  struct score_record rec;
  char* name;
  char* rec_score;
  char* endptr;
  size_t size;
  long long_score;
  int score;
  
  printf("\n\tparse_record file\n");
  printf("parsing record: %s %s\n", rec_buf, rec_buf+10);

  name = calloc(FIELD_SIZE, sizeof(*rec_buf));
  rec_score = calloc(FIELD_SIZE, sizeof(*rec_buf));

  if(name == NULL || rec_score == NULL)
  {
    setMessage("%d parse_record: allocating memory with calloc failed. %s\n", 
              __LINE__);
    exit(EXIT_FAILURE);
  }

  strncpy(name, rec_buf, FIELD_SIZE);
  strncpy(rec_score,rec_buf+FIELD_SIZE, FIELD_SIZE);

  printf("User Name: %s\n", name);
  
  //storing long int into an int. Need to check
  //for conversion errors before assignment.
  long_score = strtol(rec_score, &endptr, 0);

  if(long_score > INT_MAX || long_score < MIN_SCORE)
  {
    //overflow detected.
    //fprintf(stderr, "Integer Overflow detected!\n");
    setMessage("%d parse_record: Integer overflow detected when assigning recorded score to int. %s\n", 
              __LINE__);
    exit(EXIT_FAILURE);

  } else {
    //safely assign.
    score = long_score;
  }

  if(*endptr != '\0')
  {
    //technically not needed
    //still works, the NaN, gets
    //replaced by a nul byte.
    //fprintf(stderr, "AAHHAHHAHHA, not a number detected!!!!\n\n Abort!...\n ABORT!!\n");
    setMessage("%d parse_record: Not a number detected in the score segment of the record. %s\n", 
              __LINE__);
    exit(EXIT_FAILURE);
  }

  printf("Total Score: %i\n", score);

  score_record_init(&rec, name, score);

  free(name);
  free(rec_score);
  // Note that writing the `rec_buf` parameter as `rec_buf[REC_SIZE]`
  // serves only as documentation of the intended use of the
  // function - C doesn't prevent arrays of other sizes being passed.
  //
  // In fact, nearly any time you use an array type in C,
  // it "decays" into a pointer -
  // the C11 standard, sec 6.3.2.1 ("Lvalues, arrays, and
  // function designators").
  //
  // (One significant exception is when you use sizeof() on an
  // array - in that case, the proper size of the array is
  // returned.)
  return rec;
}

/** Stores the player name and score in `rec` into a buffer of size
  * \ref REC_SIZE, representing a line from the scores file.
  *
  * The fields of rec should contain values that are valid for the
  * scores file; if not, the behaviour of the function is undefined.
  *
  * If the caller passes a buffer of size less than \ref REC_SIZE,
  * the behaviour of function is undefined.
  *
  * \param buf a `char` array of size \ref REC_SIZE.
  * \param rec pointer to a player's score record.
  */
void store_record(char buf[REC_SIZE], const struct score_record *rec) {
  //check size of buff
  
  //reset value of all bytes in bufferto null
  memset(buf, 0, REC_SIZE);

  //copy name and score
  snprintf(buf, FIELD_SIZE,"%s",rec->name);
  snprintf(buf+FIELD_SIZE, FIELD_SIZE,"%d",rec->score);

  //don't forget \n character
  buf[REC_SIZE-1] = '\n';

  printf("new record: %s %s\n", buf, buf+FIELD_SIZE);
}

/** search within the open scores file with file descriptor
  * `fd` for a line containing the score for `player_name`.
  * If no such line exists, -1 is returned; otherwise, the
  * offset within the file is returned.
  *
  * `filename` is used only for diagnostic purposes.
  *
  * \param filename name of the file described by `fd`.
  * \param fd file descriptor for an open scores file.
  * \param player_name player name to seek for.
  * \return position in the file where a record can be found,
  *   or -1 if no no such record exists.
  */
off_t find_record(const char * filename, int fd, const char * player_name) {
  char buffer[REC_SIZE];
  off_t offset;
  ssize_t bytes_read;
  int error;

  //error is a value signifying error in this function
  error = -2;
  offset = 0;

  if(hasError(lseek(fd, offset, SEEK_SET)))
    {
      setMessage("%d find_record: Failed to set file offset to start of file. %s\n", 
                __LINE__);
      return error;
    }

  printf("\n\t find_record file\n");
  printf("file size: %li\n", file_size(filename, fd));

  while((bytes_read = read(fd, buffer, REC_SIZE)) > 0)
  {
    //debugging
    char * p = buffer+10;
    
    // overwrite \n with \0//
    buffer[20] = '\0';
    printf("bytes read: %li\n", bytes_read);
    printf("contents: %s\n", buffer);
    printf("score: %s\n", p);

    if(strncmp(buffer, player_name, 10) == 0)
    {
      printf("\t\nPlayer FOUND: \n%s\n", buffer);
      printf("score: %s\n", p);
      printf("offset: %li\n", offset);

      return offset;

    } else {
      //Increment offset to skip this record.
      offset += bytes_read;
    }
  }

  if(hasError(bytes_read))
  {
    //read error occured.
    setMessage("%d find_record: Failed to read score file. %s\n", 
              __LINE__);
    return error;
  } else {
    printf("No record found.\n");
    //if there was no error, no record was found.
    return -1;
  }
}

/** Adjust the score for player `player_name` in the open file
  * with file descriptor `fd`, incrementing it by
  * `score_to_add`. If no record for a player with that name
  * is found in the file, then one is created and appended to
  * the file.
  *
  * The `filename` parameter is purely for diagnostic purposes.
  *
  * If the file is not a valid "scores" file, or player name is
  * longer than the allowable length for a score record,
  * the function may abort program execution.
  *
  * \param filename name of the file from which `fd` was obtained.
  * \param fd file descriptor for an open scores file.
  * \param player_name name of the player whose score should be incremented.
  * \param score_to_add amount by which to increment the score.
  */
void adjust_score_file(const char * filename, int fd, const char * player_name, int score_to_add) {
  int offset;
  long result;
  struct score_record rec;
  struct score_record oldRec;
  //NEED TO CHECK IF SIZE == REC_SIZE IN FUNCTIONS USED
  char buf[REC_SIZE];

  printf("\n\t adjust_score_file\n");
  score_record_init(&rec, player_name, score_to_add);
  offset = find_record(filename, fd, player_name);

  if(offset == -2){
    //if there was an error
    return;
  }else if(offset != -1)
  {
    //if old record was found
    if(hasError(lseek(fd, offset, SEEK_SET)))
    {
      setMessage("%d adjust_score_file: Failed to set file offset to record found. %s\n", 
                __LINE__);
      return;
    }

    if(hasError(read(fd, buf, REC_SIZE)))
    {
      setMessage("%d adjust_score_file: Failed to read contents of record found. %s\n", 
                __LINE__);
      return;
    } else {
      oldRec = parse_record(buf);

      printf("old record score: %i\n", oldRec.score);
      printf("score to add: %i\n", score_to_add);

      //check for overflow or underflow!!!!!!
      result = rec.score + oldRec.score;

      if(result > INT_MAX || result < MIN_SCORE)
      {
        //fprintf(stderr, "Integer Overflow detected!\n");
        setMessage("%d adjust_score_file: Buffer overflow detected when"
                   " summing score recorded with the score to add. %s\n", 
                __LINE__);
        return;
      } else{
        //otherwise safe to add
        rec.score += oldRec.score;
      }

      printf("new record score: %i\n", rec.score);
    }
  } else {
    offset = file_size(filename, fd);
  }

  //place back into file.
  if(hasError(lseek(fd, offset, SEEK_SET)))
  {
      setMessage("%d adjust_score_file: Failed to either, set offset to end of file"
                 " or set offset to where the record was found. %s\n", 
              __LINE__);
      return;
  } else {
    //call store_record.
    store_record(buf, &rec);

    if(hasError(write(fd, buf, REC_SIZE)))
    {
      setMessage("%d adjust_score_file: Failed to write to the score file. %s\n", 
              __LINE__);
      return;
    } else {



/******************************************/
      //THIS IS JUST FOR DEBUGGING REMOVE!!!
      printf("\n\nNEW RECORD:\n");
      lseek(fd, offset, SEEK_SET);
      read(fd, buf, REC_SIZE);
      printf("%s ", buf);
      printf("%s\n", buf+FIELD_SIZE);
    
/*****************************************/
    }
  }
}


/** Adjust the score for player `player_name`, incrementing it by
  * `score_to_add`. The player's current score (if any) and new score
  * are stored in the scores file at `/var/lib/curdle/scores`.
  * The scores file is owned by user ID `uid`, and the process should
  * use that effective user ID when reading and writing the file.
  * If the score was changed successfully, the function returns 1; if
  * not, it returns 0, and sets `*message` to the address of
  * a string containing an error message. It is the caller's responsibility
  * to free `*message` after use.
  *
  * \todo implement the function.
  *
  * \param uid user ID of the owner of the scores file.
  * \param player_name name of the player whose score should be incremented.
  * \param score_to_add amount by which to increment the score.
  * \param message address of a pointer in which an error message can be stored.
  * \return 1 if the score was successfully changed, 0 if not.
  */
int adjust_score(uid_t uid, const char * player_name, int score_to_add, char **message) {
  int fd;

  errno = 0;
  error_message = &message; 

  printf("\n\t adjust_score file\n");
  printf("curdle uid: %d\n", uid);
  printf("current user uid: %d\n", getuid());

  //increase permissions.
  if(hasError(seteuid(uid)))
  {
    setMessage(
        "%d adjust_score: Failed to increase current user's eUID to \"curdle\". %s\n", 
        __LINE__);
    fprintf(stderr, "%s", *message);
    return FAILURE;
  }

  printf("increased effective user uid: %d\n", geteuid());
  fd = open(FILEPATH, O_RDWR);

  //lower permissions.
  if(hasError(seteuid(getuid())))
  {
    setMessage("%d adjust_score: Failed to lower current user's eUID to UID. %s\n", 
        __LINE__);
    fprintf(stderr, "%s", *message);
    return FAILURE;
  }

  printf("lowered effective user uid: %d\n", geteuid());

  if(hasError(fd))
  {
    setMessage("%d adjust_score: Failed to open score file. %s\n", 
                __LINE__);
    fprintf(stderr, "%s", *message);

    return FAILURE;

  } else {
    printf("%li\n",file_size(FILEPATH, fd));
    adjust_score_file(FILEPATH, fd, player_name, score_to_add); 
    close(fd);

    if(message != NULL)
    {
      fprintf(stderr, "%s", *message);
      return FAILURE;
    } else {
      return SUCCESS;
    }
  }

}
