#define GLM_FORCE_RADIANS // to stop the whining

#define BIT(x) (1<<(x))
enum collisionTypes
{    
    COL_WALL = BIT(0),
    COL_PADDLE = BIT(1),
    COL_PUCK = BIT(2),
    COL_INVWALL = BIT(3),
    COL_GOAL = BIT(4)
};

#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>
   
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier
 
#include <string>
#include <fstream>
#include <sstream>

#include "ShaderLoader.cpp"
#include "ObjectData.cpp"

#include <vector>

//assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h> //includes the aiScene object 
#include <assimp/postprocess.h> //includes the postprocessing variables for the importer 
#include <assimp/color4.h> //includes the aiColor4 object, which is used to handle the colors from the mesh objects 

#include <Magick++.h>
 
#include <btBulletDynamicsCommon.h>

//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat uv[2];
    GLfloat normal[3];
};

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our geometry
GLuint vbo_cube;// VBO handle for our geometry
GLuint vbo_cylinder;// VBO handle for our geometry
GLuint vbo_sphere;// VBO handle for our geometry
string textureFileName = "";
btTriangleMesh *objTriMesh = NULL;
int globalObjCount = 0; //Count of the objects created.
int viewNum = 1;
float camX, camY, camZ;
bool gameStarted = false;

float gravityX = 0;
float gravityZ = 0;

float offsetX = 0;
float offsetY = 0;
float offsetZ = 0;

//Make an object class containing a list of all objects to store rotation/orbit flags
ObjectData objectsDataList;
 
//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_uv;
GLint loc_normal;

GLint loc_lightType;
float lightTypeVal = 0.f;

GLint loc_lightType2;
float lightTypeVal2 = 0.f;

GLuint aTexture;
bool pauseBool = true;
float scoreOne = 1000;
float scoreTwo = 0;
int cooldown = 0;
int oldMouseX = -1;
int oldMouseY = -1;
float difTime = 0;
float pastElapsedTime = 0;

//transform matrices
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection
//glm::mat4 mvpMoon; // dont need anymore

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);
void demo_menu(int id);
void myMouse(int button, int state, int x, int y);
void handleSpecialKeypress(int key, int x, int y);
void keyBoardUp( int key, int x, int y );
void specialUp(unsigned char key, int x, int y );
string numToStr(int num);
float elapsedDT();

void movePlayerOnePad (int x, int y);

//--Resource management
bool initialize();
void cleanUp();

// custom
void renderObject ( glm::mat4 MVP, int index );
bool loadOBJ(const char * obj, Vertex **data, int id);
int numberTriangles = 0;
void score();
void resetGame();

void checkGoal();

//trying to show text, but not wokring
void renderBitmapString(float x, float y, void *font, string ptrStr);

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2,start;
         
Magick::PixelPacket* loadTextureImage(int &width, int &height);

//Bullet Dynamic World Setup
btBroadphaseInterface *broadphase = new btDbvtBroadphase();
btDefaultCollisionConfiguration *collisionConfiguration = new btDefaultCollisionConfiguration();
btCollisionDispatcher *dispatcher = new btCollisionDispatcher(collisionConfiguration);
btSequentialImpulseConstraintSolver *solver = new btSequentialImpulseConstraintSolver;
btDiscreteDynamicsWorld *dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration); 

//--Main 
int main(int argc, char *argv[])
{
    //cout << "Makes it past Main!" << endl;
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);

    // Name and create the Window
    glutCreateWindow("Labyrinth Demo");

    // create menu for window
    glutCreateMenu(demo_menu);
    glutAddMenuEntry("Play", 1);
    glutAddMenuEntry("Pause", 2);
    glutAddMenuEntry("Reset", 3);
    glutAddMenuEntry("Reset Game", 4);
    glutAddMenuEntry("Quit Program", 5);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    //Bullet Initalization
    dynamicsWorld->setGravity(btVector3(0, -10, 0)); 

    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK) 
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input
    glutSpecialFunc(handleSpecialKeypress); //Called if there is a special keyboard input
    glutMouseFunc(myMouse); //to capture mouse events in glut
    glutSpecialUpFunc(keyBoardUp);
    glutKeyboardUpFunc(specialUp);
    glutPassiveMotionFunc( movePlayerOnePad );     

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now(); //Sets the start time.
        glutMainLoop();
    }

    // Clean up after ourselves
    delete dynamicsWorld;
    delete broadphase;
    delete dispatcher;
    delete collisionConfiguration;
    delete solver;

    dynamicsWorld = NULL;
    dispatcher = NULL;
    broadphase = NULL;
    collisionConfiguration = NULL;
    solver = NULL;

    cleanUp();
    return 0;
}


void score()
{
    if(!pauseBool)
    {
        if(elapsedDT() - difTime >= 1)
        {
            scoreOne -= 1;
            difTime = elapsedDT();
        }
    }

    /*
    if ( playerID == 1 )
        {
         scoreOne++;
         resetGame();
         cooldown = 100;
        }
    else if ( playerID == 2 )
        {
         scoreTwo++;
         resetGame();
         cooldown = 100;
        }
    
    if ( scoreOne > 6)
        {
          cout << endl << "Player 1 wins! Game over" << endl;
          scoreOne = 0;
        }
    else if ( scoreTwo > 6)
        {
          cout << endl << "Player 2 wins! Game over" << endl;
          scoreTwo = 0;
        }
    else 
        {
         system("clear");
         cout << "Goal Scored!" << endl;
         cout << "Current score: " << endl << "Player 1: " << scoreOne << endl;
         cout << "Current score: " << endl << "Player 2: " << scoreTwo << endl;
        }
    */
}

// trying to show text ...      -_-
void renderBitmapString(float x, float y, void *font, string prtStr)
{
    unsigned int counter;
    glRasterPos2f(x, y);
    for (counter = 0; counter < prtStr.length(); counter++) 
    {
        glutBitmapCharacter(font, prtStr[counter]);
    }
} 

//--Implementations
void render()
{
    int counter;
    //clear the screen
    glClearColor(1.0, 1.0, 1.0, 1.0); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //enable the shader program
    glUseProgram(program);

    glUniform1f(loc_lightType, lightTypeVal);
    glUniform1f(loc_lightType2, lightTypeVal2);


    for(counter = 0; counter < globalObjCount; counter++)
    {
        //matrix MVP for planet    
        mvp = projection * view * objectsDataList.getModelMatrix(counter);

        // load each objects mvp
        renderObject( mvp, counter );
    }

    score();                      
  
    //Render Text
    string playerOneStr = "Player One Score:" + numToStr(scoreOne);
    string elapsedTime = "Elapsed Time:" + numToStr(elapsedDT());

    glColor3d(0.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    //glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glScalef(1, -1, 1);
    glTranslatef(0, -h, 0);
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    renderBitmapString(30,40,(void *)GLUT_BITMAP_HELVETICA_18,playerOneStr);
    glColor3d(0.0, 0.0, 0.0);
    renderBitmapString(30,60,(void *)GLUT_BITMAP_HELVETICA_18,elapsedTime);
    glPushMatrix();
    //glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
       

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_uv);
    glDisableVertexAttribArray(loc_normal);

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //swap the buffers
    glutSwapBuffers();
}

string numToStr(int num)
{   
    ostringstream temp;
    temp << num;
    return temp.str();
}

void checkGoal()
{
    /*   
    glm::vec3 puckLoc = glm::vec3(objectsDataList.getModelMatrix(3)[3][0], objectsDataList.getModelMatrix(3)[3][1], objectsDataList.getModelMatrix(3)[3][2]);

    if (puckLoc.y < -20)
        {
            if (puckLoc.x < -10)
                {
                 //score(2);
                }
            else if (puckLoc.x > 10)
                {
                 //score(1);
                }

            resetGame();
        }

    if ( cooldown > 0 )
        {
         cooldown--;
        }
    */
}

void renderObject ( glm::mat4 MVP, int index )
{
    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(MVP));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, objectsDataList.getTexture(index));

    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_uv);
    glEnableVertexAttribArray(loc_normal);

    glBindBuffer(GL_ARRAY_BUFFER, objectsDataList.getVBO(index));
    //set pointers into the vbo for each of the attributes(position and color)
    glVertexAttribPointer( loc_position,//location of attribute
                           3,//number of elements
                           GL_FLOAT,//type
                           GL_FALSE,//normalized?
                           sizeof(Vertex),//stride
                           0);//offset

    // for uv data
    glVertexAttribPointer( loc_uv,
                           2,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex, uv));

    // for normal data
    glVertexAttribPointer( loc_normal,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex, normal));

    glDrawArrays(GL_TRIANGLES, 0, objectsDataList.getTriangles(index));//mode, starting index, count
}

 
void update()
{

    glm::vec3 tempPlayerOne = glm::vec3(objectsDataList.getModelMatrix(1)[3][0], objectsDataList.getModelMatrix(1)[3][1], objectsDataList.getModelMatrix(1)[3][2]);

    //glm::vec3 tempPlayerTwo = glm::vec3(objectsDataList.getModelMatrix(2)[3][0], objectsDataList.getModelMatrix(2)[3][1], objectsDataList.getModelMatrix(2)[3][2]);

    //glm::vec3 puckLoc = glm::vec3(objectsDataList.getModelMatrix(3)[3][0], objectsDataList.getModelMatrix(3)[3][1], objectsDataList.getModelMatrix(3)[3][2]);

    //cout << "Makes it to update!" << endl;
    float dt = getDT();
    int counter; 

    // pass dt to all objects and let them update themselves based on their
    // current flags/status
    if(!pauseBool)
    {
        dynamicsWorld->setGravity(btVector3(gravityX, -10, gravityZ)); 
        dynamicsWorld->stepSimulation(dt, 10.0f);

        for (counter = 0; counter < globalObjCount; counter++)
        {
            objectsDataList.updateObject(counter);
        }
    }

    switch(viewNum)
        {
            case 0:
                 view = glm::lookAt( glm::vec3(camX, camY, camZ), //Eye Position
                                glm::vec3(0, 0.0, 0.0), //Focus point
                                glm::vec3(0.0, 0.0, 1.0)); //Positive Y is up
                break;

            case 1:
                view = glm::lookAt( glm::vec3(tempPlayerOne.x + camX + offsetX, 30.0 + offsetY, tempPlayerOne.z + camZ + offsetZ), //Eye Position
                        glm::vec3(tempPlayerOne.x + offsetX, tempPlayerOne.y + offsetY, tempPlayerOne.z + offsetZ), //Focus point
                        glm::vec3(0.0, 0.0, 1.0)); //Positive Y is up
                break;

            case 2:
                //view = glm::lookAt( glm::vec3(tempPlayerTwo.x, 40.0, tempPlayerTwo.z), //Eye Position
                        //glm::vec3(tempPlayerTwo.x, tempPlayerTwo.y, tempPlayerTwo.z), //Focus point
                        //glm::vec3(0.0, 0.0, 1.0)); //Positive Y is up
                break;

        }

    //checkGoal();
   
    // Update the state of the scene
    glutPostRedisplay();//call the display callback    
}

float elapsedDT()
{
    if(!pauseBool)
    {
        float ret;
        t2 = std::chrono::high_resolution_clock::now();
        ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-start).count();
        pastElapsedTime = ret;
        return ret;
    }
    else
        return pastElapsedTime;
}

bool initialize()    
{
    view = glm::lookAt( glm::vec3(0, 30.0, 0), //Eye Position
                        glm::vec3(0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 0.0, 1.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 


    bool loadedSuccess = true;
    objTriMesh = new btTriangleMesh();
    btCollisionShape *tempShape = NULL;
    btDefaultMotionState *tempMotionState = NULL;
    btScalar tempMass;
    btVector3 tempInertia;
    btRigidBody *tempRigidBody = NULL;
    
    //Collision Masks
    int paddleColidesWith = COL_WALL | COL_PADDLE | COL_PUCK | COL_INVWALL;
    //int goalColidesWith = COL_PUCK;    
    int wallColidesWith = COL_PUCK | COL_PADDLE;
    int invWallColidesWith = COL_PADDLE;
    int puckCollidesWith = COL_PADDLE | COL_GOAL | COL_WALL;


    //TABLE
    globalObjCount++;
    Vertex *geometry;
    loadedSuccess = loadOBJ("LabyrinthBoard.obj", &geometry, 0);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }
 
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry)*numberTriangles*4, geometry, GL_STATIC_DRAW);
    
    //Create collision Objects
    //Initalize the Hockey Table.
    tempShape = new btBvhTriangleMeshShape(objTriMesh, true); //Hockey Table
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0,-15,0)));
    tempMass = 0;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(shapeRigidBodyCI);
    objectsDataList.addObject(0, tempShape, tempMotionState, tempMass, tempInertia, vbo_geometry, numberTriangles, 4.0f, tempRigidBody, textureFileName);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_WALL, wallColidesWith);



    tempRigidBody = NULL;
    delete geometry;
    numberTriangles = 0;
    objTriMesh = NULL;
    
    //Paddle 1
    globalObjCount++;
    Vertex *cylinder;
    //btVector3 cylinderVect = btVector3(1.15f, 1.15f, 1.15f);

    objTriMesh = new btTriangleMesh();
    loadedSuccess = loadOBJ("Earth.obj", &cylinder, 1);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }
 
    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_cylinder);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cylinder);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cylinder)*numberTriangles*4, cylinder, GL_STATIC_DRAW);

    //Initalize the Cylinder
    //tempShape = new btCylinderShape(cylinderVect);    
    tempShape = new btConvexTriangleMeshShape(objTriMesh, true); //Paddle One
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-5.0f,0.0f,0.0f)));
    tempMass = 10;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    tempShape->calculateLocalInertia(tempMass, tempInertia);
    btRigidBody::btRigidBodyConstructionInfo paddleOneRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(paddleOneRigidBodyCI);
    tempRigidBody->setRestitution(btScalar(0.01));
    tempRigidBody->setFriction(btScalar(1));
    objectsDataList.addObject(1, tempShape, tempMotionState, tempMass, tempInertia, vbo_cylinder, numberTriangles, .7f, tempRigidBody, textureFileName);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_PADDLE, paddleColidesWith);

    delete cylinder;
    tempRigidBody = NULL;
    numberTriangles = 0;
    objTriMesh = NULL;
    //objTriMesh = new btTriangleMesh();

/*
    //Paddle 2
    globalObjCount++;
    Vertex *Cube;
    loadedSuccess = loadOBJ("paddleTwo.obj", &Cube, 2);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_cube);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Cube)*numberTriangles*3, Cube, GL_STATIC_DRAW);

    //Initalize the Cylinder
    //tempShape = new btCylinderShape(cylinderVect);    
    tempShape = new btConvexTriangleMeshShape(objTriMesh, true); //Paddle Two
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(5.0f,0.0f,0.0f)));
    tempMass = 10;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    tempShape->calculateLocalInertia(tempMass, tempInertia);
    btRigidBody::btRigidBodyConstructionInfo paddleTwoRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(paddleTwoRigidBodyCI);
    tempRigidBody->setRestitution(btScalar(0.01));
    tempRigidBody->setFriction(btScalar(1));
    objectsDataList.addObject(2, tempShape, tempMotionState, tempMass, tempInertia, vbo_cylinder, numberTriangles, 1.5f, tempRigidBody, textureFileName);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_PADDLE, paddleColidesWith);

    delete Cube;
    tempRigidBody = NULL;
    numberTriangles = 0;
    objTriMesh = NULL;
    objTriMesh = new btTriangleMesh();

    //PUCK
    globalObjCount++;
    Vertex *sphere;
    objTriMesh = new btTriangleMesh();
    loadedSuccess = loadOBJ("Puck.obj", &sphere, 3);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_sphere);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_sphere);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphere)*numberTriangles*3, sphere, GL_STATIC_DRAW);

    //Initalize the Sphere
    //tempShape = new btCylinderShape(cylinderVect); 
    tempShape = new btConvexTriangleMeshShape(objTriMesh, true); //Puck
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0.0f,0.0f,0.0f)));
    tempMass = 1;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    tempShape->calculateLocalInertia(tempMass, tempInertia);
    btRigidBody::btRigidBodyConstructionInfo puckRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(puckRigidBodyCI);
    tempRigidBody->setRestitution(btScalar(0.01));
    tempRigidBody->setFriction(btScalar(.01));
    objectsDataList.addObject(3, tempShape, tempMotionState, tempMass, tempInertia, vbo_sphere, numberTriangles, 1.5f, tempRigidBody, textureFileName);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_PUCK, puckCollidesWith);
    

    delete sphere;
    numberTriangles = 0;
    objTriMesh = NULL;

    ///Walls
    globalObjCount++;
    btVector3 middle_wall = btVector3(0.0f, 40.0f, 40.0f);
    tempShape = new btBoxShape(middle_wall);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 4, 0.0f)));
    tempMass = 0;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    btRigidBody::btRigidBodyConstructionInfo invWallRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(invWallRigidBodyCI);
    objectsDataList.addObject(4, tempShape, tempMotionState, tempMass, tempInertia, vbo_sphere, numberTriangles, 4.0f, tempRigidBody, textureFileName);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_INVWALL, invWallColidesWith);


    // right goal
    globalObjCount++;
    btVector3 right_wall = btVector3(0.0f, 20.0f, 10.0f);
    tempShape = new btBoxShape(right_wall);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-31.5f,4.0f,0.0f)));
    tempMass = 0;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    btRigidBody::btRigidBodyConstructionInfo rightWallRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(rightWallRigidBodyCI);
    objectsDataList.addObject(5, tempShape, tempMotionState, tempMass, tempInertia, vbo_sphere, numberTriangles, 4.0f, tempRigidBody, textureFileName);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_INVWALL, invWallColidesWith );


    // left goal
    globalObjCount++;
    btVector3 left_wall = btVector3(0.0f, 20.0f, 10.0f);
    tempShape = new btBoxShape(left_wall);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(31.5f,4.0f,0.0f)));
    tempMass = 0;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    btRigidBody::btRigidBodyConstructionInfo leftWallRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(leftWallRigidBodyCI);
    objectsDataList.addObject(6, tempShape, tempMotionState, tempMass, tempInertia, vbo_sphere, numberTriangles, 4.0f, tempRigidBody, textureFileName);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_INVWALL, invWallColidesWith );
*/

    // clean
    tempShape = NULL;
    tempMotionState = NULL;

    //--Load vertex shader and fragment shader from 2 text files
    ShaderLoader loader("vertexShader.txt", "fragmentShader.txt");
    program = loader.LoadShader();
    
    //Now we set the locations of the attributes and uniforms
    //this allows us to access them easily while rendering
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        //return false;
    }

    loc_uv = glGetAttribLocation(program,
                    const_cast<const char*>("v_uv"));
    if(loc_uv == -1)
    {
        std::cerr << "[F] V_UV NOT FOUND" << std::endl;
        //return false;
    }

    loc_normal = glGetAttribLocation(program,
                    const_cast<const char*>("v_normal"));
    if(loc_normal == -1)
    {
        std::cerr << "[F] normal NOT FOUND" << std::endl;
        //return false;
    }

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        //return false;
    }

    loc_lightType = glGetUniformLocation(program, "v_lightType");
    if(loc_lightType == -1)
    {
        std::cerr << "[F] lightType NOT FOUND" << std::endl;
        //return false;
    }

    loc_lightType2 = glGetUniformLocation(program, "v_lightType2");
    if(loc_lightType2 == -1)
    {
        std::cerr << "[F] lightType2 NOT FOUND" << std::endl;
        //return false;
    }

    //lights
    GLfloat white[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat cyan[] = {.8f, 0.f, 0.f, 1.f};

    glMaterialfv(GL_FRONT, GL_DIFFUSE, cyan);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    //glMaterialfv(GL_FRONT, GL_AMBIENT, white);

    GLfloat shininess[] = {20};
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

    GLfloat lightpos[] = {0, 5, 0, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);


    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    //glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 20.0);
    //GLfloat spot_direction[] = { -1.0, -1.0, 0.0 };
    //glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);

    //and its done
    return true;
}


//Changed to make the object a complex collision shape.
bool loadOBJ(const char * obj, Vertex **data, int id)
{

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(obj, aiProcess_Triangulate);
    btVector3 triArray[3]; 
   
    if ( scene==NULL )
        {
         return false;
        }

    aiMesh *mesh = scene->mMeshes[0];

    float *vertexArray;
    float *normalArray;
    float *uvArray;
 
    int numVerts;
 
    // extract data
    numVerts = mesh->mNumFaces*3;
    *data = new Vertex[numVerts];
    vertexArray = new float[mesh->mNumFaces*3*3];
    normalArray = new float[mesh->mNumFaces*3*3];
    uvArray = new float[mesh->mNumFaces*3*2];
    aiVector3D uv;
    aiVector3D pos;

    for(unsigned int i=0;i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        
        for(int j=0;j<3;j++)
        {
            //cout << "Line 568" << endl;
            if (mesh->HasTextureCoords( 0 ) )
            {
                 uv = mesh->mTextureCoords[0][face.mIndices[j]];
                 memcpy(uvArray,&uv,sizeof(float)*2);
                 uvArray+=2;
            }
             
            aiVector3D normal = mesh->mNormals[face.mIndices[j]];
            memcpy(normalArray,&normal,sizeof(float)*3);
            normalArray+=3;
            
            pos = mesh->mVertices[face.mIndices[j]];
            triArray[j] = btVector3(pos.x, pos.y, pos.z);
            memcpy(vertexArray,&pos,sizeof(float)*3);
            vertexArray+=3;
            //cout << "Line 585" << endl;
        }
        objTriMesh->addTriangle(triArray[0], triArray[1], triArray[2]); 
    }
     
    uvArray-=mesh->mNumFaces*3*2;
    normalArray-=mesh->mNumFaces*3*3;
    vertexArray-=mesh->mNumFaces*3*3;

    int vertIndex = 0;
    int uvIndex = 0;
    // For each vertex of each triangle
    for( int i=0; i < numVerts; i++ )
        {
        
         // update our vertex data
         data[0][i].position[0] = vertexArray[vertIndex];
         data[0][i].position[1] = vertexArray[vertIndex+1];
         data[0][i].position[2] = vertexArray[vertIndex+2];

         // update uv coords
         data[0][i].uv[0] = uvArray[uvIndex];
         data[0][i].uv[1] = uvArray[uvIndex+1];

         // update normals coords
         data[0][i].normal[0] = normalArray[vertIndex];
         data[0][i].normal[1] = normalArray[vertIndex+1];
         data[0][i].normal[2] = normalArray[vertIndex+2];

         uvIndex+=2;
         vertIndex+=3;       
         numberTriangles++;
        }

    // get text file name
    const aiMaterial* material = scene->mMaterials[1]; 
    aiString path;  // filename
    if( mesh->HasTextureCoords( 0 ) )
        { 
         material->GetTexture(aiTextureType_DIFFUSE, 0, &path); 
         textureFileName = path.data; 
        }
    else
        {
         std::cerr << "obj loader warrning: no texture file specified in obj file" << endl;
        }


    return true;
}
 

void reshape(int n_w, int n_h)
{
    w = n_w;
    h = n_h;
    //Change the viewport to be correct
    glViewport( 0, 0, w, h);
    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(w)/float(h), 0.01f, 100.0f);
}

void cleanUp()
{
    // Clean up, Clean up
    int counter;

    for(counter = 0; counter < globalObjCount; counter++)
    {
        objectsDataList.dealocateObject(counter);
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo_geometry);
}

//returns the time delta
float getDT()
{
    float ret;
    t2 = std::chrono::high_resolution_clock::now();
    ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
    t1 = std::chrono::high_resolution_clock::now();
    return ret;
}

void resetGame()
{
    float dt;
    start = std::chrono::high_resolution_clock::now(); 
    scoreOne = 1000;
    
    cout << "globalObjCount" << globalObjCount << endl;
// reset items
    for (int index = 0; index < globalObjCount; index++)
        {

        // if on the puck
        if ( index != 0 )
            {
            btVector3 startPos;

            if (objectsDataList.getRigidBody(index) && objectsDataList.getRigidBody(index)->getMotionState())
                {
	               
                     btTransform puckTrans;
		             puckTrans.setIdentity();

                     if ( index == 1 )
                        {
                         startPos = btVector3(-5.0f, -3.0f, -5.0f);
                        }
                     
	                 puckTrans.setOrigin(startPos);
		             btDefaultMotionState* puckMotionState = new btDefaultMotionState(puckTrans);
		             objectsDataList.getRigidBody(index)->setMotionState(puckMotionState);
                     objectsDataList.getRigidBody(index)->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
                     objectsDataList.getRigidBody(index)->setAngularVelocity(btVector3(0,0,0));
                }
            }
        }
    dt = getDT();
    dynamicsWorld->stepSimulation(dt, 10.0f);
}

///////////////////////////////////////
// human interaction functions below //   
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv///
void movePlayerOnePad (int x, int y) 
{
    btRigidBody* tempRB = objectsDataList.getRigidBody(1);
    btVector3 tempVec3; 
    tempVec3 = tempRB->getLinearVelocity();
    int xValue = 0;
    int yValue = 0;
    int zValue = 0;

   int topFourth = h / 4;
   int bottomFourth = topFourth * 3;

   int leftFourth = w / 4;
   int rightFourth = leftFourth * 3;

   if ( x < leftFourth )
        {
            xValue += 5;
            xValue = xValue + tempVec3.getX();
            if(xValue > 15)
                xValue = 15;
        }
    
    if ( x > rightFourth )
        {
            xValue -= 5;
            xValue = xValue + tempVec3.getX();
            if(xValue < -15)
                xValue = -15;
        }            

   if ( y < topFourth )
        {
            zValue += 5;
            zValue = zValue + tempVec3.getZ();
            if(zValue > 15)
                zValue = 15;
        }

    if ( y > bottomFourth )
        {
            zValue -= 5;
            zValue = zValue + tempVec3.getZ();
            if(zValue < -15)
                zValue = -15;
        }


    tempRB->setLinearVelocity(btVector3(xValue, yValue , zValue));
}

//player 1 arrow keys ( being replaced with mouse )
void handleSpecialKeypress(int key, int x, int y) 
{
    //glm::mat4 tmpMVP =  glm::translate( objectsDataList.getModelMatrix(1), glm::vec3( 4 * sin(90), 0.0f, 4 * cos(90)));
    //glm::vec3 tempPlayerOne = glm::vec3(objectsDataList.getModelMatrix(1)[3][0], objectsDataList.getModelMatrix(1)[3][1], objectsDataList.getModelMatrix(1)[3][2]);
    //objectsDataList.setMVP( 1, tmpMVP );

//btRigidBody* tempRB1 = objectsDataList.getRigidBody(0);
//btTransform tr;
//tr.setIdentity();
//btQuaternion quat;
//quat.setEuler(50,90,180); //or quat.setEulerZYX depending on the ordering you want
//tr.setRotation(quat);

//tempRB1->setCenterOfMassTransform(tr);



//btQuaternion qNewOrientation;
//qNewOrientation.setEuler(0, 0, 0);
   
   // Update box model
   //m_pModel->SetPosition(cons(transform.getOrigin()));

    //objectsDataList.getRigidBody(1)->activate();
   //objectsDataList.getRigidBody(1)->applyTorque(btVector3(200, 50, 49));

    //btMatrix3x3 orn = objectsDataList.getRigidBody(0)->getWorldTransform().getBasis(); //get basis of world transformation
    //orn *= btMatrix3x3(btQuaternion(btVector3(0,1,0), 20));     //Multiply it by rotation matrix
    //objectsDataList.getRigidBody(0)->getWorldTransform().setBasis(orn); //set new rotation for the object


//btTransform transform = btTransform(objectsDataList.getRigidBody(0)->getWorldTransform() ); 
//btQuaternion quat;
//quat.setEuler(30,0,0);
//transform.setRotation(quat);
//objectsDataList.getRigidBody(0)->activate();
//objectsDataList.getRigidBody(0)->setWorldTransform(transform);

    if(key == GLUT_KEY_LEFT)
        {
            camX -= .5;
            gravityX += 1;
        }
    
    if(key == GLUT_KEY_RIGHT)
        {
            camX += .5; 
            gravityX -= 1;
        }

    if(key == GLUT_KEY_UP)
        {
            camZ -= .5;
            gravityZ += 1;
        }

    if(key == GLUT_KEY_DOWN)
        {
            camZ += .5;
            gravityZ -= 1;
        }
}

// release of player 1 arrow keys ( being replaced with mouse )
void keyBoardUp( int key, int x, int y )
{
    //btRigidBody* tempRB1 = objectsDataList.getRigidBody(1);
    if(!pauseBool)
    {
     if ( key == GLUT_KEY_LEFT || GLUT_KEY_RIGHT || key == GLUT_KEY_UP || key == GLUT_KEY_DOWN )
        {
         //tempRB1->setLinearVelocity(btVector3(0, 0, 0));
        }
    }
}

// release of wasd keys
void specialUp(unsigned char key, int x, int y )
{
    //btRigidBody* tempRB2 = objectsDataList.getRigidBody(2);
    if(!pauseBool)
    {
     if ( key == 'a' || key == 'w' || key == 's' || key == 'd' )
        {
         //tempRB2->setLinearVelocity(btVector3(0, 0, 0));
        } 
    }
}

// press of wasd keys
void keyboard(unsigned char key, int x_pos, int y_pos)
{        
    int xValue = 0;
    int yValue = 0;
    int zValue = 0;

    if(key == 27)//ESC
    {
        exit(0);
    }

    // move camera
    else if(key == 'a')
    {
        offsetX ++;
    }
    else if(key == 'd')
    {
        offsetX --;
    }
    else if(key == 's')
    {
        offsetZ --;
    }
    else if(key == 'w')
    {
        offsetZ ++;
    }

    else if(key == 'q')
        {
            offsetY ++;
        }

    else if(key == 'e')
        {
            offsetY --;
        }


    // cycle light types
    else if(key == 'n')
    {
        if ( lightTypeVal == 0 )
        lightTypeVal++;
    }
    else if(key == 'm')
    {
        if ( lightTypeVal == 1 )
        lightTypeVal--;
    }


    // cycle light2 types
    else if(key == 'j')
    {
        if ( lightTypeVal2 == 0 )
        lightTypeVal2++;
    }
    else if(key == 'k')
    {
        if ( lightTypeVal2 == 1 )
        lightTypeVal2--;
    }


    if(key == '.')
        camY += 1;

    if(key == '/')
        camY -= 1;

    if(key == 'f')
        {
            camX = 0;
            camY = 30;
            camZ = 0;
            viewNum = 0;
        }

    if(key == '1')
        {
            viewNum = 1;
        }
    if(key == '2')
        {
            viewNum = 2;
        }
} 

void myMouse(int button, int state, int x, int y)
{ 
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        cout << "Left Button Pressed" << endl;
        

        btRigidBody* tempRB = objectsDataList.getRigidBody(1);
        btVector3 tempVec3;
        int xValue = 0;
        int yValue = 0;
        int zValue = 0;

        int topFourth = h / 4;
        int bottomFourth = topFourth * 3;

        int leftFourth = w / 4;
        int rightFourth = leftFourth * 3;

        // move up
        if ( y < topFourth )
        {
        zValue += 15;
        }
        // move down
        if ( y > bottomFourth )
        {
        zValue -= 15;
        }
        // move left
        if ( x < leftFourth )
        {
        xValue += 15;
        }
        // move rith
        if ( x > rightFourth )
        {
        xValue -= 15;
        }

        tempVec3 = tempRB->getLinearVelocity();
        tempRB->setLinearVelocity(btVector3(xValue + tempVec3.getX(), yValue + tempVec3.getY(), zValue + tempVec3.getZ()));
        
    } 
}
 
// glut menu callback
void demo_menu(int id)
{
    switch(id)
    {
        case 1:
        pauseBool = false;
        if(!gameStarted)
        {
        start = std::chrono::high_resolution_clock::now();
        gameStarted = true;
        }
        break;

        case 2:
        pauseBool = true;
        break;

        case 3:
        resetGame();
        break;

        case 4:
        resetGame();
        scoreOne = 0;
        scoreTwo = 0;
        break;

        case 5:
        exit(0);
        break; 
    }
    glutPostRedisplay();
}


