#include "model.h"

Model::Model(GLchar *path)
{
    this->loadModel(path);
}

void Model::Draw(Shader shader)
{
    for (GLuint i = 0; i < this->meshes.size(); i++)
        this->meshes[i].Draw(shader);
}

void Model::loadModel(string path)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return;
    }
    this->directory = path.substr(0, path.find_last_of('/'));
    this->processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        this->meshes.push_back(this->processMesh(mesh, scene));
    }
    for (GLuint i = 0; i < node->mNumChildren; i++)
        this->processNode(node->mChildren[i], scene);
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    vector<Vertex>  vertices;
    vector<GLuint>  indices;
    vector<Texture> textures;

    for (GLuint i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.Position  = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.Normal    = { mesh->mNormals[i].x,   mesh->mNormals[i].y,   mesh->mNormals[i].z };
        vertex.TexCoords = mesh->mTextureCoords[0]
            ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
            : glm::vec2(0.0f);
        vertices.push_back(vertex);
    }

    for (GLuint i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (GLuint j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

        // Diffuse fallback chain:
        // 1. Standard DIFFUSE (OBJ/FBX legacy)
        // 2. GLTF2 PBR BASE_COLOR (Blender, most GLB exporters)
        // 3. EMISSIVE — Hyper3D "shaded" exports bake the albedo into the emissive channel
        auto diffuse = loadMaterialTextures(mat, aiTextureType_DIFFUSE,   "texture_diffuse", scene);
        if (diffuse.empty())
            diffuse   = loadMaterialTextures(mat, aiTextureType_BASE_COLOR,"texture_diffuse", scene);
        if (diffuse.empty())
            diffuse   = loadMaterialTextures(mat, aiTextureType_EMISSIVE,  "texture_diffuse", scene);
        textures.insert(textures.end(), diffuse.begin(), diffuse.end());

        // Specular
        auto specular = loadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular", scene);
        textures.insert(textures.end(), specular.begin(), specular.end());
    }

    return Mesh(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                                             string typeName, const aiScene* scene)
{
    vector<Texture> textures;

    for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        bool skip = false;
        for (auto& tl : textures_loaded)
        {
            if (tl.path == str) { textures.push_back(tl); skip = true; break; }
        }

        if (!skip)
        {
            Texture texture;
            // GLB embedded textures have paths like "*0", "*1", ...
            if (str.data[0] == '*')
                texture.id = TextureFromEmbedded(scene->mTextures[atoi(str.data + 1)]);
            else
                texture.id = TextureFromFile(str.C_Str(), this->directory);

            texture.type = typeName;
            texture.path = str;
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }
    return textures;
}

// Load a texture embedded directly in a GLB/GLTF file
GLint TextureFromEmbedded(const aiTexture* tex)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height;
    unsigned char* image = nullptr;
    bool usedSOIL = false;

    if (tex->mHeight == 0)
    {
        // Compressed format (PNG/JPEG stored as raw bytes)
        image = SOIL_load_image_from_memory(
            reinterpret_cast<const unsigned char*>(tex->pcData),
            tex->mWidth, &width, &height, nullptr, SOIL_LOAD_RGBA);
        usedSOIL = true;
    }
    else
    {
        // Uncompressed ARGB8888
        width  = tex->mWidth;
        height = tex->mHeight;
        image  = new unsigned char[width * height * 4];
        for (int p = 0; p < width * height; p++)
        {
            image[p*4+0] = tex->pcData[p].r;
            image[p*4+1] = tex->pcData[p].g;
            image[p*4+2] = tex->pcData[p].b;
            image[p*4+3] = tex->pcData[p].a;
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    if (image)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (usedSOIL) SOIL_free_image_data(image);
    else          delete[] image;

    return textureID;
}

GLint TextureFromFile(const char *path, string directory)
{
    string filename = directory + '/' + string(path);
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height;
    // RGBA instead of RGB — preserves alpha channel present in many GLB textures
    unsigned char* image = SOIL_load_image(filename.c_str(), &width, &height, nullptr, SOIL_LOAD_RGBA);

    glBindTexture(GL_TEXTURE_2D, textureID);
    if (image)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cout << "WARNING: texture not found: " << filename << endl;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    SOIL_free_image_data(image);
    return textureID;
}
