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

#include <Magick++.h> 

using namespace std;

// returns the object name for the give index
string ObjectData::getObjectName(int index)
{
    int counter = 0;
   
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            if ( counter == index )
            {
                return  iter->id;
            }
         counter++;
        }

    return "na";
}

int ObjectData::getNumObjects()
{
    int size = 0;
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
         size++;
        }
    
    return size;

}

// to load the class privates with the passed in params
void ObjectData::addObject(string objID , bool planet, string parent, float scale, string texture)
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

    loadTexture(objID, texture);

}

void ObjectData::loadTexture(string objID, string textureFileName)
{

    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin() ; iter != listOfObjects.end(); iter++)
        {
            if ( iter->id == objID )
            {
                // load texture image
                Magick::InitializeMagick("");
                Magick::Image image;
                Magick::Blob m_blob;
                try 
                    { 
                     // Read a file into image object 
                     if ( textureFileName != "")
                        {
                         image.read( textureFileName );
                         image.flip();
                         image.write(&m_blob, "RGBA");
                        }
                     else
                        {
                         throw std::invalid_argument("No texture file found");
                        }

                    } 
                catch(exception& tmp) 
                    { 
                     cout << "Error while reading in texture image, texture file not found"  << endl; 
                    } 

                int imageWidth = image.columns();
                int imageHeight = image.rows();


                // setup texture
                glGenTextures(1, &(iter->bTexture)); 
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D,  iter->bTexture);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_blob.data());

                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                }
        }

}

GLuint ObjectData::getTexture(string id)
{
    list<singleObject>::iterator iter;
    for (iter = listOfObjects.begin(); iter != listOfObjects.end(); iter++)
        {
            if ( iter->id == id )
            {
                return  iter->bTexture;
            }
        }
    // if not found return blank slate
    return 0;
}

char ObjectData::getPlanetSet(string id)
{
	list<singleObject>::iterator iter;
	for (iter = listOfObjects.begin(); iter != listOfObjects.end(); iter++)
	    {
	        if ( iter->id == id )
	        {
	            return  iter->planetSet;
	        }
	    }
	return '\0';
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

void ObjectData::setSpecialValues(string objectName, float xOrbit, float yOrbit, float rotSpeed, float orbSpeed, char planetSet)
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
			 iter->planetSet = planetSet;
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

