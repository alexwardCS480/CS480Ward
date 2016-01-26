#include"ShaderLoader.h"


// constructor to load the class privates with the constructor params
ShaderLoader::ShaderLoader(const char *vertexFileName, const char *fragmentFileName)
{
    vertexShaderFileName = vertexFileName;
    fragmentShaderFileName = fragmentFileName;
}


// to actually load the shaders and return
GLuint ShaderLoader::LoadShader() 
{

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    // load vs and fs form file into a string
    const std::string vsString = loadFromFile( vertexShaderFileName );
    const std::string fsString = loadFromFile( fragmentShaderFileName );

    // convert string to char pointer for this program
    const char *vs = vsString.c_str();
    const char *fs = fsString.c_str();

    //compile the shaders
    GLint shader_status;

    // Vertex shader first
    glShaderSource(vertex_shader, 1, &vs, NULL);
    glCompileShader(vertex_shader);
    //check the compile status
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE VERTEX SHADER!" << std::endl;
        return 0;
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
        return 0;
    }

    //Now we link the 2 shader objects into a program
    //This program is what is run on the GPU
    GLuint programTmp = glCreateProgram();
    glAttachShader(programTmp, vertex_shader);
    glAttachShader(programTmp, fragment_shader);
    glLinkProgram(programTmp);
    //check if everything linked ok
    glGetProgramiv(programTmp, GL_LINK_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] THE SHADER PROGRAM FAILED TO LINK" << std::endl;
        return 0;
    }

    return programTmp;
}



// function to load data from file
const std::string ShaderLoader::loadFromFile( std::string filename)
{
    std::string line;
    std::string allText;

    // open file in src directory
    std::ifstream myfile ( filename );

    // if file exists
    if (myfile.is_open())
    {
     // read in file
     while ( getline (myfile,line) )
        {
         line+="\n";
         allText+=line;
        }
     myfile.close();
    }   
    // if file invalid print to user
    else 
    {
     std::cerr << "SHADER TEXT FILE NOT FOUND";
    }

    return allText;
}



