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



Run Notes:
------------------------------

Press the right mouse button to open the programs menu. From the menu you can pause the planet's and moon's orbit and rotation.



Press the left mouse button to reverse the planet's orbit.

Press the 'a' key to reverse the planets's rotation.

Press the 'b' key to reverse the moon's rotation.

Press the 'c' key to reverse the planet's orbit.

Press the 'd' key to reverse the moon's rotation.



Press the 'left' key to reverse the planets's rotation.

Press the 'right' key to reverse the planet's orbit.

Press the 'up' key to reverse the moon's rotation.

Press the 'down' key to reverse the moon's orbit.


*Be sure the planet's and moon's rotation is not paused before pressing any key, reversing the rotation will have not effect if the cube's rotation is paused*

*Be sure the planet's orbit is not paused before pressing the right mouse button, reversing the orbit will have not effect if the cube's orbit is paused*

Comments to graders:
------------------------------
I took a rather aggressive sive approach to put the objects with their own data in a class system. This makes it easy for me to add planets and moons with unique params (rotation speed, parent planet, etc) with just a few lines of code. If this was a bad idea just let me know and I can go back no problem.



