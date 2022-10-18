#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500

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
#include <sys/types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

//Size of a field (name or score) in a line of the `scores` file.
#define FIELD_SIZE 10

//Size of a line in the `scores` file.
#define REC_SIZE 21

//location of scores file
//#define FILEPATH  "/var/lib/curdle/scores"

//test good file
#define FILEPATH "/home/vagrant/Project/curdle/src/test.txt"

//Minimum and Maximum value score can have before it exceeds 10 bytes.
#define SCORE_MIN -999999999
#define SCORE_MAX 9999999999
//size of error message
#define MESSAGE_LEN 500
//denote SUCCESS & FAILURE in main function
#define SUCCESS 1
#define FAILURE 0
//used to denote an error that was not caused by a library function failing.
#define CUSTOMERROR -1

/*global reference to the errors encountered
Field: message is a reference to the error message passed to adjust_score.
Field: hasOccured is used to check if an error occured.*/
struct error_record{
  char*** message;
  bool hasOccured;
}error;

//do not compile with curdle.h!
//similar to one in skeleton, except stores score as a long
struct score_record{
  char name[FIELD_SIZE];
  long score;
};

/** Initialize a score_record struct.
  *
  * \param rec pointer to a \ref score_record struct to initialize.
  * \param name player name to be inserted into `rec`.
  * \param score player score to be inserted into `rec`.
  */
void score_record_init(struct score_record *rec, const char *name, long score) {
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
    free(**error.message); //clear any old contents
    **error.message = malloc(sizeof(char)*MESSAGE_LEN);
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
  * It concatenates strerror(errno) to the error message.
  * This function is used for errors that occured in library functions.
  *
  *
  * \param message is the message to to set, include formating.
  * \lineNo the line where the error occured (roughly).
  */
void perrorMessage(const char* message, int lineNo)
{
  snprintf(**error.message, MESSAGE_LEN, 
        message, FILEPATH, __FILE__, lineNo, strerror(errno));
}

/**
  * This function sets the error.message to the message specified.
  * This function does not assume hasError has been called. It
  * calls hasError to allocate memory for the message.
  * It makes sure all custom messages have the same format.
  *
  * Unlike perrorMessage, there is no error number associated
  * with this error. Therefore strerr(errno) is not contatenated
  * to this error message.
  *
  * \param message is the message to to set, can include formating.
  * \lineNo the line where the error occured (roughly).
  */
void setMessage(const char* message, int lineNo)
{
  /*hasError is called to allocate memory and 
    signify an error has occured*/
  hasError(CUSTOMERROR);
  snprintf(**error.message, MESSAGE_LEN, 
        message, FILEPATH, __FILE__, lineNo);
}

/** Return the size of the open file with file descriptor `fd`.
  * If the size cannot be determined, the function may abort program
  * execution (after printing an error message).
  *
  * \param fd file descriptor for an open file.
  * \return the size of the file described by `fd`.
  */
size_t file_size(int fd) {
  struct stat file_info;
  int value;

  value = fstat(fd,&file_info);

  if(hasError(value))
  {
    perrorMessage("Score file %s\n%s: %d: file_size: "
                "Failed to determine the size of the score file. %s\n",
                __LINE__);
    
    return value;

  } else {
    
    return file_info.st_size;

  }
}


/** Parse a score_record struct from an
  * array of size REC_SIZE.
  *
  * If a name and score cannot be found in `rec_buf`,
  * the function may abort program
  * execution (after printing an error message).
  *
  * NOTE: this function does not checking for invalid 
  * record formats, that where already checked for in 
  * find_record. Namely invalid name size, invalid score
  * size, and a lack of a '\n' character.
  * However, it does check whether there are non-decimal
  * characters in the score.
  *
  * the local variable error has no real purpose, used
  * to signify that the function returned due to an
  * error. So it's just for readability.
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

  name = calloc(FIELD_SIZE, sizeof(*rec_buf));
  rec_score = calloc(FIELD_SIZE, sizeof(*rec_buf));
  copy_rec_buf = &rec_buf[0];
  
  score_record_init(&error, "", 0);

  if(name == NULL || rec_score == NULL)
  {
    setMessage("Score file %s\n: %s: %d: parse_record: "
              "Allocating memory with calloc failed.\n",
              __LINE__);

    return error;
  }

  strncpy(name, rec_buf, FIELD_SIZE);
  strncpy(rec_score,copy_rec_buf+FIELD_SIZE, FIELD_SIZE);

  score = strtol(rec_score, &endptr, 0);

  if(*endptr != '\0') {

    setMessage("Score file %s\n%s: %d: parse_record: "
              "Not a number detected in the score segment of the record.\n",
              __LINE__);

    return error;

  } else {

    score_record_init(&rec, name, score);
    free(name);
    free(rec_score);

    return rec;
  }
}

/** Stores the player name and score in `rec` into a buffer of size
  * REC_SIZE, representing a line from the scores file.
  *
  * The fields of rec should contain values that are valid for the
  * scores file; if not, the behaviour of the function is undefined.
  *
  * If the caller passes a buffer of size less than REC_SIZE,
  * the behaviour of function is undefined.
  *
  * \param buf a `char` array of size REC_SIZE.
  * \param rec pointer to a player's score record.
  */
void store_record(char buf[REC_SIZE], const struct score_record *rec) {
  
  memset(buf, 0, REC_SIZE);
  snprintf(buf, FIELD_SIZE,"%s",rec->name);

  //FIELD_SIZE+1, else snprintf will only read 9 bytes of score
  snprintf(buf+FIELD_SIZE, FIELD_SIZE+1,"%li",rec->score);
  buf[REC_SIZE-1] = '\n';
}

/** search within the open scores file with file descriptor
  * `fd` for a line containing the score for `player_name`.
  * If no such line exists, -1 is returned; otherwise, the
  * offset within the file is returned.
  *
  * Note the `error` local variable has no real purpose
  * just denotes that the function is returning due to
  * an error. In other words it's purely for readability.
  *
  * \param fd file descriptor for an open scores file.
  * \param player_name player name to seek for.
  * \return position in the file where a record can be found,
  *   or -1 if no no such record exists.
  */
off_t find_record(int fd, const char * player_name) {
  char buffer[REC_SIZE];
  off_t offset;
  ssize_t bytes_read;
  int error;

  error = -2;
  offset = 0;

  if(hasError(lseek(fd, offset, SEEK_SET)))
  {
    perrorMessage("Score file %s\n%s: %d: find_record: "
                "Failed to set file offset to start of file. %s\n", 
                __LINE__);

    return error;
  }

  while((bytes_read = read(fd, buffer, REC_SIZE)) > 0)
  {
    
    if(buffer[REC_SIZE-1] != '\n')
    {
      setMessage("Score file %s\n%s: %d: find_record: "
                "Score file has invalid format, the 21st character " 
                "in a record was not a '\\n'.\n",
                __LINE__);

    } else if(buffer[FIELD_SIZE-1] != '\0'){

      setMessage("Score file %s\n%s: %d: find_record: "
                "Score file has invalid format, 10th character " 
                "in a record was not a '\\0'.\n",
                __LINE__);

    } else if(isdigit(buffer[FIELD_SIZE]) == 0 && buffer[FIELD_SIZE] != '-'){

      setMessage("Score file %s\n%s: %d: find_record: "
                "Score file has invalid format, a record does not "
                "contain a decimal number in the valid format. The score must "
                "begin on the 11th character right padded with '\\0'. \n",
                __LINE__);
    
    } else if(strncmp(buffer, player_name, FIELD_SIZE) == 0){

      return offset;

    } else {
      //Increment offset to skip this record.
      offset += bytes_read;
    }
  }

  if(hasError(bytes_read))
  {
    perrorMessage("Score file %s\n%s: %d: "
                "find_record: Failed to read score file. %s\n", 
                __LINE__);

    return error;

  } else {
    //no record was found.
    return -1;
  }
}

/** Adjust the score for player `player_name` in the open file
  * with file descriptor `fd`, incrementing it by
  * `score_to_add`. If no record for a player with that name
  * is found in the file, then one is created and appended to
  * the file.
  *
  * If the file is not a valid "scores" file, or player name is
  * longer than the allowable length for a score record,
  * the function may abort program execution.
  *
  * \param fd file descriptor for an open scores file.
  * \param player_name name of the player whose score should be incremented.
  * \param score_to_add amount by which to increment the score.
  */
void adjust_score_file(int fd, const char * player_name, int score_to_add) {
  int offset;
  struct score_record rec;
  struct score_record oldRec;
  char buf[REC_SIZE];
  
  score_record_init(&rec, player_name, score_to_add);
  offset = find_record(fd, player_name);
  strncpy(buf, player_name, FIELD_SIZE+1);

  if(error.hasOccured){

    //error occured in find_record
    return;

  } else if(buf[FIELD_SIZE-1] != '\0'){

    setMessage("Score file %s\n%s: %d: adjust_score_file: "
        "Invalid player name passed. Name is too large. "
        "Name must be 10 bytes in size with the tenth byte being a nul byte.\n", 
        __LINE__);

    return;

  } else if(offset < 0){

    //record was not found in find_record
    offset = file_size(fd);

    if(error.hasOccured)
    {
      //error occured in file_size
      return;
    }

  } else if(hasError(lseek(fd,offset, SEEK_SET))){

    perrorMessage("Score file %s\n%s: %d: adjust_score_file: "
                "Failed to set file offset to record found. %s\n", 
                __LINE__);

    return;

  } else {
    
    //offset > -1 record was found
    if(hasError(read(fd, buf, REC_SIZE)))
    {
      perrorMessage("Score file %s\n%s: %d: adjust_score_file: "
                "Failed to read contents of record found. %s\n", 
                __LINE__);
      return;
    }

    oldRec = parse_record(buf);

    if(error.hasOccured)
    {
      //error occured in parse_record
      return;
    }

    rec.score += oldRec.score;

    if(rec.score > SCORE_MAX || rec.score < SCORE_MIN)
    {
      setMessage("Score file %s\n%s: %d: adjust_score_file: "
                "Invalid byte size detected when "
                "summing score recorded with the score to add. "
                "New score is larger than 10 bytes.\n",
              __LINE__);

      return;
    }
  }

  //place back into file.
  if(hasError(lseek(fd, offset, SEEK_SET)))
  {
    perrorMessage("Score file %s\n%s: %d: adjust_score_file: "
                "Failed to either, set offset to end of file "
                "or set offset to where the record was found. %s\n",
                __LINE__);

    return;

  } else {

    store_record(buf, &rec);

    if(hasError(write(fd, buf, REC_SIZE)))
    {
      perrorMessage("Score file %s\n%s: %d: adjust_score_file: "
                  "Failed to write to the score file. %s\n", 
                  __LINE__);
      return;
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
  * \param uid user ID of the owner of the scores file.
  * \param player_name name of the player whose score should be incremented.
  * \param score_to_add amount by which to increment the score.
  * \param message address of a pointer in which an error message can be stored.
  * \return 1 if the score was successfully changed, 0 if not.
  */
int adjust_score(uid_t uid, const char * player_name, int score_to_add, char **message) {
  int fd;

  errno = 0;
  error.message = &message; 
  error.hasOccured = false;

  if(score_to_add < SCORE_MIN){

    setMessage("Score file %s\n%s: %d: adjust_score: "
              "Invalid score_to_add passed. `score_to_add` " 
              "was larger than 10 bytes. `score_to_add` must be an int and "
              "can only be at most 10 bytes in size this includes the minus symbol.\n", 
              __LINE__);
    
    return FAILURE;

  } else if(message == NULL){

    /*invalid message value passed. No point setting an error message, 
      since the user won't see it anyway.*/
    return FAILURE;

  } else if(hasError(seteuid(uid)))
  {
    perrorMessage("Score file %s\n%s: %d: adjust_score: "
                "Failed to increase current user's eUID to \"curdle\". %s\n",
                __LINE__);

    return FAILURE;

  } else {
    fd = open(FILEPATH, O_RDWR);
  }



  if(hasError(seteuid(getuid())))
  {
    perrorMessage("Score file %s\n%s: %d: adjust_score: "
                "Failed to lower current user's eUID to UID. %s\n", 
                __LINE__);

    return FAILURE;

  } else if(hasError(fd)){

    perrorMessage("Score file %s\n%s: %d: adjust_score: "
                "Failed to open score file. %s\n", 
                __LINE__);
    
    return FAILURE;

  } else if((file_size(fd)%21) != 0){

    setMessage("Score file %s\n%s: %d: adjust_score: "
              "Score file has invalid size. "
              "Not all records are of length 21.\n", 
              __LINE__);
    
    return FAILURE;

  }else {

    adjust_score_file(fd, player_name, score_to_add); 
    close(fd);

    if(error.hasOccured)
    {
      //error occured in adjust_score_file.
      return FAILURE;

    } else {

      return SUCCESS;

    }
  }
}
