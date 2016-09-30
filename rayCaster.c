/*
Created by Zowie Haugaard
9/16/16
PPM Converter
  This program is used to convert between the ascii and binary versions of the
  .ppm image file format.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <math.h>
 #include <parser.h>

 // Plymorphism in C
 typedef struct {
   int kind; // 0 = camera, 1 = sphere, 2 = plane
   double color[3];
   union {
     struct {
       double width;
       double height;
     } camera;
     struct {
       double color[3];
       double position[3];
       double radius;
     } sphere;
     struct {
       double color[3];
       double position[3];
       double nomral[3];
     } plane;
   };
 } Object;

 Object* read_scene(char* filename) {
   int c, i = 0;
   FILE* json = fopen(filename, "r");
   Object** objects;
   objects = malloc(sizeof(Object*)*2);

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
     if (c == ']') {
       fprintf(stderr, "Error: This is the worst scene file EVER.\n");
       fclose(json);
       return;
     }
     if (c == '{') {
       skip_ws(json);
       i+= 1;

       // Parse the object
       char* key = next_string(json);
       if (strcmp(key, "type") != 0) {
 	fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
 	exit(1);
       }
       objects[i-1] = malloc(sizeof(Object));

       skip_ws(json);

       expect_c(json, ':');

       skip_ws(json);

       char* value = next_string(json);

       if (strcmp(value, "camera") == 0) {
         objects[i-1]->kind = 0;
       } else if (strcmp(value, "sphere") == 0) {
         objects[i-1]->kind = 1;
       } else if (strcmp(value, "plane") == 0) {
         objects[i-1]->kind = 2;
       } else {
 	fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
 	exit(1);
       }

       skip_ws(json);

       while (1) {
 	// , }
 	c = next_c(json);
 	if (c == '}') {
 	  // stop parsing this object
 	  break;
 	} else if (c == ',') {
 	  // read another field
 	  skip_ws(json);
 	  char* key = next_string(json);
 	  skip_ws(json);
 	  expect_c(json, ':');
 	  skip_ws(json);

 	  if (strcmp(key, "width") == 0){
      if(objects[i-1].kind != 0) {
        fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
        exit(1);
      }
      else{
        objects[i-1]->camera.width =next_number(json);
      }
    }
 	      else if(strcmp(key, "height") == 0){

          if(objects[i-1].kind != 0) {
            fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
            exit(1);
          }
          else{
            objects[i-1]->camera.height =next_number(json);
          }
        }
 	      else if(strcmp(key, "radius") == 0){
          {
            if(objects[i-1].kind != 1) {
              fprintf(stderr, "Error, type/key missmatch, line number %d \n", line);
              exit(1);
            }
            else{
              objects[i-1]->sphere.radius =next_number(json);
            }
          }
 	    double value = next_number(json);
 	  } else if ((strcmp(key, "color") == 0) ||
 		     (strcmp(key, "position") == 0) ||
 		     (strcmp(key, "normal") == 0)) {
 	    double* value = next_vector(json);
 	  } else {
 	    fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
 		    key, line);
 	    //char* value = next_string(json);
 	  }
 	  skip_ws(json);
 	} else {
 	  fprintf(stderr, "Error: Unexpected value on line %d\n", line);
 	  exit(1);
 	}
       }
       skip_ws(json);
       c = next_c(json);
       if (c == ',') {
 	// noop
 	skip_ws(json);
       } else if (c == ']') {
 	fclose(json);
 	return;
       } else {
 	fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
 	exit(1);
       }
     }
   }
 }

 static inline double sqr(double v) {
   return v*v;
 }


 static inline void normalize(double* v) {
   double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
   v[0] /= len;
   v[1] /= len;
   v[2] /= len;
 }


 double cylinder_intersection(double* Ro, double* Rd,
 			     double* C, double r) {
   // Step 1. Find the equation for the object you are
   // interested in..  (e.g., cylinder)
   //
   // x^2 + z^2 = r^2
   //
   // Step 2. Parameterize the equation with a center point
   // if needed
   //
   // (x-Cx)^2 + (z-Cz)^2 = r^2
   //
   // Step 3. Substitute the eq for a ray into our object
   // equation.
   //
   // (Rox + t*Rdx - Cx)^2 + (Roz + t*Rdz - Cz)^2 - r^2 = 0
   //
   // Step 4. Solve for t.
   //
   // Step 4a. Rewrite the equation (flatten).
   //
   // -r^2 +
   // t^2 * Rdx^2 +
   // t^2 * Rdz^2 +
   // 2*t * Rox * Rdx -
   // 2*t * Rdx * Cx +
   // 2*t * Roz * Rdz -
   // 2*t * Rdz * Cz +
   // Rox^2 -
   // 2*Rox*Cx +
   // Cx^2 +
   // Roz^2 -
   // 2*Roz*Cz +
   // Cz^2 = 0
   //
   // Steb 4b. Rewrite the equation in terms of t.
   //
   // t^2 * (Rdx^2 + Rdz^2) +
   // t * (2 * (Rox * Rdx - Rdx * Cx + Roz * Rdz - Rdz * Cz)) +
   // Rox^2 - 2*Rox*Cx + Cx^2 + Roz^2 - 2*Roz*Cz + Cz^2 - r^2 = 0
   //
   // Use the quadratic equation to solve for t..
   double a = (sqr(Rd[0]) + sqr(Rd[2]));
   double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[2] * Rd[2] - Rd[2] * C[2]));
   double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

   double det = sqr(b) - 4 * a * c;
   if (det < 0) return -1;

   det = sqrt(det);

   double t0 = (-b - det) / (2*a);
   if (t0 > 0) return t0;

   double t1 = (-b + det) / (2*a);
   if (t1 > 0) return t1;

   return -1;
 }

 int main() {

   Object** objects;
   objects = malloc(sizeof(Object*)*2);
   objects[0] = malloc(sizeof(Object));
   objects[0]->kind = 0;
   objects[0]->cylinder.radius = 2;
   // object[0]->teapot.handle_length = 2;
   objects[0]->cylinder.center[0] = 0;
   objects[0]->cylinder.center[1] = 0;
   objects[0]->cylinder.center[2] = 20;
   objects[1] = NULL;

   double cx = 0;
   double cy = 0;
   double h = 0.7;
   double w = 0.7;

   int M = 20;
   int N = 20;

   double pixheight = h / M;
   double pixwidth = w / N;
   for (int y = 0; y < M; y += 1) {
     for (int x = 0; x < N; x += 1) {
       double Ro[3] = {0, 0, 0};
       // Rd = normalize(P - Ro)
       double Rd[3] = {
 	cx - (w/2) + pixwidth * (x + 0.5),
 	cy - (h/2) + pixheight * (y + 0.5),
 	1
       };
       normalize(Rd);

       double best_t = INFINITY;
       for (int i=0; objects[i] != 0; i += 1) {
 	double t = 0;

 	switch(objects[i]->kind) {
 	case 0:
 	  t = cylinder_intersection(Ro, Rd,
 				    objects[i]->cylinder.center,
 				    objects[i]->cylinder.radius);
 	  break;
 	default:
 	  // Horrible error
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

   return 0;
 }
