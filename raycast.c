#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Color3 {
  unsigned char r, g, b;
} Color3

typedef struct Vector3 {
  double x, y, z;
} Vector3;

typedef struct Camera {
  double width, height;
} Camera;

typedef struct Sphere {
  Color3 color;
  Vector3 position;
  double radius;
} Sphere;

typedef struct Plane {
  Color3 color;
  Vector3 position;
  Vector3 normal;
}

void skipWhitespace(FILE* json) {
  int c = fgetc(json);
  while (isspace(c)) {
    c = fgetc(json);
  }
  ungetc(c, json);
}

char* parseString(FILE* json) {
  char buffer[128];
  int c = fgetc(json);
  if (c != '"') {
    fprintf(stderr, "Error: String must start with '\"'");
    exit(1);
  }

  int i = 0;
  while (c = fgetc(json) != '"') {
    buffer[i++] = c;
  }

  buffer[i] = '\0';
  return strdup(buffer);
}

void parseJSON(char* fileName) {
  FILE* json = fopen(fileName, "r");
  skipWhitespace(json);

  int c = fgetc(json);
  if (c != '[') {
    fprintf(stderr, "Error: JSON file must start with '['");
    fclose(json);
    exit(1);
  }

  skipWhitespace(json);

  c = fgetc(json);
  while (c == '{') {
    //Parse the object
    char* key = parseString(json);
    if (strcmp(key, "type") != 0) {
      fprintf(stderr, "JSON objects must start with \"type\"");
      exit(1);
    }

    free(key);

    skipWhitespace(json);
    c = fgetc(json);
    if (c != ':') {
      fprintf(stderr, "Error: Type not followed by ':'");
      exit(1);
    }

    skipWhitespace(json);
    char* type = parseString(json);

    if (strcmp(type, "camera") == 0) {

    } else if (strcmp(type, "sphere") == 0) {

    } else if (strcmp(type, "plane") == 0) {

    } else {
      fprintf(stderr, "Error: Invalid type. Must be \"camera\", \"sphere\", or \"plane\".");
      fclose(json);
      exit(1);
    }

  }

  c = fgetc(json);
  if (c == ']') {
    fclose(json);
  }
}

int main(int argc, char* argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: raycast width height input.json output.ppm");
    exit(1);
  }

  parseJSON(argv[3]);

  return 0;
}
