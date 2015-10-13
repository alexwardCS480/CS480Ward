Alex Ward PA2
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

Enter the file name of the object that you would like to load as command line paramaters. For example: './Matrix.exe capsule.obj'

The capsule rotates and orbits so the grader can see all sides. 

Press the right mouse button to open the programs menu. From the menu you can pause the capsule's rotation and orbit.

Press the left mouse button to reverse the capsule's orbit.
