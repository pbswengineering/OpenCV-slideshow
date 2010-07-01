/* The constant DELAY determines a timeout (the default value is 
   2 seconds) during which there won't be any space key press even 
   if the right color is detected; this avoids going too many slides
   ahead in a little time... ;-) */

#define DELAY 2

/* The constants WIDTH and HEIGHT are the webcam frames size, in
   pixel. */

#define WIDTH 640
#define HEIGHT 480

/* The three color recognition parameters; they can also be adjusted
   while the program is running but modifying them here is the only
   way to keep a persistent configuration. */
#define DEFAULT_GREEN_TRESHOLD 100
#define DEFAULT_GREEN_RED_RATIO_PERCENTAGE 200
#define DEFAULT_GREEN_BLUE_RATIO_PERCENTAGE 200
