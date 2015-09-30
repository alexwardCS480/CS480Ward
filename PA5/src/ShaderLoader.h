// shader class to load vs and fs
class ShaderLoader 
{
  public:
    ShaderLoader(const char *vertexFileName, const char *fragmentFileName);
    const std::string loadFromFile( std::string filename);
    GLuint LoadShader();

  private:
    // filenames
    const char* vertexShaderFileName;
    const char* fragmentShaderFileName;

};
