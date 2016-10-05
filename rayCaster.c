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


//returnthe square of the supplied number
static inline double sqr(double v) {
   return v*v;
 }

//return the dot product of two double vectors
static inline double dot(double* v1, double* v2){
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

//Subtract one vector from another
static inline double* sub(double* v1, double* v2){
  static double result[3];
  result[0] = v1[0] - v2[0];
  result[1] = v1[1] - v2[1];
  result[2] = v1[2] - v2[2];
  return result;
}

  //normalize a vector
 static inline double normalize(double* v) {
   double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
   v[0] /= len;
   v[1] /= len;
   v[2] /= len;
   return len;
 }

 //get the distance between two points, ie the length of the difference
 static inline double dist(double* v1, double* v2){
   return normalize(sub(v1, v2));
 }

 //check for ray-plane intersections
 double plane_intersection(double* Ro, double* Rd,
          double* P, double* n){
            double a, b;

            a = dot(n, sub(P, Ro));
            b = dot(n, Rd);

            return a/b;
           }
  //check for ray-sphere intersections
 double sphere_intersection(double* Ro, double* Rd,
 			     double* C, double r){

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

   //Make sure the right number of arguments was supplied
   if(argc < 5 || argc > 5) {
     fprintf(stderr, "usage: ./cast width height object-file.json output-file.ppm \n");
     exit(1);
   }

   //check the object file
   char* inFile = argv[3];
   if(strstr(inFile, ".json") == NULL){
     fprintf(stderr, "Please provide a valid json file\n");
     exit(1);
   }

   //read in the scene file, terminate the array
   Object** objects = read_scene(inFile);
   objects[getSize()] = NULL;

   double *curcolor;
   static unsigned char backcolor[3] = {85, 85, 85};

   //get width and height from the input
   int width = atoi(argv[1]);
   int height = atoi(argv[2]);

   if(width < 1 || height < 1){
     printf("Please provide valid width and height values\n");
   }
   printf("%d\n", width);

   //create and allociate space for the ppmimage
   PPMImage image;
   image.width = width;
   image.height = height;
   image.data = malloc(sizeof(RGBPix)* width * height);

   //camera origin
   double cx = 0;
   double cy = 0;

   //view plane dimension variables
   float h;
   float w;

   //First, get the camera object. Then set the view plane dimensions based on
   // the camera's width and height
   for (int i=0; objects[i] != NULL; i += 1){
      if(objects[i]-> kind == 0){
        if(objects[i]->camera.width > 0 && objects[i]->camera.height > 0){
          w = objects[i]->camera.width;
          h = objects[i]->camera.height;
        }
        else {
          printf("Error: please provide a valid camera size\n");
          exit(1);
        }
      }
   }

   //check the ppm image file
   char* outName = argv[4];
   if(strstr(outName, ".ppm") == NULL) {
     fprintf(stderr, "Please provide a valid .ppm file name to write to\n");
     exit(1);
   }

   //open the ppm image
   FILE *outFile = fopen(outName, "wb");

   //Print the header info in the image file
   (void) fprintf(outFile, "P6\n%d %d\n255\n", width, height);

   //pixel width and height
   double pixheight = h / height;
   double pixwidth = w / width;

   //loop through each pixel
   for (int y = 0; y <= height; y += 1) {
     for (int x = 0; x <= width; x += 1) {
       //Ray casting origin
       double Ro[3] = {0, 0, 0};
       //Ray from Ro through the center of the current pixel
       double Rd[3] = {
 	       cx - (w/2) + pixwidth * (x + 0.5),
 	       cy - (h/2) + pixheight * (y + 0.5),
 	       1
       };
       //normalize the ray
      (void) normalize(Rd);

      double best_t = INFINITY;
      //Loop through objects and check for collisions with each
      for (int i=0; objects[i] != NULL; i += 1) {
        double t = 0;
        // run appropriate collision detection based on object type
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

        //If we have a valid collision that is closer than the next best, set our
        //  current pixel color
        if (t > 0 && t < best_t) {
          best_t = t;
          if(objects[i]-> kind == 1) curcolor = objects[i]->sphere.color;
          else if(objects[i]-> kind == 2) curcolor = objects[i]->plane.color;
          else printf("Error: unknown primative type\n");

        }
      }

      //24-bit colors from our double color values
     static unsigned char outcolor[3];
     outcolor[0] = (char) (255 * curcolor[0]);
     outcolor[1] = (char) (255 * curcolor[1]);
     outcolor[2] = (char) (255 * curcolor[2]);

     //Give us the current pixel in terms of the PPMImage data array
     int curPix = (height - y) * width + x;

     //If we got a collision, color it with the current pixel
     if (best_t > 0 && best_t != INFINITY) {
       image.data[curPix].r = outcolor[0];
       image.data[curPix].g = outcolor[1];
       image.data[curPix].b = outcolor[2];
     }
     //otherwise, color it with the background color
     else {
       image.data[curPix].r = backcolor[0];
       image.data[curPix].g = backcolor[1];
       image.data[curPix].b = backcolor[2];
     }
   }
 }
 //Write our data array to the image, then close it out
 fwrite(image.data, 3 * image.width, image.height, outFile);
 (void) fclose(outFile);

 return 0;
 }
