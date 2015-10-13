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

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;
         
Magick::PixelPacket* loadTextureImage(int &width, int &height); 
void renderAllObjects();
void loadObjectDataFromFile( string fileName );

//--Main 
int main(int argc, char *argv[])
{
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
    //std::string defualtOBJName = "hockeyTable.obj";

    // Name and create the Window
    glutCreateWindow("Hockey Table Example");

    // create menu for window
    glutCreateMenu(demo_menu);
    glutAddMenuEntry("Quit Program", 1);
    glutAddMenuEntry("Play/Pause Object Rotation", 2);
    glutAddMenuEntry("Play/Pause Object Orbit", 3);
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

    //Check to see if the obj was entered into the command line.
    if(argc <= 1) 
    {
        std::cout << "No obj file found!" << std::endl;
        return 0;
    }

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize(argv[1]);
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
    //clear the screen
    glClearColor(0.0, 0.0, 0.2, 1.0); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //enable the shader program
    glUseProgram(program);

    renderAllObjects();


    // lets look at the earth for some reason
    view = glm::lookAt( glm::vec3(0.0, 0.0, 4 ), //Eye Position
                        glm::vec3(objectsDataList.getModelMatrix("earth")[3][0], objectsDataList.getModelMatrix("earth")[3][1], objectsDataList.getModelMatrix("earth")[3][2]), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
 
    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_uv);
                           
    //swap the buffers
    glutSwapBuffers();
}
 
void renderAllObjects()
{
    for ( int index = 0; index < objectsDataList.getNumObjects(); index++)
        {
         renderObject( objectsDataList.getObjectName(index) );        
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
    objectsDataList.updateObjects(dt);

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}


void loadObjectDataFromFile( string fileName )
{
    string objectName, parentName, textureName;
    bool isParent;
    float scale, xOrbit, yOrbit, rotSpeed, orbSpeed;
    
    std::ifstream fin;
    fin.open (fileName, std::ifstream::in);

    while (fin.good())
        {
         fin >> objectName >> isParent >> parentName >> scale >> textureName >> xOrbit >> yOrbit >> rotSpeed >> orbSpeed;
         
         scale = scale / 432474.0f; // sun is scale of 1
         xOrbit = xOrbit / 0.307;   // mercury is orbit rad of 1 
         yOrbit = yOrbit / 0.307;   // mercury is orbit rad of 1 

         objectsDataList.addObject(objectName, isParent, parentName, scale, textureName);  
         objectsDataList.setSpecialValues(objectName, xOrbit, yOrbit, rotSpeed, orbSpeed); 
        }
}


bool initialize(char* fileName)    
{
    bool loadedSuccess = true;

    loadObjectDataFromFile( "planetFile.txt" );

    // to load the OBJ file
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
    objectsDataList.changeRotationDir( "sun" );
   
    break;
    case GLUT_KEY_RIGHT:
    objectsDataList.changeOrbitDir( "sun" );
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
    else if(key == 'a')
    {
        // change rotation direction here
        objectsDataList.changeRotationDir( "sun" );
    }
    else if(key == 'd')
    {
        // change rotation direction here
        objectsDataList.changeOrbitDir( "sun" );
    }

} 

void myMouse(int button, int state, int x, int y)
{ 
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
         // change rotation direction here
         objectsDataList.changeRotationDir( "sun" );
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
         objectsDataList.pauseRotation( "sun" );
         break;
         case 3:
         objectsDataList.pauseOrbit( "sun" );
         break;

    }
    glutPostRedisplay();
}


