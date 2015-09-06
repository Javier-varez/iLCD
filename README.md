# iLCD
Command line utility to convert bitmap files to C arrays, with the purpose of embedding images into a flash memory with very little effort.

The aim of this project is to provide an equivalent application to LCD Assistant, which is only available for Windows machines. 

So far it supports vertical (default) and horizontal byte formatting, and prints out the results to stdout, which can, of course, be redirected to a file. 

###Usage 

To display the usage of iLCD, enter the name of the program:

`iLCD`

The following should appear:

		usage:		iLCD	     	 [-n arrayName] [-v] [-h] file_name

		options:	-v:    			sets vertical byte orientation (default)

					-h:				sets horizontal byte orientation

					-n:arrayName	sets the name of the generated array

Currently, iLCD only supports 1-bit Bitmap files, as it is intended to be used with b/w LCD's mainly. You are welcome to contribute and enhance iLCD.

###Compiling the source code

You can compile the application for your system using the command Make. The application is called iLCD, and you will have to add the container directory to your path enviroment varible if you want it to be accessible from any directory.

To compile the application for Mac OS X move to the directory of the program and simply type:

`Make`

For some linux distributions you may need to tweek the gcc call. For example, if you use Ubuntu, you either need to tweek the Makefile or issue the following command: 

`gcc -Os -o iLCD main.c -lm -Wno-unused-result`
