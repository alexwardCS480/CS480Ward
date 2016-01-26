#define GLM_FORCE_RADIANS // to stop the whining

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
//magic
#include <Magick++.h> 


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
GLuint vbo_ring;
//GLuint vbo_ring;
string textureFileName = "";

//Make an object class containing a list of all objects to store rotation/orbit flags
ObjectData objectsDataList;
 
//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_uv;

//transform matrices
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip

string planetOfChoice = "earth";
int planetOfChoiceNum = 3;  
float planetOffset = .0005;
float heightOffset = 0;

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);
void demo_menu(int id);
void myMouse(int button, int state, int x, int y);
void handleSpecialKeypress(int key, int x, int y);

//--Resource management
bool initialize(char* fileName);
void cleanUp();

// custom
void renderObject (  string objectName );
bool loadOBJ(const char * obj, Vertex **data);
int numberTriangles = 0;
bool planetZoom = false;

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;
         
Magick::PixelPacket* loadTextureImage(int &width, int &height); 
void renderAllObjects();
void loadObjectDataFromFile( string fileName );

void DrawCircle(float cx, float cy, float r, int num_segments);
int highlightCount = 0;
int highlightNumber = 0;

//--Main 
int main(int argc, char *argv[])
{
    // Initialize glut
    char defaultObj[] = "Earth.obj";
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);

    // Name and create the Window
    glutCreateWindow("Solar System");

    // create menu for window
    glutCreateMenu(demo_menu);
    glutAddMenuEntry("Quit Program", 1);
    glutAddMenuEntry("Scaled Planets/Zoomed Plaents", 2);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

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
    glutSpecialFunc(handleSpecialKeypress);
    //to capture mouse events in glut
    glutMouseFunc(myMouse);

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize(defaultObj); //argv[1]);
    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
    return 0;
}

//--Implementations
void render()
{ 
    string camName = planetOfChoice + "Cam";

    //clear the screen
    glClearColor(0.0, 0.0, 0.0, 1.0); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //enable the shader program
    glUseProgram(program);

    if(planetZoom == false)
    {
        glm::vec3 tempCamCords = glm::vec3(objectsDataList.getModelMatrix(camName)[3][0], objectsDataList.getModelMatrix(camName)[3][1], objectsDataList.getModelMatrix(camName)[3][2]);

        glm::vec3 tempPlanetCords = glm::vec3(objectsDataList.getModelMatrix(planetOfChoice)[3][0], objectsDataList.getModelMatrix(planetOfChoice)[3][1], objectsDataList.getModelMatrix(planetOfChoice)[3][2]);

        // lets look at the earth for some reason
        view = glm::lookAt( glm::vec3( tempCamCords.x + planetOffset, tempCamCords.y + heightOffset, tempCamCords.z + planetOffset), //Eye Position 
                            glm::vec3( tempPlanetCords.x, tempPlanetCords.y, tempPlanetCords.z), //Focus point 
                            glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
    }
    else if (planetZoom == true)
    {
        glm::vec3 tempCamCords = glm::vec3(objectsDataList.getModelMatrix("sun")[3][0], objectsDataList.getModelMatrix("sun")[3][1], objectsDataList.getModelMatrix("sun")[3][2]);

        view = glm::lookAt( glm::vec3( tempCamCords.x + 10, tempCamCords.y + 12, tempCamCords.z + 10), //Eye Position 
                            glm::vec3( tempCamCords.x, tempCamCords.y, tempCamCords.z), //Focus point 
                            glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
    }
       
    renderAllObjects();

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_uv);
                           
    //swap the buffers
    glutSwapBuffers();
}
 
void renderAllObjects()
{
    string satRing = "saturnRings";

    for ( int index = 0; index < objectsDataList.getNumObjects(); index++)
        {
            if(objectsDataList.getPlanetSet(objectsDataList.getObjectName(index)) == 'A')
            {
                renderObject(objectsDataList.getObjectName(index));
            }
            else if(planetZoom == false && objectsDataList.getPlanetSet(objectsDataList.getObjectName(index)) == 'N')
            {
                renderObject(objectsDataList.getObjectName(index));        
            }
            else if(planetZoom == true && objectsDataList.getPlanetSet(objectsDataList.getObjectName(index)) == 'Z')
            {
                renderObject(objectsDataList.getObjectName(index));        
            }

            // draw triangle to highlight planet if user pressed a key
            if ( index == highlightNumber && highlightCount > 0)
            {
             DrawCircle(0, 0, 20, 3);  
             highlightCount--;
            }
        }

}

void renderObject ( string objectName )
{
    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr( projection * view * objectsDataList.getModelMatrix(objectName) ));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, objectsDataList.getTexture(objectName));

    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_uv);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);

    // to render the ring object around saturn
    if ( objectName == "satRing" || objectName == "satRingZoom" )
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_ring);
    }

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

    glDrawArrays(GL_TRIANGLES, 0, numberTriangles);//mode, starting index, count

}

 
void update()
{
    float dt = getDT();
    
    // pass dt to all objects and let them update themselves based on their
    // current flags/status
    objectsDataList.updateObjects(dt/2);

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}


void loadObjectDataFromFile( string fileName )
{
    // vars
    string objectName, parentName, textureName;
    bool isParent;
    float scale, xOrbit, yOrbit, rotSpeed, orbSpeed;
    char planetSet;
    
    std::ifstream fin;
    fin.open (fileName, std::ifstream::in);

    // read all data from file
    while (fin.good())
        {
         // fill vars with data from file
         fin >> objectName >> isParent >> parentName >> scale >> textureName >> xOrbit >> yOrbit >> rotSpeed >> orbSpeed >> planetSet;
         
         scale = scale / 432474.0f; // sun is scale of 1
         xOrbit = xOrbit / 0.307;   // mercury is orbit rad of 1 
         yOrbit = yOrbit / 0.307;   // mercury is orbit rad of 1 

         // create new planet object with data
         objectsDataList.addObject(objectName, isParent, parentName, scale, textureName);  
         objectsDataList.setSpecialValues(objectName, xOrbit, yOrbit, rotSpeed, orbSpeed, planetSet); 
        }
}


bool initialize(char* fileName)    
{
    bool loadedSuccess = true;

    loadObjectDataFromFile( "planetFile.txt" );

    // load ring obj file
    Vertex *ring;
    loadOBJ("ring.obj", &ring);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_ring);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ring);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ring)*160*3, ring, GL_STATIC_DRAW);

    delete ring;
    numberTriangles = 0;

    // load sphere obj file
    Vertex *geometry;
    
    loadedSuccess = loadOBJ(fileName, &geometry);
    if ( !loadedSuccess )
    {
        cout << "OBJ file not found or invalid format" << endl;
        return false;
    }

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry)*numberTriangles*3, geometry, GL_STATIC_DRAW);

    delete geometry;

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
    view = glm::lookAt( glm::vec3(0.0, 8.0, -16.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 0.0, 1.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 


    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);


    //and its done
    return true;
}

void DrawCircle(float cx, float cy, float r, int num_segments)
{
    glBegin(GL_LINE_LOOP);
    for(int ii = 0; ii < num_segments; ii++)
    {
        float theta = 2.0f * 3.1415926f * float(ii) / float(num_segments);//get the current angle

        float x = r * cosf(theta);//calculate the x component
        float y = r * sinf(theta);//calculate the y component

        glVertex2f(x + cx, y + cy);//output vertex

    }
    glEnd();
}

bool loadOBJ(const char * obj, Vertex **data)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(obj, aiProcess_Triangulate); 

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
            memcpy(vertexArray,&pos,sizeof(float)*3);
            vertexArray+=3;

        }

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
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo_geometry);
    glDeleteBuffers(1, &vbo_ring);
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
    switch (key) 
    {
        case GLUT_KEY_LEFT:
            switch(planetOfChoiceNum)
            {
                case 1:
                    planetOffset = .0000005;
                    planetOfChoiceNum = 9;
                    break;
                case 2:
                    planetOfChoiceNum = 1;
                    break;
                case 3:
                    planetOfChoiceNum = 2;
                    break;
                case 4:
                    planetOfChoiceNum = 3;
                    break;
                case 5:
                    planetOfChoiceNum = 4;
                    heightOffset = 0;
                    break;
                case 6:
                    planetOfChoiceNum = 5;                    
                    planetOffset = .00005;
                    break;
                case 7:
                    planetOfChoiceNum = 6;
                    break;
                case 8:
                    planetOfChoiceNum = 7;
                    break;
                case 9:
                    planetOffset = .0005;
                    heightOffset = .12;
                    planetOfChoiceNum = 8;
                    break;

            }
        planetOfChoice = objectsDataList.getObjectName(planetOfChoiceNum);   
            break;
    
        case GLUT_KEY_RIGHT:

            switch(planetOfChoiceNum)
            {
                case 1:
                    planetOfChoiceNum = 2;
                    break;
                case 2:
                    planetOfChoiceNum = 3;
                    break;
                case 3:
                    planetOfChoiceNum = 4;
                    break;
                case 4:
                    planetOfChoiceNum = 5;
                    heightOffset = .12;
                    break;
                case 5:
                    planetOffset = .0005;
                    planetOfChoiceNum = 6;
                    break;
                case 6:
                    planetOfChoiceNum = 7;
                    break;
                case 7:
                    planetOfChoiceNum = 8;
                    break;
                case 8:
                    planetOffset = .0000005;
                    heightOffset = 0;
                    planetOfChoiceNum = 9;
                    break;
                case 9:
                    planetOffset = .00005;
                    planetOfChoiceNum = 1;
                    break;
            }

        planetOfChoice = objectsDataList.getObjectName(planetOfChoiceNum);   
            break;
    }
}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
    
    // highlihgt planets
    else if (key == '1' )
    {
     highlightCount = 1000;
     highlightNumber = 12;
    }
    else if (key == '2' )
    {
     highlightCount = 1000;
     highlightNumber = 13;
    }
    else if (key == '3' )
    {
     highlightCount = 1000;
     highlightNumber = 14;
    }
    else if (key == '4' )
    {
     highlightCount = 1000;
     highlightNumber = 15;
    }
    else if (key == '5' )
    {
     highlightCount = 1000;
     highlightNumber = 16;
    }
    else if (key == '6' )
    {
     highlightCount = 1000;
     highlightNumber = 17;
    }
    else if (key == '7' )
    {
     highlightCount = 1000;
     highlightNumber = 18;
    }
    else if (key == '8' )
    {
     highlightCount = 1000;
     highlightNumber = 19;
    }
    else if (key == '9' )
    {
     highlightCount = 1000;
     highlightNumber = 20;
    }

} 

void myMouse(int button, int state, int x, int y)
{ 
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
         for ( int index = 0; index < objectsDataList.getNumObjects(); index++)
        {
            objectsDataList.pauseRotation(objectsDataList.getObjectName(index));
            objectsDataList.pauseOrbit(objectsDataList.getObjectName(index));
        } 
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
         case 2:
            if ( planetZoom == true)
                planetZoom = false;
            else
                planetZoom = true;
         break;

    }
    glutPostRedisplay();
}


