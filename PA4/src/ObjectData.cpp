#include"ObjectData.h"
#include <string>
#include <list>
#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>
   
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

using namespace std;


// to load the class privates with the passed in params
void ObjectData::addObject(string objID , bool planet, string parent, float scale)
{
    // create struct with default data
    singleObject object;
    // fill with real data
    object.id = objID;
    object.isPlanet = planet;
    object.modelMatrix = glm::mat4(1.0f);
    object.parentPlanet = parent;
    object.scale = scale;
    // put this new item in the list
    listOfObjects.push_back(object);

}


// returns the model matrix of the object with the specified id
glm::mat4 ObjectData::getModelMatrix(string id)
{
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            if ( iter->id == id )
            {
                return  iter->modelMatrix;
            }
        }
    // if not found return blank slate
    return glm::mat4(1.0f);
}


// loop over all objects and update params 
void ObjectData::updateObjects(float dt)
{
    // iterate over all objects
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
           // update rotation
            if ( iter->rotationStatus == 1 )
            {
                 iter->rotationAngle += dt * M_PI/2;
            }
            else if ( iter->rotationStatus == -1 )
            {
                iter->rotationAngle -= dt * M_PI/2;
            }
            else
            {
                 // do nothing, rotation is paused
            }


           // update orbit
            if ( iter->orbitStatus == 1 )
            {
                 iter->orbitAngle += dt * M_PI/2;
            }
            else if ( iter->orbitStatus == -1 )
            {
                iter->orbitAngle -= dt * M_PI/2;
            }
            else
            {
                 // do nothing, rotation is paused
            }

            float finalOrbit = iter->orbitSpeed * iter->orbitAngle;
            float finalAngle = iter->rotationAngle * iter->rotationSpeed;

            // if this is a planet object make orbit about center
            if ( iter->isPlanet )
            {
                // make planet orbit
                iter->modelMatrix = glm::translate( glm::mat4(1.0f), glm::vec3( iter->xOrbitSkew * sin(finalOrbit), 0.0f, iter->yOrbitSkew * cos(finalOrbit))); 
            }

            // if moon make orbit about parent
            else
            {
                glm::mat4 parentPlanetMVP;
                parentPlanetMVP = getModelMatrix(iter->parentPlanet);
                iter->modelMatrix = glm::translate( glm::mat4(1.0f), glm::vec3( parentPlanetMVP[3][0], parentPlanetMVP[3][1], parentPlanetMVP[3][2])); 

                iter->modelMatrix = glm::translate( iter->modelMatrix, glm::vec3( iter->xOrbitSkew * sin(finalOrbit*2), 0.0f, iter->yOrbitSkew * cos(finalOrbit*2)));
            }

            // make both planet and moon rotate
            iter->modelMatrix = glm::rotate( iter->modelMatrix, finalAngle, glm::vec3( 0.0f, 1.0f, 0.0f ) ); 
            // scale object
            iter->modelMatrix = glm::scale( iter->modelMatrix, glm::vec3(iter->scale) ); 

        }


  
}



/////////////////////////////////////////////////
/////////////////////////////////////////////////
// BORING FUNCTION TO MANIPULATE LITTLE PARAMS //
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv//
/////////////////////////////////////////////////

void ObjectData::setSpecialValues(string objectName, float xOrbit, float yOrbit, float rotSpeed, float orbSpeed)
{
    // iterate over all objects
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            // update rotation
            if ( iter->id == objectName )
            {
             iter->xOrbitSkew = xOrbit;
             iter->yOrbitSkew = yOrbit;
             iter->rotationSpeed = rotSpeed;
             iter->orbitSpeed = orbSpeed;
            }

        }
}

void ObjectData::changeRotationDir(string objectName)
{
    // iterate over all objects
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            // update rotation
            if ( iter->id == objectName )
            {
             iter-> rotationStatus = iter-> rotationStatus*-1;
            }

        }

}

void ObjectData::changeOrbitDir(string objectName)
{
    // iterate over all objects
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            // update orbit flag
            if ( iter->id == objectName )
            {
             iter-> orbitStatus = iter-> orbitStatus*-1;
            }

        }
}


void ObjectData::pauseRotation(string objectName)
{
    // iterate over all objects
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            // update rotation
            if ( iter->id == objectName )
            {
             if ( iter-> rotationStatus == 0)
                 {
                  iter-> rotationStatus = 1;
                 }
             else 
                 {
                  iter-> rotationStatus = 0;
                 }
            }

        }

}

void ObjectData::pauseOrbit(string objectName)
{
    // iterate over all objects
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            // update orbit
            if ( iter->id == objectName )
            {
             if ( iter-> orbitStatus == 0)
                 {
                  iter-> orbitStatus = 1;
                 }
             else 
                 {
                  iter-> orbitStatus = 0;
                 }
            }

        }
}

