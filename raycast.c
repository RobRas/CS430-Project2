#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PLANE 0
#define SPHERE 1

int line = 1;

typedef struct {
  double width;
  double height;
} Camera;

typedef struct {
  int kind; // 0 = Plane, 1 = Sphere
  double color[3];
  double position[3];
  union {
    struct {
      double normal[3];
    } plane;
    struct {
      double radius;
    } sphere;
  };
} Object;

Camera camera;
Object** objects;

static inline double sqr(double v) {
  return v*v;
}

static inline void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}


// Wraps the getc() function and provides error checking and
// number maintenance
int fnextc(FILE* json) {
  int c = fgetc(json);
#ifdef DEBUG
  printf("fnextc: '%c'\n", c);
#endif
  if (c == '\n') {
    line++;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}

// fexpectc() checks that the next character in d. If it is not it
// emits and error.
void fexpectc(FILE* json, int d) {
  int c = fnextc(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);
}

// skipWhitespace skips white space in the file.
void skipWhitespace(FILE* json) {
  int c = fnextc(json);
  while (isspace(c)) {
    c = fnextc(json);
  }
  ungetc(c, json);
}

// parseString gets the next string from the file handle and emits
// an error if a string can not be obtained.
char* nextString(FILE* json) {
  char buffer[129];
  int c = fnextc(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }
  c = fnextc(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported. See line %d.\n", line);
      exit(1);
    } else if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supperted. See line %d.\n", line);
      exit(1);
    } else if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters. See line %d.\n", line);
      exit(1);
    }
    buffer[i] = c;
    i++;
    c = fnextc(json);
  }
  buffer[i] = '\0';
  return strdup(buffer);
}

double nextNumber(FILE* json) {
  double value;
  fscanf(json, "%lf", &value);
  // Error check this...
  return value;
}

double* nextVector(FILE* json) {
  double* v = malloc(3 * sizeof(double));
  fexpectc(json, '[');
  skipWhitespace(json);
  v[0] = nextNumber(json);
  skipWhitespace(json);
  fexpectc(json, ',');
  skipWhitespace(json);
  v[1] = nextNumber(json);
  skipWhitespace(json);
  fexpectc(json, ',');
  skipWhitespace(json);
  v[2] = nextNumber(json);
  skipWhitespace(json);
  fexpectc(json, ']');
  return v;
}

void parseObject(FILE* json, int currentObject) {
  int c;
  while (1) {
    c = fnextc(json);
    if (c == '}') {
      // Stop parsing this object
      break;
    } else if (c == ',') {
      skipWhitespace(json);
      char* key = nextString(json);
      skipWhitespace(json);
      fexpectc(json, ':');
      skipWhitespace(json);

      if (strcmp(key, "width") == 0) {
        camera.width = nextNumber(json);
      } else if (strcmp(key, "height") == 0) {
        camera.height = nextNumber(json);
      } else if (strcmp(key, "radius") == 0) {
        objects[currentObject]->sphere.radius = nextNumber(json);
      } else if (strcmp(key, "color") == 0) {
        double* v = nextVector(json);
        for (int i = 0; i < 3; i++) {
          objects[currentObject]->color[i] = v[i];
        }
      } else if (strcmp(key, "position") == 0) {
        double* v = nextVector(json);
        for (int i = 0; i < 3; i++) {
          objects[currentObject]->position[i] = v[i];
        }
      } else if (strcmp(key, "normal") == 0) {
        double* v = nextVector(json);
        for (int i = 0; i < 3; i++) {
          objects[currentObject]->plane.normal[i] = v[i];
        }
      } else {
        fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n", key, line);
        exit(1);
      }
      skipWhitespace(json);
    } else {
      fprintf(stderr, "Error: Unexpected value on line %d.\n", line);
      exit(1);
    }
  }
}

void parseJSON(char* fileName) {
  int c;
  FILE* json = fopen(fileName, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", fileName);
    exit(1);
  }

  skipWhitespace(json);

  // Find the beginning of the list
  fexpectc(json, '[');

  skipWhitespace(json);

  int currentObject = 0;
  while (1) {
    c = fnextc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      return;
    } else if (c == '{') {
      skipWhitespace(json);

      // Parse the object
      char* key = nextString(json);
      if (strcmp(key, "type") != 0) {
        fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
        exit(1);
      }

      skipWhitespace(json);

      fexpectc(json, ':');

      skipWhitespace(json);

      char* value = nextString(json);

      skipWhitespace(json);
      if (strcmp(value, "camera") == 0) {
        objects[currentObject] = malloc(sizeof(Object));
        parseObject(json, currentObject);
      } else if (strcmp(value, "sphere") == 0) {
        objects[currentObject] = malloc(sizeof(Object));
        objects[currentObject]->kind = SPHERE;
        parseObject(json, currentObject);
        currentObject++;
      } else if (strcmp(value, "plane") == 0) {
        objects[currentObject] = malloc(sizeof(Object));
        objects[currentObject]->kind = PLANE;
        parseObject(json, currentObject);
        currentObject++;
      } else {
        fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
        exit(1);
      }

      skipWhitespace(json);
      c = fnextc(json);
      if (c == ',') {
        skipWhitespace(json);
      } else if (c == ']') {
        objects[currentObject] = NULL;
        fclose(json);
        return;
      } else {
        fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
        exit(1);
      }
    }
  }
}

void createImage(int width, int height) {
  double cx = 0;
  double cy = 0;
  double h = camera.height;
  double w = camera.width;

  int M = height;
  int N = width;

  double pixheight = h / M;
  double pixwidth = w / N;

  for (int y = 0; y < M; y++) {
    for (int x = 0; x < N; x++) {
      double Ro[3] = {0, 0, 0};
      double Rd[3] = {
        cx - (w/2) + pixwidth * (x + 0.5),
        cy - (h/2) + pixheight * (y + 0.5),
        1
      };
      normalize(Rd);

      double best_t = INFINITY;
      for (int i = 0; objects[i] != NULL; i++) {
        double t = 0;

        switch(objects[i]->kind) {
          case PLANE:
            break;
          case SPHERE:
            break;
          default:
            fprintf("Error: Object does not have an appropriate kind.");
            exit(1);
        }

        if (t > 0 && t < best_t) best_t = t;
      }
      if (best_t > 0 && best_t != INFINITY) {
        printf("#");
      } else {
        printf(".");
      }
    }
    printf("\n");
  }
}

void displayObjects() {
  printf("Camera:\n\tWidth: %lf\n\tHeight: %lf\n", camera.width, camera.height);
  int i = 0;
  while (objects[i] != NULL) {
    if (objects[i]->kind == PLANE) {
      printf("Plane:\n\tColor.r: %lf\n\tColor.g: %lf\n\tColor.b: %lf\n", objects[i]->color[0], objects[i]->color[1], objects[i]->color[2]);
      printf("\tPosition.x: %lf\n\tPosition.y: %lf\n\tPosition.z: %lf\n", objects[i]->position[0], objects[i]->position[1], objects[i]->position[2]);
      printf("\tNormal.x: %lf\n\tNormal.y: %lf\n\tNormal.z: %lf\n", objects[i]->plane.normal[0], objects[i]->plane.normal[1], objects[i]->plane.normal[2]);
    } else if (objects[i]->kind == SPHERE) {
      printf("Sphere:\n\tColor.r: %lf\n\tColor.g: %lf\n\tColor.b: %lf\n", objects[i]->color[0], objects[i]->color[1], objects[i]->color[2]);
      printf("\tPosition.x: %lf\n\tPosition.y: %lf\n\tPosition.z: %lf\n", objects[i]->position[0], objects[i]->position[1], objects[i]->position[2]);
      printf("\tRadius: %lf\n", objects[i]->sphere.radius);
    }
    i++;
  }
}

int main(int argc, char* argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: raycast width height input.json output.ppm");
    exit(1);
  }

  objects = malloc(sizeof(Object*) * 129);

  parseJSON(argv[3]);
  createImage();

#ifdef DEBUG
  displayObjects();
#endif


  return 0;
}
