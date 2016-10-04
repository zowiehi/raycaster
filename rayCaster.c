/*
Created by Zowie Haugaard
10/4/16
Ray Caster
  This Ray caster creates a .ppm image from a .json style list of objects
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 #include <math.h>
 #include "parser.h"


 static inline double sqr(double v) {
   return v*v;
 }

static inline double dot(double* v1, double* v2){
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

 double* sub(double* v1, double* v2){
  static double result[3];
  result[0] = v1[0] - v2[0];
  result[1] = v1[1] - v2[1];
  result[2] = v1[2] - v2[2];
  return result;
}

 static inline double normalize(double* v) {
   double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
   v[0] /= len;
   v[1] /= len;
   v[2] /= len;
   return len;
 }

 static inline double dist(double* v1, double* v2){
   return normalize(sub(v1, v2));
 }

 double plane_intersection(double* Ro, double* Rd,
          double* P, double* n){
            double a, b;

            a = dot(n, sub(P, Ro));
            b = dot(n, Rd);

            return a/b;
           }

 double sphere_intersection(double* Ro, double* Rd,
 			     double* C, double r) {
             // Step 1. Find the equation for the object you are
             // interested in..  (e.g., sphere)
             //
             // x^2 + y^2 + z^2 = r^2
             //
             // Step 2. Parameterize the equation with a center point
             // if needed
             //
             // (x-Cx)^2 + (y-Cy) + (z-Cz)^2 = r^2
             //
             // Step 3. Substitute the eq for a ray into our object
             // equation.
             //
             // (Rox + t*Rdx - Cx)^2 + (Roy + t*Rdy - Cy)^2) (Roz + t*Rdz - Cz)^2 - r^2 = 0
             //
             // Step 4. Solve for t.
             //
             // Step 4a. Rewrite the equation (flatten).
             //
             // -r^2 +
             // t^2 * Rdx^2 +
             // t^2 * Rdy^2 +
             // t^2 * Rdz^2 +
             // 2*t * Rox * Rdx -
             // 2*t * Rdx * Cx +
             // 2*t * Roy * Rdy -
             // 2*t * Rdy * Cy +
             // 2*t * Roz * Rdz -
             // 2*t * Rdz * Cz +
             // Rox^2 -
             // 2*Rox*Cx +
             // Cx^2 +
             // Roy^2 -
             // 2*Roy*Cy +
             // Cy^2 +
             // Roz^2 -
             // 2*Roz*Cz +
             // Cz^2 = 0
             //
             // Steb 4b. Rewrite the equation in terms of t.
             //
             // t^2 * (Rdx^2 + rdy^2 + Rdz^2) +
             // t * (2 * (Rox * Rdx - Rdx * Cx + Roy * Rdy - Rdy * Cy + Roz * Rdz - Rdz * Cz)) +
             // Rox^2 - 2*Rox*Cx + Cx^2 + Roy^2 - 2*Roy*Cy + Cy^2 + Roz^2 - 2*Roz*Cz + Cz^2 - r^2 = 0
             //
             // Use the quadratic equation to solve for t..
             double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
             double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[1] * Rd[1] - Rd[1] * C[1] + Ro[2] * Rd[2] - Rd[2] * C[2]));
             double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + Ro[1] - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

             double det = sqr(b) - 4 * a * c;
             if (det < 0) return -1;

             det = sqrt(det);

             double t0 = (-b - det) / (2*a);
             if (t0 > 0) return t0;

             double t1 = (-b + det) / (2*a);
             if (t1 > 0) return t1;

             return -1;

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


 //represents a single pixel object
 typedef struct RGB {
   unsigned char r, g, b;
 }RGBPix;

 //this struct is used to store the entire image data, along with the width and height
 typedef struct Image {
   int width, height;
   RGBPix *data;
 }PPMImage;

 int main(int argc, char *argv[]) {

   Object** objects = read_scene("objects.json");
   printf("%d\n", getSize());
   objects[getSize()] = NULL;

   if(objects[0]->kind != 0){
     printf("Error: please provide a camera object\n");
     exit(1);
   }
   if(objects[0]->camera.width > 0 && objects[0]->camera.height > 0){

   }


   //Make sure the right number of arguments was supplied
   if(argc < 5 || argc > 5) {
     perror("usage: ./cast width height object-file.json output-file.ppm \n");
     exit(1);
   }

   FILE *outFile = fopen(argv[3], "wb");

   double *curcolor;

   int width = 300;
   int height = 300;
   printf("%d\n", width);

   //create and allociate space for the ppmimage
   PPMImage image;
   image.width = width;
   image.height = height;
   image.data = malloc(sizeof(RGBPix)* width * height);

   static unsigned char backcolor[3] = {85, 85, 85};

   double cx = 0;
   double cy = 0;
   double h;
   double w;

   if(objects[0]->kind != 0){
     printf("Error: please provide a camera object\n");
     exit(1);
   }
   if(objects[0]->camera.width > 0 && objects[0]->camera.height > 0){
     w = objects[0]->camera.width;
     h = objects[0]->camera.height;
   }
   else printf("Error: please provide a valid camera size\n");

   int M = height;
   int N = width;

   (void) fprintf(outFile, "P6\n%d %d\n255\n", N, M);

   double pixheight = h / height;
   double pixwidth = w / width;
   for (int y = 0; y <= height; y += 1) {
     for (int x = 0; x <= width; x += 1) {
       double Ro[3] = {0, 0, 0};
       double Rd[3] = {
 	       cx - (w/2) + pixwidth * (x + 0.5),
 	       cy - (h/2) + pixheight * (y + 0.5),
 	       1
       };
      (void) normalize(Rd);

       double best_t = INFINITY;
       //check for collisions with each of our objects
       for (int i=0; objects[i] != NULL; i += 1) {
 	       double t = 0;

 	       switch (objects[i]->kind) {
           case 0:
            break;
 	        case 1:
            t = sphere_intersection(Ro, Rd, objects[i]->sphere.position, objects[i]->sphere.radius);
            break;
          case 2:
            t = plane_intersection(Ro, Rd, objects[i]->plane.position, objects[i]->plane.normal);
            break;
          default:
           printf("err\n");
           exit(1);
 	       }

 	if (t > 0 && t < best_t) {
    best_t = t;
    if(objects[i]-> kind == 1) curcolor = objects[i]->sphere.color;
    else if(objects[i]-> kind == 2) curcolor = objects[i]->plane.color;
    else printf("Error: unknown primative type\n");
  }
       }
       static unsigned char outcolor[3];
       outcolor[0] = (char) (255 * curcolor[0]);
       outcolor[1] = (char) (255 * curcolor[1]);
       outcolor[2] = (char) (255 * curcolor[2]);
       if (best_t > 0 && best_t != INFINITY) {
         image.data[(height - y) * width + x].r = outcolor[0];
         image.data[(height - y) * width + x].g = outcolor[1];
         image.data[(height - y) * width + x].b = outcolor[2];
       }
       else {
         image.data[(height - y) * width + x].r = backcolor[0];
         image.data[(height - y) * width + x].g = backcolor[1];
         image.data[(height - y) * width + x].b = backcolor[2];
       }
     }

   }
   fwrite(image.data, 3 * image.width, image.height, outFile);
   (void) fclose(outFile);

   return 0;
 }
