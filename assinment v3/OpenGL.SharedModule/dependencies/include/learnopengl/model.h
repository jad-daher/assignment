#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <filesystem>
#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

// Requires a current OpenGL context. Resolves embedded Assimp images before files.
inline unsigned int TextureFromAssimp(const aiScene &scene, const char *path,
                                      const string &directory, bool gamma = false);
inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);
inline unsigned int UploadAssimpTexture(const unsigned char *pixels, int width, int height,
                                        GLenum format, bool gamma);

class Model
{
public:
    // model data
    vector<Texture> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh> meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        string extension = std::filesystem::path(path).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        const bool isGltf = extension == ".glb" || extension == ".gltf";
        // Assimp already converts glTF UVs. This matches the application's
        // stbi_set_flip_vertically_on_load(true) convention.
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;
        flags |= isGltf ? aiProcess_PreTransformVertices : aiProcess_FlipUVs;
        const aiScene *scene = importer.ReadFile(path, flags);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = std::filesystem::path(path).parent_path().string();

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex{};
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            if (mesh->HasTangentsAndBitangents())
            {
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, scene,
                                                           material->GetTextureCount(aiTextureType_BASE_COLOR) ? aiTextureType_BASE_COLOR : aiTextureType_DIFFUSE, "texture_diffuse");
        if (diffuseMaps.empty())
        {
            // The shader always samples a diffuse texture, including for materials
            // with only a solid base color. Never reuse a previous mesh's texture.
            aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
            if (material->Get(AI_MATKEY_BASE_COLOR, color) != AI_SUCCESS)
                material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            unsigned char pixel[4];
            for (unsigned int channel = 0; channel < 4; ++channel)
                pixel[channel] = static_cast<unsigned char>(std::clamp(color[channel], 0.0f, 1.0f) * 255.0f);
            Texture fallback{UploadAssimpTexture(pixel, 1, 1, GL_RGBA, gammaCorrection), "texture_diffuse", ""};
            diffuseMaps.push_back(fallback);
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, scene, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, scene,
                                                               material->GetTextureCount(aiTextureType_NORMALS) ? aiTextureType_NORMALS : aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, scene, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, const aiScene *scene, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    Texture cached = textures_loaded[j];
                    cached.type = typeName;
                    textures.push_back(cached);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            { // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromAssimp(*scene, str.C_Str(), this->directory,
                                               gammaCorrection && typeName == "texture_diffuse");
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture); // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return textures;
    }
};

// Shared upload path: RGBA decoding also handles grayscale and two-channel images,
// and avoids row-alignment problems for RGB images with odd widths.
inline unsigned int UploadAssimpTexture(const unsigned char *pixels, int width, int height,
                                        GLenum format, bool gamma)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, gamma ? GL_SRGB_ALPHA : GL_RGBA,
                 width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}

inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string relativePath(path);
    std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
    const auto filename = std::filesystem::path(directory) / relativePath;
    int width, height, channels;
    unsigned char *data = stbi_load(filename.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data)
        throw std::runtime_error("Texture failed to load: " + filename.string());
    unsigned int textureID = UploadAssimpTexture(data, width, height, GL_RGBA, gamma);
    stbi_image_free(data);
    return textureID;
}

inline unsigned int TextureFromAssimp(const aiScene &scene, const char *path,
                                      const string &directory, bool gamma)
{
    const aiTexture *embedded = scene.GetEmbeddedTexture(path);
    if (!embedded)
        return TextureFromFile(path, directory, gamma);

    if (embedded->mWidth > static_cast<unsigned int>(std::numeric_limits<int>::max()) ||
        embedded->mHeight > static_cast<unsigned int>(std::numeric_limits<int>::max()))
        throw std::runtime_error("Embedded texture is too large: " + string(path));

    if (embedded->mHeight == 0)
    {
        // PNG/JPEG bytes: mWidth is the byte count, not the image width.
        int width, height, channels;
        unsigned char *data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char *>(embedded->pcData),
            static_cast<int>(embedded->mWidth), &width, &height, &channels, STBI_rgb_alpha);
        if (!data)
            throw std::runtime_error("Embedded texture failed to decode: " + string(path));
        unsigned int textureID = UploadAssimpTexture(data, width, height, GL_RGBA, gamma);
        stbi_image_free(data);
        return textureID;
    }

    // Uncompressed Assimp texels are BGRA. Flip rows to match stbi's convention
    // used by this application, since these pixels do not pass through stbi.
    vector<aiTexel> pixels(static_cast<size_t>(embedded->mWidth) * embedded->mHeight);
    for (unsigned int y = 0; y < embedded->mHeight; ++y)
        std::copy_n(embedded->pcData + static_cast<size_t>(y) * embedded->mWidth,
                    embedded->mWidth,
                    pixels.data() + static_cast<size_t>(embedded->mHeight - 1 - y) * embedded->mWidth);
    return UploadAssimpTexture(reinterpret_cast<const unsigned char *>(pixels.data()),
                               static_cast<int>(embedded->mWidth), static_cast<int>(embedded->mHeight), GL_BGRA, gamma);
}
#endif
