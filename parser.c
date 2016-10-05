/*
Created by Zowie Haugaard
10/4/16
JSON Scene File Parser
  This parser reads a supplied json scene file containing any number of primative forms
  and returns a list of objects to be used by a ray caster. Function prototypes included in
  the parser.h file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

int line = 1;
int size = 0;

//Get the next character from the file, perform some error checking
int next_c(FILE* json){

  int c = fgetc(json);

  if (c == '\n'){
    line += 1;
  }
  if (c == EOF){
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }

  return c;
}


//check if the next character is an expected value, if not return an error
void expect_c(FILE* json, int d){
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);
}


// skip_ws() skips white space in the file.
void skip_ws(FILE* json){
  int c = next_c(json);
  while (isspace(c)){
    c = next_c(json);
  }
  ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json){
  char buffer[129];

  int c = next_c(json);
  //strings should begin with a quote
  if (c != '"'){
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }

  c = next_c(json);
  int i = 0;

  //read the string until we come to the closing quote
  while (c != '"') {
    //maximum size
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);
    }
    //disallow escapes
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);
    }
    //ensure only ascii values
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    //put the character into the buffer
    buffer[i] = c;
    i += 1;
    //get the next character
    c = next_c(json);
  }
  //indicate end of string
  buffer[i] = 0;

  return strdup(buffer);
}

//Get the next numerical value from the file
double next_number(FILE* json) {
  float value;
  if(fscanf(json, "%f", &value)!= 1){
    fprintf(stderr, "Error: character not found on line %d\n", line);
    exit(1);
  }

  return value;
}

//use our other methods to help return a double vector
double* next_vector(FILE* json) {
  //allocate the memory
  double* v = malloc(3*sizeof(double));
  //look for the begining of the vector
  expect_c(json, '[');
  skip_ws(json);
  //put the first number in
  v[0] = next_number(json);
  skip_ws(json);
  //expect a comma after each value
  expect_c(json, ',');
  skip_ws(json);
  //get the second value
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  //get the third value
  v[2] = next_number(json);
  skip_ws(json);
  //look for the end of the vector
  expect_c(json, ']');
  return v;
}

//return the number of elements in the object array
int getSize(){
  return size;
}

//read in the json info and return an array of objects
Object** read_scene(char* filename) {
  //c for character, i for index
  int c, i = 0;
  //open the file
  FILE* json = fopen(filename, "r");

  //create a new array of objects
  Object** objects = malloc(sizeof(Object*)*2);

  //error if file un openable
  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }

  skip_ws(json);

  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects
  while (1) {

    c = fgetc(json);
    //error if empty list or unexpected end of scene
    if (c == ']') {
      fprintf(stderr, "Error: Unexpected end of scene file.\n");
      fclose(json);
      return objects;
    }

    //beginning of an object
    if (c == '{') {

      size += 1;

      printf("Parsing object #%d ...\n",size);

      skip_ws(json);
      //allocate space for the object
      objects[i] = malloc(sizeof(Object));

      // Parse the object type keyword
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
        fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
        exit(1);
      }

      skip_ws(json);

      //colin seperating key value pairs as per the json specification
      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);

      //Check what type of object we are currently parsing, and set it
      if (strcmp(value, "camera") == 0) {
        objects[i]->kind = 0;
      }
      else if (strcmp(value, "sphere") == 0) {
        objects[i]->kind = 1;
      }
      else if (strcmp(value, "plane") == 0) {
        objects[i]->kind = 2;
      }
      //if not one of these, throw error
       else {
         fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
         exit(1);
      }

      skip_ws(json);

      //Key check values
      int col = 0, p = 0, r = 0, n = 0, w = 0, h = 0;

      //loop goes through each attribute of the object until the closing bracket is read
      while (1) {

        c = next_c(json);
        //end of the object
        if (c == '}') {

          //This switch statement ensures that each object has the correct attributes
          //  and that none of the objects are incomplete
          switch (objects[i]->kind) {
            case 0:
              if(w == 0 || h == 0){
                fprintf(stderr, "Error: please provide a valid camera object\n" );
                exit(1);
              }
              break;
            case 1:
              if(p == 0 || col == 0 || r == 0){
                fprintf(stderr, "Error: please provide a valid sphere object\n" );
                exit(1);
              }
              break;
            case 2:
              if(p == 0 || col == 0 || n == 0){
                fprintf(stderr, "Error: please provide a valid plane object\n" );
                exit(1);
              }
              break;
          }
          break;
        }
        //We have another attribute to read
         else if (c == ',') {

           skip_ws(json);
           //get the next key
           char* key = next_string(json);

           skip_ws(json);
           expect_c(json, ':');
           skip_ws(json);

           //Get the width - only applies to camera objects
           if (strcmp(key, "width") == 0){
             if(objects[i]->kind != 0) {
                fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
                exit(1);
              }
              w = 1;
                objects[i]->camera.width =next_number(json);
            }

            //Get the height - only applies to camera objects
            else if(strcmp(key, "height") == 0){
              if(objects[i]->kind != 0) {
                fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
                exit(1);
              }
              h = 1;
                objects[i]->camera.height =next_number(json);
            }

            //Get the radius - only applies to sphere objects
            else if(strcmp(key, "radius") == 0){
              if(objects[i]->kind != 1) {
                fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
                exit(1);
              }
              r = 1;
              objects[i]->sphere.radius = next_number(json);
            }
            //Get the color - applies to all but camera objects
            else if(strcmp(key, "color") == 0){
              if(objects[i]->kind == 0) {
                fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
                exit(1);
              }
              col = 1;
              //colors are stored as double vectors
              double* val = next_vector(json);
              //color for spheres
              if(objects[i]->kind == 1) {
                objects[i]->sphere.color[0] = val[0];
                objects[i]->sphere.color[1] = val[1];
                objects[i]->sphere.color[2] = val[2];

              }
              //Color for planes
              else {
                objects[i]->plane.color[0] = val[0];
                objects[i]->plane.color[1] = val[1];
                objects[i]->plane.color[2] = val[2];
              }
            }

            //Get the position - applies to all but camera objects
            else if(strcmp(key, "position") == 0) {
              if(objects[i]->kind == 0) {
                fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
                exit(1);
              }

              p = 1;
              double* value = next_vector(json);

              if(objects[i]->kind == 1) {
                objects[i]->sphere.position[0] = value[0];
                objects[i]->sphere.position[1] = value[1];
                objects[i]->sphere.position[2] = value[2];

              }
              else {
                objects[i]->plane.position[0] = value[0];
                objects[i]->plane.position[1] = value[1];
                objects[i]->plane.position[2] = value[2];

              }

            }
            //Get the normal - only applies to plane camera objects
            else if(strcmp(key, "normal") == 0){
              if(objects[i]->kind != 2){
                fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
                exit(1);
              }

              n = 1;
              double* value = next_vector(json);

              objects[i]->plane.normal[0] = value[0];
              objects[i]->plane.normal[1] = value[1];
              objects[i]->plane.normal[2] = value[2];
            }
            //We didnt recognize the property
            else {
              fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",key, line);
              exit(1);
            }
            skip_ws(json);
          }
          //We recieved some other character
          else {
            fprintf(stderr, "Error: Unexpected value on line %d\n", line);
            exit(1);
          }
        } //End of object attribute loop

        skip_ws(json);
        c = next_c(json);
        //check to see if there are more objects to read
        if (c == ',') {
          //add more space to our array
          objects = realloc(objects, sizeof(*objects)*(i+2));
          skip_ws(json);
          //increase index
          i += 1;
        }
        //Check to see if e have reached the end of our file
        else if (c == ']') {
          //close it out
          fclose(json);
          //return the array
          return objects;
        }
        //otherwise, we read an unexpected symbol, fail
        else {
          fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
          exit(1);
        }
      }
    }
  }
  //
  // int main() {
  //   Object** objects = read_scene("objects.json");
  //
  //   return 0;
  // }
