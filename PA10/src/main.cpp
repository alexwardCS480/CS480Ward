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
    GLfloat normal[3];
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
GLint loc_normal;

GLint loc_lightType;
float lightTypeVal = 0.f;

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
bool initialize(char* fileName);
void cleanUp();

// custom
void renderObject ( glm::mat4 MVP );
bool loadOBJ(const char * obj, Vertex **data);
int numberTriangles = 0;

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;
         
Magick::PixelPacket* loadTextureImage(int &width, int &height); 

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


    //matrix MVP for planet
    mvp = projection * view * objectsDataList.getModelMatrix("object");
    
    glUniform1f(loc_lightType, lightTypeVal);
   
    //enable the shader program
    glUseProgram(program);


    // load each objects mvp
    renderObject( mvp );


    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_uv);
    glDisableVertexAttribArray(loc_normal);
                           
    //swap the buffers
    glutSwapBuffers();
}

void renderObject ( glm::mat4 MVP )
{

    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(MVP));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, aTexture);


    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_uv);
    glEnableVertexAttribArray(loc_normal);

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

    // for normal data
    glVertexAttribPointer( loc_normal,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex, normal));

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


bool initialize(char* fileName)    
{
    bool loadedSuccess = true;


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
 
    // create the main planet cube and moon cube objects
    objectsDataList.addObject("object", true, "", 4.0f);            // add object with id = planetCube, with scale of 1 
    //objectsDataList.addObject("moonCube", false, "planetCube", .7f);    // add object with id = moonCube, scale to .7
    // here is the function def for reference
    // void addObject(string objID, bool isPlanet, string parent, float scale);
 
    objectsDataList.setSpecialValues("object", 1.0, 1.0, 0.f, 0.f);  
    // here is the function def for reference
    // void setSpecialValues(string objectName, float xOrbit, float yOrbit, float rotSpeed, float orbSpeed);


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
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*numberTriangles, geometry, GL_STATIC_DRAW);

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


    //lights
    GLfloat white[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat cyan[] = {.8f, 0.f, 0.f, 1.f};

    glMaterialfv(GL_FRONT, GL_DIFFUSE, cyan);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    //glMaterialfv(GL_FRONT, GL_AMBIENT, white);

    GLfloat shininess[] = {20};
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

    GLfloat lightpos[] = {2, 5, 5, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);


    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 20.0);
    GLfloat spot_direction[] = { -1.0, -1.0, 0.0 };
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);


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
    float *normalArray;
    float *uvArray;


    int numVerts;

    // extract data
    numVerts = mesh->mNumFaces*3;
    *data = new Vertex[numVerts];
    vertexArray = new float[mesh->mNumFaces*3*3];
    normalArray = new float[mesh->mNumFaces*3*3];
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
             
            
            aiVector3D normal = mesh->mNormals[face.mIndices[j]];
            memcpy(normalArray,&normal,sizeof(float)*3);
            normalArray+=3;
            
            aiVector3D pos = mesh->mVertices[face.mIndices[j]];
            memcpy(vertexArray,&pos,sizeof(float)*3);
            vertexArray+=3;

        }

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
    objectsDataList.changeRotationDir( "object" );
   
    break;
    case GLUT_KEY_RIGHT:
    objectsDataList.changeOrbitDir( "object" );
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
        lightTypeVal = 0;
    }
    else if(key == 'd')
    {
        lightTypeVal = 1;
    }
    else if(key == 's')
    {
        lightTypeVal = 2;
    }
    else if(key == 'p')
    {
        lightTypeVal = 3;
    }
    else if(key == 'y')
    {
        lightTypeVal = 4;
    }

} 

void myMouse(int button, int state, int x, int y)
{ 
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
         // change rotation direction here
         objectsDataList.changeRotationDir( "object" );
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
         objectsDataList.pauseRotation( "object" );
         break;
         case 3:
         objectsDataList.pauseOrbit( "object" );
         break;

    }
    glutPostRedisplay();
}


