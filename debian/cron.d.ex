#
# Regular cron jobs for the opencv-slideshow package
#
0 4	* * *	root	[ -x /usr/bin/opencv-slideshow_maintenance ] && /usr/bin/opencv-slideshow_maintenance
