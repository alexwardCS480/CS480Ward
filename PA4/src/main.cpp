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

//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat color[3];
};

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our geometry
int NUM_VERTICES = 400; // max

//Make an object class containing a list of all objects to store rotation/orbit flags
ObjectData objectsDataList;
 
//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_color;

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
void renderObject ( glm::mat4 MVP );
bool loadOBJ(const char * obj, Vertex geometry []);
int numberTriangles = 0;

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;
         
 
//--Main 
int main(int argc, char **argv)
{
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);


    // Name and create the Window
    glutCreateWindow("Moon Orbit Example");

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

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
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

    //enable the shader program
    glUseProgram(program);


    // load each objects mvp
    renderObject( mvp );


    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);
                           
    //swap the buffers
    glutSwapBuffers();
}

void renderObject ( glm::mat4 MVP )
{

    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(MVP));


    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    //set pointers into the vbo for each of the attributes(position and color)
    glVertexAttribPointer( loc_position,//location of attribute
                           3,//number of elements
                           GL_FLOAT,//type
                           GL_FALSE,//normalized?
                           sizeof(Vertex),//stride
                           0);//offset

    glVertexAttribPointer( loc_color,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,color));

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


bool initialize()    
{

    // create the main planet cube and moon cube objects
    objectsDataList.addObject("object", true, "", 4.0f);            // add object with id = planetCube, with scale of 1 
    //objectsDataList.addObject("moonCube", false, "planetCube", .7f);    // add object with id = moonCube, scale to .7
    // here is the function def for reference
    // void addObject(string objID, bool isPlanet, string parent, float scale);

    objectsDataList.setSpecialValues("object", 1.0, 1.0, 0.1f, 0.1f);   // lets make moon rotate faster and skew orbit in x direction for elliptical orbit
    // here is the function def for reference
    // void setSpecialValues(string objectName, float xOrbit, float yOrbit, float rotSpeed, float orbSpeed);



    // to load the OBJ file
    Vertex geometry [NUM_VERTICES];
    loadOBJ("hockeyTable.obj", geometry);



    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);


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

    loc_color = glGetAttribLocation(program,
                    const_cast<const char*>("v_color"));
    if(loc_color == -1)
    {
        std::cerr << "[F] V_COLOR NOT FOUND" << std::endl;
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
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

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

bool loadOBJ(const char * obj, Vertex data [])
{
    char lineHeader[256];   // this size will do for my model
    std::vector< unsigned int > vertexIndices;
    std::vector< glm::vec3 > temp_vertices;

    FILE * file = fopen(obj, "r");
    if( file == NULL )
        {
         printf(".obj file missing\n");
         return false;
        }

    // read in all data
    while( fscanf(file, "%s", lineHeader) != EOF)
        {
       
        // dave the vertex data
        if ( strcmp( lineHeader, "v" ) == 0 )
            {
             glm::vec3 vertex;
             fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
             temp_vertices.push_back(vertex);
            }
        // we dont use the normal or texture data right now, uncomment later if needed
        /*
        else if ( strcmp( lineHeader, "vt" ) == 0 )
            {
             glm::vec2 uv;
             fscanf(file, "%f %f\n", &uv.x, &uv.y );
             temp_uvs.push_back(uv);
            }
        else if ( strcmp( lineHeader, "vn" ) == 0 )
            {
             glm::vec3 normal;
             fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
             temp_normals.push_back(normal);
            }
        */

        // save face data
        else if ( strcmp( lineHeader, "f" ) == 0 )
            {
             std::string vertex1, vertex2, vertex3;
             unsigned int vertexIndex[3], uvIndex[3];

             // this is just the way I formated the obj object when I exported it. I dont use uv or normal but since I exported with uv data I read it in here
             int matches = fscanf(file, "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2] );

            // if wrong format
            if (matches != 6)
                {
                 printf("File can't be read by our simple parser : ( Try exporting with other options\n");
                 return false;
                }

        vertexIndices.push_back(vertexIndex[0]);
        vertexIndices.push_back(vertexIndex[1]);
        vertexIndices.push_back(vertexIndex[2]);
        /*  we dont use the normal or texture data right now, uncomment later if needed
        uvIndices.push_back(uvIndex[0]);
        uvIndices.push_back(uvIndex[1]);
        uvIndices.push_back(uvIndex[2]);
        normalIndices.push_back(normalIndex[0]);
        normalIndices.push_back(normalIndex[1]);
        normalIndices.push_back(normalIndex[2]);
        */
         }

     }

    // For each vertex of each triangle
    for( unsigned int i=0; i<vertexIndices.size(); i++ )
        {
         unsigned int vertexIndex = vertexIndices[i];
         glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];

         // update our vertex data
         data[i].position[0] = vertex.x;
         data[i].position[1] = vertex.y;
         data[i].position[2] = vertex.z;

         // update color data. lets make it change a little for each triangle
         data[i].color[0] = (i%3)*.5;
         data[i].color[1] = (i%3)*.5;
         data[i].color[2] = (i%3)*.5;

         numberTriangles++;
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
        // change rotation direction here
        objectsDataList.changeRotationDir( "object" );
    }
    else if(key == 'd')
    {
        // change rotation direction here
        objectsDataList.changeOrbitDir( "object" );
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


