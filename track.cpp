// -*- mode: c++; coding: utf-8 -*-

/* Detects a particular green shade on the webcam and simulates the
   space key press; useful, among other things, to go through the
   slides while doing a presentation. :-)
   
   The function test_color determines what kind of color will be
   detected; if you want to tune the program you want to mess with
   said function.
   
   To better tune this program for your needs you should take a look
   and, if needed, change the constants on config.h.
   
   In order to compile this program you need:
   1) OpenCV
   2) XLib development files

   You may need to adjust the Makefile in order to use these libraries
   on your system, of course.
   
   Enjoy!
   
   Copyright (c) 2010 Paolo Bernardi <paolo.bernardi@gmx.it>
   
   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following
   conditions:
  
   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.
  
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
*/
   
#include <iostream>
#include <sstream>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <ctime>
#include <cstdlib>

#include "config.h"

using namespace std;

#define RED_MASK 0x00
#define GREEN_MASK 0xFF
#define BLUE_MASK 0x00

#define SEARCHRANGE 1000

#define ENTER 10
#define ESCAPE 27

#define GENERATED_KEYCODE XK_space

#define WHITE cvScalar(255, 255, 255)

#define RADIUS (HEIGHT / 12)

int green_treshold = 100;
int gr_ratio = 200;
int gb_ratio = 200;

int searchx = -1;
int searchy = -1;

int lastx = -100;
int lasty = -100;
bool idle = false;
bool calibration = true;
int slides = 0;

time_t last_time = 0;

CvFont regular_font, big_font;
stringstream top_text, bottom_text;
CvScalar bg_color;

CvPoint topleft;
CvPoint bottomright;

IplImage *frame = 0;

XKeyEvent create_key_event(Display *display, Window &win, Window &winRoot, bool press, int keycode, int modifiers)
{
    XKeyEvent event;

    event.display     = display;
    event.window      = win;
    event.root        = winRoot;
    event.subwindow   = None;
    event.time        = CurrentTime;
    event.x           = 1;
    event.y           = 1;
    event.x_root      = 1;
    event.y_root      = 1;
    event.same_screen = True;
    event.keycode     = XKeysymToKeycode(display, keycode);
    event.state       = modifiers;

    if(press)
        event.type = KeyPress;
    else
        event.type = KeyRelease;

    return event;
}

void simulate_keypress(Display *display) {
// Get the root window for the current display.
    Window winRoot = XDefaultRootWindow(display);

// Find the window which has the current keyboard focus.
    Window winFocus;
    int revert;
    XGetInputFocus(display, &winFocus, &revert);
    
// Send a fake key press event to the window.
    XKeyEvent event = create_key_event(display, winFocus, winRoot, true, GENERATED_KEYCODE, 0);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
    
// Send a fake key release event to the window.
    event = create_key_event(display, winFocus, winRoot, false, GENERATED_KEYCODE, 0);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
}

template < class T > class Image {

private:
    IplImage * imgp;

public:
    Image(IplImage * img = 0) {
        imgp = img;
    }
    ~Image() {
        imgp = 0;
    }
    void operator=(IplImage * img) {
        imgp = img;
    }
    inline T *operator[] (const int rowIndex) {
        return ((T *) (imgp->imageData + rowIndex * imgp->widthStep));
    }
};

typedef struct {
    unsigned char b, g, r;
} RgbPixel;

typedef Image < RgbPixel > RgbImage;

void calibrate() {
    cout << "CALIBRATION" << endl;
    
    RgbImage image(frame);
    
    int ymin = HEIGHT / 2 - RADIUS;
    int ymax = HEIGHT / 2 + RADIUS;
    int xmin = WIDTH / 2 - RADIUS;
    int xmax = WIDTH / 2 + RADIUS;
    
    int gt_count = 0;
    int gr_count = 0;
    int gb_count = 0;

    for (int i = ymin; i < ymax; i++) {
        for (int j = xmin; j < xmax; j++) {
            int r = image[i][j].r;
            int g = image[i][j].g;
            int b = image[i][j].b;
            green_treshold += g;
            ++gt_count;
            
            if (r != 0) {
                gr_ratio += g * 100 / r;
                ++gr_count;
            }
            
            if (b != 0) {
                gb_ratio += g * 100 / b;
                ++gb_count;
            }
        }
    }
    green_treshold /= gt_count;
    gr_ratio /= gr_count;
    gb_ratio /= gb_count;
    cout << "GT: " << green_treshold << endl;
    cout << "GR: " << gr_ratio << endl;
    cout << "GB: " << gb_ratio << endl;
}

inline bool test_color(float r, float g, float b)
{
    return (g > green_treshold)
        && ((((float)g) / r) > (((float)gr_ratio) / 100))
        && ((((float)g) / b) > (((float)gb_ratio) / 100));
}

void detect_green(Display * dpy)
{
    int xmin, xmax, ymin, ymax;

    int top = HEIGHT, bottom = -1, left = WIDTH, right = -1;

    if (searchx == -1 || searchy == -1) {
        xmin = 0;
        ymin = 0;
        xmax = WIDTH;
        ymax = HEIGHT;

    } else {
        xmin = MAX(0, searchx - SEARCHRANGE);
        xmax = MIN(WIDTH, searchx + SEARCHRANGE);
        ymin = MAX(0, searchy - SEARCHRANGE);
        ymax = MIN(HEIGHT, searchy + SEARCHRANGE);
    }

    RgbImage image(frame);

    int targetxt = 0, targetyt = 0;
    int count = 0;

    for (int i = ymin; i < ymax; i++) {
        for (int j = xmin; j < xmax; j++) {
            int r = image[i][j].r;
            int g = image[i][j].g;
            int b = image[i][j].b;
            if (test_color(r, g, b)) {
                image[i][j].r = RED_MASK;
                image[i][j].g = GREEN_MASK;
                image[i][j].b = BLUE_MASK;
                targetyt += i;
                targetxt += j;
                count++;

                top = MIN(top, i);
                bottom = MAX(bottom, i);
                left = MIN(left, j);
                right = MAX(right, j);
            }
        }
    }

    if (count) {
        searchx = targetxt / count;
        searchy = targetyt / count;

        float targetx = (float) targetxt / count / WIDTH;
        float targety = (float) targetyt / count / HEIGHT;

        int screenx = (int) (targetx * 1480.0 - 100);
        int screeny = (int) (targety * 1000.0 - 100);

        topleft = cvPoint(left, top);
        bottomright = cvPoint(right, bottom);

        CvRect box;

        box.x = (int) (left + ((right - left) / 2));
        box.y = (int) (top + ((bottom - top) / 2));

        if (lastx >= 0 && lasty >= 0 && lastx != left && lasty != top && !idle && time(NULL) - last_time > DELAY) {
            ++slides;
            bottom_text.str("");
            bottom_text << slides;
            if (slides != 1)
            bottom_text << " slides done          Press ESC to quit";
            else
            bottom_text << " slide done           Press ESC to quit";
            
            simulate_keypress(dpy);
            XFlush(dpy);

            idle = true;
            last_time = time(NULL);
        }

        lastx = left;
        lasty = top;

        XFlush(dpy);

    } else {
        searchx = -1;
        searchy = -1;
        idle = false;
    }

}

void on_mouse(int event, int x, int y, int flags, void *param)
{
    if (event == CV_EVENT_MOUSEMOVE && frame != 0) {
        RgbImage image(frame);
        int r = image[y][x].r;
        int g = image[y][x].g;
        int b = image[y][x].b;

        top_text.str("");
        top_text << "xy(" << x << ", " << y << ") ";
        top_text << "rgb(" << r << ", " << g << ", " << b << ")  ";
        top_text << "g/r = " << (r == 0 ? 0 : (double)g / r * 100) << "  ";
        top_text << "g/b = " << (b == 0 ? 0 : (double)g / b * 100) << "  ";
    }
}

void on_green_treshold_calibrated(int position) {
    green_treshold = position;
}

void on_gr_ratio_calibrated(int position) {
    gr_ratio = position;
}

void on_gb_ratio_calibrated(int position) {
    gb_ratio = position;
}

int main(int argc, char **argv)
{
    bool done = false;
    int key;

    Display *dpy = XOpenDisplay(0);
    if(dpy == NULL) {
        cerr << "Could not open display :0" << endl;
        return 1;
    }

    CvCapture *capture = cvCaptureFromCAM(0);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, WIDTH);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

    if (!capture) {
        cerr << "Could not initialize the webcam..." << endl;
        return 1;
    }

    cvNamedWindow("Slideshow", 1);
    cvSetMouseCallback("Slideshow", on_mouse, 0);
    cvCreateTrackbar("Green treshold", "Slideshow", &green_treshold, 255, on_green_treshold_calibrated);
    cvCreateTrackbar("Green/Red percentage", "Slideshow", &gr_ratio, 500, on_gr_ratio_calibrated);
    cvCreateTrackbar("Green/Blue percentage", "Slideshow", &gb_ratio, 500, on_gb_ratio_calibrated);

    cvInitFont(&regular_font, CV_FONT_HERSHEY_SIMPLEX, 0.4, 0.4, 0, 1);
    cvInitFont(&big_font, CV_FONT_HERSHEY_TRIPLEX, 1, 1, 0, 1);

    frame = cvQueryFrame(capture);

    int step = frame->widthStep / sizeof(uchar);

    top_text << "Move the mouse over the picture to see informations about the single pixels";
    bottom_text << "0 slides done          Press ESC to quit";

    while (!done) {
        frame = cvQueryFrame(capture);
        if (!frame)
            break; // No frame, no party!

        cvFlip(frame, 0, 1);
        detect_green(dpy);
        if (idle) {
            cvRectangle(frame, topleft, bottomright, WHITE, 3, 8, 0);
            bg_color = cvScalar(0, 0, 255);
        } else {
            bg_color = cvScalar(0, 0, 0);
        }
        
        // Top banner: informations about the pixel above the mouse pointer
        cvRectangle(frame, cvPoint(0, 0), cvPoint(WIDTH, 25), bg_color, CV_FILLED, 8, 0);
        cvPutText(frame , top_text.str().c_str(), cvPoint(5, 15), &regular_font, WHITE);

        // Bottom banner: how many slides did you show?
        cvRectangle(frame, cvPoint(0, HEIGHT - 25), cvPoint(WIDTH, HEIGHT), bg_color, CV_FILLED, 8, 0);
        cvPutText(frame , bottom_text.str().c_str(), cvPoint(5, HEIGHT - 10), &regular_font, WHITE);

        if (calibration) {
            // Calibration target
            cvCircle(frame, cvPoint(WIDTH / 2, HEIGHT / 2), RADIUS, WHITE, 2);
            cvLine(frame, cvPoint(WIDTH / 2 - RADIUS - RADIUS / 5, HEIGHT / 2), cvPoint(WIDTH / 2 + RADIUS + RADIUS / 5, HEIGHT / 2), WHITE, 2);
            cvLine(frame, cvPoint(WIDTH / 2, HEIGHT / 2 - RADIUS - RADIUS / 5), cvPoint(WIDTH / 2, HEIGHT / 2 + RADIUS + RADIUS / 5), WHITE, 2);
            cvPutText(frame , "Press ENTER for calibration", cvPoint(WIDTH / 2 - 220, HEIGHT / 2 + RADIUS * 3), &big_font, WHITE);
        }
        
        cvShowImage("Slideshow", frame);

        // User input handling
        int key = cvWaitKey(5) & 0xFF;        
        switch (key) {
            case ENTER: 
                if (calibration) {
                    calibration = false;
                    calibrate();
                }
                break;
            case ESCAPE: 
                done = true;
                break;
        }
    }

    cvReleaseCapture(&capture);
    //cvReleaseImage(&frame); // It crashes badly... Why?
    cvDestroyWindow("Slideshow");
    XCloseDisplay(dpy);

    return 0;
}
