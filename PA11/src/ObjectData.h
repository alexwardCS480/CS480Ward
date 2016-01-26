#include <string>
#include <list>
#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>
   
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

#include <btBulletDynamicsCommon.h>

using namespace std;

//struct to hold all flags and current data of an object
struct singleObject
    {
    // flags/vals
    int idNum = 0;
    btCollisionShape *shape = NULL;
    btDefaultMotionState *motionState = NULL;
    btScalar mass = 1.0;
    btVector3 inertia = btVector3(0.0f, 0.0f, 0.0f);
    btRigidBody* rigidBody;
    GLuint bTexture;
    string texture;
    
    // IDs
    glm::mat4 modelMatrix;

    // special params
    btVector3 startPos = btVector3(0.0f, 0.0f, 0.0f);

    GLuint VBO;

    int numbTri = 0;
    float scale = 1;

    };

//class to hold all objects
class ObjectData 
{
 public:

    //Clean Up
    void dealocateObject(int id);

    // create a new object
    void addObject(int objNum, btCollisionShape* &shape, btDefaultMotionState* &motionState, btScalar mass, btVector3 inertia, GLuint VBO, int numbTri, float scale, btRigidBody* &rigidBody, string texture);

    void loadTexture(int objID, string textureFileName);
    GLuint getTexture(int id);   

    // run through all objects and update val based on dt
    void updateObject(int id);

    // helper functions for main program to manipulate flags/data
    glm::mat4 getModelMatrix(int id);

    void setSpecialValues(int id, btVector3 startPos);

    btRigidBody* getRigidBody(int id);

    void updateInertia(int id, btVector3 inertia);

    GLuint getVBO (int id );

    int getTriangles (int id );

    void setMVP( int id,  glm::mat4 mvp);

  private:
    // the actual list of all objects, private so must be accessed throught 
    // helper functions
    list<singleObject> listOfObjects;

};
