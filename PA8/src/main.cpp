#define GLM_FORCE_RADIANS // to stop the whining

#define BIT(x) (1<<(x))
enum collisionTypes
{    
    COL_WALL = BIT(0),
    COL_SHAPE = BIT(1)
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

//Make an object class containing a list of all objects to store rotation/orbit flags
ObjectData objectsDataList;
 
//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_uv;
GLuint aTexture;

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

//--Resource management
bool initialize();
void cleanUp();

// custom
void renderObject ( glm::mat4 MVP, int index );
bool loadOBJ(const char * obj, Vertex **data);
int numberTriangles = 0;

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;
         
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
    glutCreateWindow("Hockey Table with Bullet");

    // create menu for window
    glutCreateMenu(demo_menu);
    glutAddMenuEntry("Quit Program", 1);
    //glutAddMenuEntry("Play/Pause Object Rotation", 2);
    //glutAddMenuEntry("Play/Pause Object Orbit", 3);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    //Bullet Initalization
    dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

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

//--Implementations
void render()
{
    //cout << "Makes it to Render!" << endl;
    int counter;
    //clear the screen
    glClearColor(0.2, 0.2, 0.2, 1.0); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //enable the shader program
    glUseProgram(program);


    for(counter = 0; counter < globalObjCount; counter++)
    {
        //matrix MVP for planet    
        mvp = projection * view * objectsDataList.getModelMatrix(counter);

        // load each objects mvp
        renderObject( mvp, counter );
    }
                           
    //swap the buffers
    glutSwapBuffers();
}

void renderObject ( glm::mat4 MVP, int index )
{
    //cout << "Makes it to renderObject!" << endl;
    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(MVP));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, aTexture);


    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_uv);
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

    glDrawArrays(GL_TRIANGLES, 0, objectsDataList.getTriangles(index));//mode, starting index, count

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_uv);

}


void update()
{
    //cout << "Makes it to update!" << endl;
    float dt = getDT();
    int counter; 

    // pass dt to all objects and let them update themselves based on their
    // current flags/status

    //cout << "Gets to before stepsim" << endl;  
    dynamicsWorld->stepSimulation(dt, 10.0f);

    //cout << "GLOBAL COUNT" << globalObjCount << endl;

    for (counter = 0; counter < globalObjCount; counter++)
    {
        objectsDataList.updateObject(counter);
    }

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}


bool initialize()    
{
    //cout << "Makes it to initalize!" << endl;
    bool loadedSuccess = true;
    char defualtOBJName[] = "iceRink.obj"; //Change to change the default loaded object.
    btCollisionShape *tempShape = NULL;
    btDefaultMotionState *tempMotionState = NULL;
    btScalar tempMass;
    btVector3 tempInertia;
    btRigidBody *tempRigidBody = NULL;
    
    //Collision Masks
    int shapeColidesWith = COL_WALL | COL_SHAPE;
    int wallColidesWith = COL_SHAPE;    
    

//TABLE
    globalObjCount++;
    Vertex *geometry;
    btVector3 tempVect = btVector3(0.0f, 1.0f, 0.0f);
    btScalar planeScaler = 3;
    objTriMesh = new btTriangleMesh();
    loadedSuccess = loadOBJ(defualtOBJName, &geometry);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }

    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry)*numberTriangles*3, geometry, GL_STATIC_DRAW);

    //Create collision Objects
    //Initalize the Hockey Table.
    tempShape = new btBvhTriangleMeshShape(objTriMesh, true); //Hockey Table
    //tempShape = new btStaticPlaneShape(tempVect, planeScaler);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0,-15,0)));
    tempMass = 0;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    tempShape->calculateLocalInertia(tempMass, tempInertia);
    btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(shapeRigidBodyCI);
    objectsDataList.addObject(0, tempShape, tempMotionState, tempMass, tempInertia, vbo_geometry, numberTriangles, 1, tempRigidBody);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_WALL, wallColidesWith);

    tempRigidBody = NULL;
    delete geometry;
    numberTriangles = 0;
    delete objTriMesh;
    objTriMesh = new btTriangleMesh();

//cout << "Makes it past loading the table!" << endl;

/*
//CUBE
    globalObjCount++;
    Vertex *Cube;
    objTriMesh = new btTriangleMesh();
    btVector3 squareVect = btVector3(0.6f, 0.6f, 0.6f);
    loadedSuccess = loadOBJ("Cube.obj", &Cube);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }
 
    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_cube);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Cube)*numberTriangles*3, Cube, GL_STATIC_DRAW);

    //Initalize the Cube.
    //tempShape = new btBvhTriangleMeshShape(objTriMesh, true);//Cube

    tempShape = new btBoxShape(squareVect);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0.5f,4.0f,0.0f)));
    tempMass = 1;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    objectsDataList.addObject(1, tempShape, tempMotionState, tempMass, tempInertia, vbo_cube, numberTriangles, .5f);
    dynamicsWorld->addRigidBody(objectsDataList.getRigidBody(1), COL_SHAPE, shapeColidesWith);

    delete Cube;
    numberTriangles = 0;
    delete objTriMesh;
    objTriMesh = new btTriangleMesh();
 
*/

//CYLINDER
    //Paddle 1
    globalObjCount++;
    Vertex *cylinder;
    btVector3 cylinderVect = btVector3(0.6f, 0.6f, 0.6f);
    objTriMesh = new btTriangleMesh();
    loadedSuccess = loadOBJ("Paddle.obj", &cylinder);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_cylinder);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cylinder);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cylinder)*numberTriangles*3, cylinder, GL_STATIC_DRAW);

    //Initalize the Cylinder
    //tempShape = new btBvhTriangleMeshShape(objTriMesh, true); //cylinder
    tempShape = new btCylinderShape(cylinderVect);    
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-2.0f,1.0f,2.0f)));
    tempMass = 1;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    tempShape->calculateLocalInertia(tempMass, tempInertia);
    btRigidBody::btRigidBodyConstructionInfo paddleOneRigidBodyCI(tempMass, tempMotionState, tempShape, tempInertia);
    tempRigidBody = new btRigidBody(paddleOneRigidBodyCI);
    objectsDataList.addObject(1, tempShape, tempMotionState, tempMass, tempInertia, vbo_cylinder, numberTriangles, .5f, tempRigidBody);
    dynamicsWorld->addRigidBody(tempRigidBody, COL_SHAPE, shapeColidesWith);

    //delete cylinder;
    tempRigidBody = NULL;
    numberTriangles = 0;
    delete objTriMesh;
    objTriMesh = new btTriangleMesh();

    cout << "Loaded Paddle 1" << endl;
/*
    //Paddle 2
    globalObjCount++;
    Vertex *cylinder;
    objTriMesh = new btTriangleMesh();
    loadedSuccess = loadOBJ("Paddle.obj", &cylinder);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }

     Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_cylinder);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cylinder);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cylinder)*numberTriangles*3, cylinder, GL_STATIC_DRAW);

    Initalize the Cylinder
    tempShape = new btBvhTriangleMeshShape(objTriMesh, true); //cylinder
    tempShape = new btCylinderShape(cylinderVect);    
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(2.0f,1.0f,2.0f)));
    tempMass = 1;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    objectsDataList.addObject(2, tempShape, tempMotionState, tempMass, tempInertia, vbo_cylinder, numberTriangles, .5f);
    dynamicsWorld->addRigidBody(objectsDataList.getRigidBody(2), COL_SHAPE, shapeColidesWith);

    delete cylinder;
    numberTriangles = 0;
    delete objTriMesh;
    objTriMesh = new btTriangleMesh();

//SPHERE
    globalObjCount++;
    Vertex *sphere;
    objTriMesh = new btTriangleMesh();
    btScalar sphereScaler = 1;
    loadedSuccess = loadOBJ("Earth.obj", &sphere);
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
    //tempShape = new btBvhTriangleMeshShape(objTriMesh, true); //Sphere
    tempShape = new btSphereShape(sphereScaler); 
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(4.0f,1.0f,1.0f)));
    tempMass = 1;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    objectsDataList.addObject(1, tempShape, tempMotionState, tempMass, tempInertia, vbo_sphere, numberTriangles, .5f);
    dynamicsWorld->addRigidBody(objectsDataList.getRigidBody(1), COL_SHAPE, shapeColidesWith);
 
    delete sphere;
    numberTriangles = 0;
*/

///Walls
/*
    globalObjCount++;
    btVector3 left_wall = btVector3(4.f, 4.f, 4.f);
    tempShape = new btBoxShape( left_wall);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(11, 0, 0.0f)));
    tempMass = 100;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    objectsDataList.addObject(4, tempShape, tempMotionState, tempMass, tempInertia, 0, 1, .5f);
    dynamicsWorld->addRigidBody(objectsDataList.getRigidBody(4), COL_SHAPE, shapeColidesWith);

    globalObjCount++;
    btVector3 top_wall = btVector3(6.f, 6.f, 6.f);
    tempShape = new btBoxShape( top_wall);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, 9.5f)));
    tempMass = 100;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    objectsDataList.addObject(5, tempShape, tempMotionState, tempMass, tempInertia, 0, 1, .5f);
    dynamicsWorld->addRigidBody(objectsDataList.getRigidBody(5), COL_SHAPE, shapeColidesWith);

    globalObjCount++;
    btVector3 bottom_wall = btVector3(6.f, 6.f, 6.f);
    tempShape = new btBoxShape( bottom_wall);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, -9.f)));
    tempMass = 100;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    objectsDataList.addObject(6, tempShape, tempMotionState, tempMass, tempInertia, 0, 1, .5f);
    dynamicsWorld->addRigidBody(objectsDataList.getRigidBody(6), COL_SHAPE, shapeColidesWith);

    globalObjCount++;
    btVector3 right_wall = btVector3(4.f, 4.f, 4.f);
    tempShape = new btBoxShape( right_wall);
    tempMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-7, 0, 0.0f)));
    tempMass = 100;
    tempInertia = btVector3(0.0f, 0.0f, 0.0f);
    objectsDataList.addObject(7, tempShape, tempMotionState, tempMass, tempInertia, 0, 1, .5f);
    dynamicsWorld->addRigidBody(objectsDataList.getRigidBody(7), COL_SHAPE, shapeColidesWith);
*/

 


    //Clean Up
    //tempShape = NULL;
    //delete objTriMesh;
    //objTriMesh = NULL;
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
        return false;
    }

    loc_uv = glGetAttribLocation(program,
                    const_cast<const char*>("v_uv"));
    if(loc_uv == -1)
    {
        std::cerr << "[F] V_UV NOT FOUND" << std::endl;
        return false;
    }

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }
    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(.5, 7.0, 0), //Eye Position
                        glm::vec3(.5, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 0.0, 1.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 


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
    glGenTextures(1, &aTexture); 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, aTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_blob.data());

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //and its done
    return true;
}

//Changed to make the object a complex collision shape.
bool loadOBJ(const char * obj, Vertex **data)
{
    //cout << "Makes to loadOBJ" << endl;
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(obj, aiProcess_Triangulate);
    btVector3 triArray[3]; 
    //btTriangleMesh* tempTriMesh;
    //tempTriMesh = *objTriMesh;

    if ( scene==NULL )
        {
         return false;
        }

    aiMesh *mesh = scene->mMeshes[0];

    float *vertexArray;
    //not yet// float *normalArray;
    float *uvArray;


    int numVerts;

    // extract data
    numVerts = mesh->mNumFaces*3;
    *data = new Vertex[numVerts];
    vertexArray = new float[mesh->mNumFaces*3*3];
    //normalArray = new float[mesh->mNumFaces*3*3];
    uvArray = new float[mesh->mNumFaces*3*2];


    for(unsigned int i=0;i<mesh->mNumFaces;i++)
    {
        const aiFace& face = mesh->mFaces[i];
        
        for(int j=0;j<3;j++)
        {
            if ( mesh->HasTextureCoords( 0 ) )
            {
                 aiVector3D uv = mesh->mTextureCoords[0][face.mIndices[j]];
                 memcpy(uvArray,&uv,sizeof(float)*2);
                 uvArray+=2;
            }
             
            /*
            aiVector3D normal = mesh->mNormals[face.mIndices[j]];
            memcpy(normalArray,&normal,sizeof(float)*3);
            normalArray+=3;
            */
            aiVector3D pos = mesh->mVertices[face.mIndices[j]];
            triArray[j] = btVector3(pos.x, pos.y, pos.z);
            memcpy(vertexArray,&pos,sizeof(float)*3);
            vertexArray+=3;
        }
        objTriMesh->addTriangle(triArray[0], triArray[1], triArray[2]); 
    }
     
    uvArray-=mesh->mNumFaces*3*2;
    //normalArray-=mesh->mNumFaces*3*3;
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
         std::cerr << "obj loader warning: no texture file specified in obj file" << endl;
        }

    //cout << "Makes it to the end of LoadOBJ!" << endl;
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


///////////////////////////////////////
// human interaction functions below //   
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv///

void handleSpecialKeypress(int key, int x, int y) 
{
    btRigidBody* tempRB = objectsDataList.getRigidBody(2);

    switch (key) 
    {
    case GLUT_KEY_LEFT:
            tempRB->applyCentralImpulse( btVector3( 2, 0.f, 0.f ) );
        //Move circle X 
        break;  

    case GLUT_KEY_RIGHT:
            tempRB->applyCentralImpulse( btVector3( -2, 0.f, 0.f ) );
        //Move circle -X
        break;

    case GLUT_KEY_UP:
            tempRB->applyCentralImpulse( btVector3( 0, 0.f, 2.f ) );
        //Move circle -Z
        break;

    case GLUT_KEY_DOWN:
            tempRB->applyCentralImpulse( btVector3( 0, 0.f, -2.f ) );
        //Move circle Z
        break;
    
    }
}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    btRigidBody* tempRB = objectsDataList.getRigidBody(3);
    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
    switch (key)
    {
        case 'a':
        tempRB->applyForce(btVector3( 10, 0.f, 0.f ), btVector3(0.0f,0.0f,0.0f));
        //Move cylinder X
        break;
        case 'd':
        tempRB->applyForce(btVector3( -10, 0.f, 0.f ), btVector3(0.0f,0.0f,0.0f));
        //Move cylinder -X
        break;
        case 'w':
        tempRB->applyForce(btVector3( 0.0f, 0.f, 10.0f ), btVector3(0.0f,0.0f,0.0f));
        //Move cylinder -Z
        break;
        case 's':
        tempRB->applyForce(btVector3( 0.0, 0.f, -10.0f ), btVector3(0.0f,0.0f,0.0f));
        //Move cylinder Z
        break;
    }
} 

void myMouse(int button, int state, int x, int y)
{ 
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
         cout << "Moused just got clicked" << endl;
    } 
}

// glut menu callback
void demo_menu(int id)
{
    switch(id)
    {
         case 1:
         exit(0);
         break;
         //case 2:
         //break;
         //case 3:
         //break;

    }
    glutPostRedisplay();
}


