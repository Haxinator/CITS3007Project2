#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500

//#include "curdle.h"

/** \file adjust_score.c
 * \brief Functions for safely amending a player's score in the
 * `/var/lib/curdle/scores` file.
 *
 * Contains the \ref adjust_score_file function, plus supporting data
 * structures and functions used by that function.
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
#include <ctype.h>
/** Size of a field (name or score) in a line of the `scores`
  * file.
  */
#define FIELD_SIZE 10

/** Size of a line in the `scores` file.
  */
#define REC_SIZE 21

//actual
//#define FILEPATH  "/var/lib/curdle/scores"

//test good file
#define FILEPATH "/home/vagrant/Project/curdle/src/test.txt"
//FILE that should trigger invalid file format, decmial number in unexpected position
//#define FILEPATH "/home/vagrant/Project/curdle/src/bad1.txt"
//trigger invalid file, bad size, one record is not exactly 21 bytes
//#define FILEPATH "/home/vagrant/Project/curdle/src/bad2.txt"
//good test files
//#define FILEPATH "/home/vagrant/Project/curdle/tests/test-files/good/file0"

//Minimum and Maximum value score can have before it exceeds 10 bytes.
#define SCORE_MIN -999999999
#define SCORE_MAX 9999999999
//size of error message
#define MESSAGE_LEN 500
//denote SUCCESS & FAILURE in main function
#define SUCCESS 1
#define FAILURE 0
//used to denote a custom error
//not due to library function failing.
#define CUSTOMERROR -1

//global reference to the error message passed to adjust_score
//hasOccured is used to tell if a message occured.
struct error_record{
  char*** message;
  bool hasOccured;
}error;

//similar to one in skeleton, except stores score as a long
struct score_record{
  char name[FIELD_SIZE];
  long score;
};

/** Initialize a \ref score_record struct.
  *
  * \param rec pointer to a \ref score_record struct to initialize.
  * \param name player name to be inserted into `rec`.
  * \param score player score to be inserted into `rec`.
  */
void score_record_init(struct score_record *rec, const char *name, long score) {
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
    error.hasOccured = true;
    //in case the *char pointed to has old contents.
    free(**error.message);
    **error.message = malloc(sizeof(char)*MESSAGE_LEN);
    //make sure no garbage in message
    memset(**error.message, 0, FIELD_SIZE);
    if(**error.message == NULL)
    {
      //unrecoverable failure, memory failed to be allocated
      exit(EXIT_FAILURE);
    }
    return true;
  } else {
    return false;
  }
}

/**
  * This function sets the error.message to the message specified.
  * This function assumes hasError has been called and has already 
  * allocated memory to the message pointer.
  * It makes sure all messages have the same format.
  * It concatenates strerror(errno) to the end of the error message.
  * This function is used for errors that occured in library functions.
  *
  *
  * \param message is the message to to set, can include formating.
  * \lineNo the line where the error occured (roughly).
  */
void perrorMessage(const char* message, int lineNo)
{
  snprintf(**error.message, MESSAGE_LEN, 
        message, __FILE__, lineNo, strerror(errno));
}

/**
  * This function sets the error.message to the message specified.
  * This function does not assume hasError has been called. It
  * calls hasError to allocate memory for the message.
  * It makes sure all custom messages have the same format.
  *
  * Unlike perrorMessage, there is no error number associated
  * with this error. It is a custom error, unique to this API.
  *
  * \param message is the message to to set, can include formating.
  * \lineNo the line where the error occured (roughly).
  */
void setMessage(const char* message, int lineNo)
{
  hasError(CUSTOMERROR);
  snprintf(**error.message, MESSAGE_LEN, 
        message, __FILE__, lineNo);
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
  int value;

  value = fstat(fd,&file_info);

  if(hasError(value))
  {
      perrorMessage("%s:%d:file_size: Failed to determine the size of the score file. %s\n",
                __LINE__);
      return value;
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
  struct score_record error;
  char* name;
  char* rec_score;
  char* endptr;
  long score;
  char* copy_rec_buf; 


  //error used purely for readability
  score_record_init(&error, "", 0);
  printf("\n\tparse_record file\n");
  printf("parsing record: %s %s\n", rec_buf, rec_buf+10);

  name = calloc(FIELD_SIZE, sizeof(*rec_buf));
  rec_score = calloc(FIELD_SIZE, sizeof(*rec_buf));
  copy_rec_buf = &rec_buf[0];

  if(name == NULL || rec_score == NULL)
  {
    setMessage("%s:%d:parse_record: allocating memory with calloc failed.\n", 
              __LINE__);
    return error;
  }

  strncpy(name, rec_buf, FIELD_SIZE);
  strncpy(rec_score,copy_rec_buf+FIELD_SIZE, FIELD_SIZE);

  printf("User Name: %s\n", name);
  
  score = strtol(rec_score, &endptr, 0);

  //need to read whole line, to check this
  //don't need to do this, could just check
  //if byte 21 is a new line '\n'
  //if(score > SCORE_MAX || score < SCORE_MIN)
  /*if(rec_buf[20] != '\n')
  {
    hasError(CUSTOMERROR);
    setMessage(CUSTOM,
              "%d parse_record: Invalid record, 21st byte is not a new line character.\n", 
              __LINE__);
    return error;

  } else 
*/
    if(*endptr != '\0') {
    setMessage(
              "%s:%d:parse_record: Not a number detected in the score segment of the record.\n", 
              __LINE__);
    return error;
  } else {

    printf("Total Score: %li\n", score);

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
  
  printf("\tstore_record\n");
  //reset value of all bytes in bufferto null
  memset(buf, 0, REC_SIZE);

  //copy name and score
  snprintf(buf, FIELD_SIZE,"%s",rec->name);
  //FIELD_SIZE+1, else snprintf will only read 9 bytes
  snprintf(buf+FIELD_SIZE, FIELD_SIZE+1,"%li",rec->score);

  //don't forget \n character
  buf[REC_SIZE-1] = '\n';

  printf("new record: %s %s\n", buf, buf+FIELD_SIZE-1);
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

  printf("\tfind_record\n");
  //error used for ease of reading, serves no real purpose.
  error = -2;
  offset = 0;

  if(hasError(lseek(fd, offset, SEEK_SET)))
    {
      perrorMessage("%s:%d:find_record: Failed to set file offset to start of file. %s\n", 
                __LINE__);
      return error;
    }

  printf("\n\t find_record file\n");
  printf("file size: %li\n", file_size(filename, fd));

  while((bytes_read = read(fd, buffer, REC_SIZE)) > 0)
  {
    //debugging
    char * p = buffer+10;
    
    if(buffer[20] != '\n')
    {
      setMessage("%s:%d:find_record: Score file has invalid format, the 21st character " 
                "in a record was not a '\\n'.\n",
                __LINE__);
    } else if(buffer[9] != '\0'){
      setMessage("%s:%d:find_record: Score file has invalid format, 10th character " 
                "in a record was not a '\\0'.\n",
                __LINE__);

    } else if(isdigit(buffer[10]) == 0 && buffer[10] != '-'){
      setMessage("%s:%d:find_record: Score file has invalid format, a record does not "
                "contain a decimal number in the valid format. The score should "
                "begin on the 11th character right padded with '\\0'. \n",
                __LINE__);
      fprintf(stderr, "Instead got %c\n", buffer[10]);
    }
    // overwrite \n with \0//
    buffer[20] = '\0'; //remove once messages aren't needed
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
    perrorMessage("%s:%d:find_record: Failed to read score file. %s\n", 
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
  struct score_record rec;
  struct score_record oldRec;
  //NEED TO CHECK IF SIZE == REC_SIZE IN FUNCTIONS USED
  char buf[REC_SIZE];

  printf("\n\t adjust_score_file\n");
  score_record_init(&rec, player_name, score_to_add);
  offset = find_record(filename, fd, player_name);

  if(error.hasOccured){
    return;
  }

  if(offset < 0)
  {
    offset = file_size(filename, fd);
    if(error.hasOccured)
    {
      return;
    }
  }

  if(hasError(lseek(fd,offset, SEEK_SET)))
  {
    perrorMessage("%s:%d:adjust_score_file: Failed to set file offset to record found. %s\n", 
              __LINE__);
    return;
  } 

  if(offset > -1)
  {
    if(hasError(read(fd, buf, REC_SIZE)))
    {
      perrorMessage("%s:%d:adjust_score_file: Failed to read contents of record found. %s\n", 
                __LINE__);
      return;
    }

    oldRec = parse_record(buf);

    if(error.hasOccured)
    {
      return;
    }
    printf("old record score: %li\n", oldRec.score);
    printf("score to add: %i\n", score_to_add);

    rec.score += oldRec.score;

    if(rec.score > SCORE_MAX || rec.score < SCORE_MIN)
    {
      //allocate memory for error message
      setMessage("%s:%d:adjust_score_file: Invalid byte size detected when "
                 "summing score recorded with the score to add. "
                 "New score is larger than 10 bytes.\n", 
              __LINE__);
      return;
    }
  }

  //place back into file.
  if(hasError(lseek(fd, offset, SEEK_SET)))
  {
    perrorMessage("%s:%d:adjust_score_file: Failed to either, set offset to end of file "
              "or set offset to where the record was found. %s\n", 
              __LINE__);
    return;
  } else {
    //call store_record.
    store_record(buf, &rec);

    if(hasError(write(fd, buf, REC_SIZE)))
    {
      perrorMessage("%s:%d:adjust_score_file: Failed to write to the score file. %s\n", 
              __LINE__);
      return;
    }
/******************************************/
    //THIS IS JUST FOR DEBUGGING REMOVE!!!
    printf("\n\nNEW RECORD:\n");
    lseek(fd, offset, SEEK_SET);
    read(fd, buf, REC_SIZE);
    printf("%s ", buf);
    printf("%s\n", buf+FIELD_SIZE);
    
/*****************************************/
  }
/*
  if(error.hasOccured){
    //if there was an error
    return;
  }else if(offset < 0){
    //if no record was found
    printf("setting offset to EOF.\n");
    offset = file_size(filename, fd);
    if(error.hasOccured)
    {
      printf("error occured\n");
      return;
    }
  }else if(offset > -1){
    //if old record was found
    if(hasError(lseek(fd, offset, SEEK_SET)))
    {
      perrorMessage("%s:%d:adjust_score_file: Failed to set file offset to record found. %s\n", 
                __LINE__);
      return;
    } else if(hasError(read(fd, buf, REC_SIZE))){
      perrorMessage("%s:%d:adjust_score_file: Failed to read contents of record found. %s\n", 
                __LINE__);
      return;
    } else {
      oldRec = parse_record(buf);

      if(error.hasOccured)
      {
        //stop function
        return;
      }
      printf("old record score: %li\n", oldRec.score);
      printf("score to add: %i\n", score_to_add);

      rec.score += oldRec.score;

      if(rec.score > SCORE_MAX || rec.score < SCORE_MIN)
      {
        //allocate memory for error message
        setMessage("%s:%d:adjust_score_file: Invalid byte size detected when "
                   "summing score recorded with the score to add. "
                   "New score is larger than 10 bytes.\n", 
                __LINE__);
        return;
      }

      printf("new record score: %li\n", rec.score);
    }
    

  }
*/
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
  size_t fileSize;

  errno = 0;
  error.message = &message; 
  error.hasOccured = false;

  printf("\n\t adjust_score\n");
  printf("curdle uid: %d\n", uid);
  printf("current user uid: %d\n", getuid());

  //check if parameters passed are valid.
  /*
  if(player_name[9] != '\0')
  {
    //hard to check for, since I don't know if the name passed
    //is of 10 bytes in size, could be smaller
    hasError(-1);
    setMessage(
        "%d adjust_score: Invalid player name passed. The tenth byte was not a nul byte."
        "name must be 10 bytes in size with the tenth byte being a nul byte %s\n", 
        __LINE__);
    return FAILURE;
  } 
  */
  if(score_to_add < SCORE_MIN){
    setMessage("%s:%d:adjust_score: Invalid score_to_add passed. `score_to_add` " 
              "was larger than 10 bytes. `score_to_add` must be an int and "
              "can only be at most 10 bytes in size this includes the minus symbol.\n", 
              __LINE__);
    return FAILURE;
  } else if(message == NULL){
    //user didn't provide a valid address to store error messages.
    //No point setting an error message, since the user won't see it anyway.
    return FAILURE;
  }

  //maybe check if message pointer is valid?

  //increase permissions.
  if(hasError(seteuid(uid)))
  {
    perrorMessage("%s:%d:adjust_score: Failed to increase current user's eUID to \"curdle\". %s\n",
              __LINE__);
    return FAILURE;
  }

  printf("increased effective user uid: %d\n", geteuid());
  fd = open(FILEPATH, O_RDWR);

  //lower permissions.
  if(hasError(seteuid(getuid())))
  {
    perrorMessage("%s:%d:adjust_score: Failed to lower current user's eUID to UID. %s\n", 
              __LINE__);
    return FAILURE;
  }

  fileSize = file_size(FILEPATH,fd);
  printf("lowered effective user uid: %d\n", geteuid());
  printf("%li\n",fileSize%21);
  printf("%d\n",fileSize%21 == 0);
  if(hasError(fd))
  {
    perrorMessage("%s:%d:adjust_score: Failed to open score file. %s\n", 
              __LINE__);
    return FAILURE;

  } else if((fileSize%21) != 0){
    setMessage("%s:%d:adjust_score: Score file has invalid size. "
              "Not all records are of length 21.\n", 
              __LINE__);
    return FAILURE;

  }else {

    adjust_score_file(FILEPATH, fd, player_name, score_to_add); 
    close(fd);

    if(error.hasOccured)
    {
      return FAILURE;
    } else {
      return SUCCESS;
    }
  }

}
