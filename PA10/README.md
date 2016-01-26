Alex Ward, Jeffrey Bouchard PA10
========================================


Build Notes:
---------------------

*This example requires GLM*
*On ubuntu it can be installed with this command*

>$ sudo apt-get install libglm-dev

*On a Mac you can install GLM with this command(using homebrew)*
>$ brew install glm

To build this example just 

>$ cd build
>$ make

The excutable will be put in bin

This program uses Magic++ and Assimp, please ensure both are installed on your machine before running.


Run Notes:
------------------------------

Enter the file name of the object that you would like to load as command line paramaters. For example: './Matrix.exe Cube.obj'

Hit the 'a' key to toggle ambiant light on and off. 
Hit the 'd' key to toggle diffuse light on and off.
Hit the 's' key to toggle specular light on and off.
Hit the 'p' key to toggle spot light on and off.
Hit the 'y' key to combine all light sources.

Press the right mouse button to open the programs menu.

Note: The ambient light is tinted red while the diffuse light is tinted blue to allow the graders to clealry see the different lightings.
