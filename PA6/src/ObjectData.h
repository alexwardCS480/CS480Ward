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

//struct to hold all flags and current data of an object
struct singleObject
    {
    // flags/vals
    int rotationStatus = 1;
    int orbitStatus = 1;
    float rotationAngle = 0.0;
    float orbitAngle = 0.0;
    float scale = 1.0;

    // IDs
    std::string id;
    bool isPlanet = true;
    string parentPlanet;
    glm::mat4 modelMatrix;

    // special params
    float xOrbitSkew = 4.0;
    float yOrbitSkew = 4.0;
    float rotationSpeed = 1.0;
    float orbitSpeed = 1.0;

    };

//class to hold all objects
class ObjectData 
{
 public:

    // create a new object
    void addObject(string objID, bool planet, string parent, float scale);
    
    // run through all objects and update val based on dt
    void updateObjects(float dt);

    // helper functions for main program to manipulate flags/data
    glm::mat4 getModelMatrix(string id);

    void changeRotationDir(string objectName);
    void changeOrbitDir(string objectName);

    void pauseRotation(string objectName);
    void pauseOrbit(string objectName);
    void setSpecialValues(string objectName, float xOrbit, float yOrbit, float rotSpeed, float orbSpeed);

  private:
    // the actual list of all objects, private so must be accessed throught 
    // helper functions
    list<singleObject> listOfObjects;

};
