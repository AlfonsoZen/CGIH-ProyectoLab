#pragma once

// =============================================================================
//   modelStatic.h  --  Loader de modelos 3D ESTATICOS (FBX/OBJ/...) con Assimp
// =============================================================================
//
//   POR QUE EXISTE
//   --------------
//   El loader original 'modelAnim.h' (que viene con el PatoVolador) carga
//   modelos animados con huesos. Para el escenario del Duck Hunt necesitamos
//   muchos modelos SIN huesos (terreno, arboles, props), donde inicializar
//   skinning de huesos es innecesario y costoso. 'ModelStatic' es la version
//   liviana: solo geometria + texturas + bounding boxes.
//
//   QUE HACE
//   --------
//   Al construir 'ModelStatic(path)':
//     1) Parsea el archivo con Assimp (Triangulate + FlipUVs + GenSmoothNormals).
//     2) Recorre la jerarquia de nodos acumulando transformaciones.
//     3) Para cada mesh:
//          - copia vertices/normales/UVs aplicando el transform del nodo,
//          - carga texturas (diffuse y specular),
//          - calcula bounding box LOCAL del mesh y lo guarda en meshBoxes.
//     4) Actualiza el bbox GLOBAL del modelo (bboxMin/bboxMax) con todos
//        los vertices.
//
//   TEXTURAS EMBEBIDAS ("*N")
//   -------------------------
//   Cuando Blender exporta el FBX con la opcion "Embed Textures", las
//   texturas no son archivos sueltos sino que vienen DENTRO del .fbx,
//   referenciadas por nombre "*0", "*1", etc. El loader detecta este
//   caso y las carga desde memoria con SOIL. Si la textura es un archivo
//   externo (caso normal), se busca por basename en el directorio del modelo.
//
//   POR QUE LOS BBOX POR MESH
//   -------------------------
//   El FBX 'arboles.fbx' contiene 3 arboles distintos como meshes separados.
//   Tener un bbox global del modelo entero no sirve para colisiones (seria
//   una caja gigante que engloba los 3). En cambio, meshBoxes[i] da una
//   caja chica por cada arbol, lo que permite hitboxes precisos.
//
//   USO BASICO
//   ----------
//     ModelStatic m("path/al/modelo.fbx");
//     // ... cada frame ...
//     m.Draw(shader);              // pintar
//     // datos para colision:
//     m.bboxMin / m.bboxMax;       // caja global
//     m.meshBoxes[i].mn / .mx;     // caja del mesh i
// =============================================================================

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "SOIL2/SOIL2.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "shader.h"

class ModelStatic
{
public:
    // ---- DATOS PUBLICOS (accesibles desde el codigo cliente) ----
    std::vector<Mesh> meshes;                // submeshes ya con VAO/VBO listos
    std::string directory;                   // carpeta del archivo (para texturas externas)
    std::vector<Texture> textures_loaded;    // cache para no recargar la misma textura
    const aiScene* scene = nullptr;          // puntero al aiScene de Assimp
    Assimp::Importer importer;               // se mantiene vivo mientras viva el modelo

    // Bounding box GLOBAL del modelo (envoltura de TODOS los vertices).
    // Se inicializa con valores extremos opuestos para que el primer
    // glm::min/max lo reemplace correctamente.
    glm::vec3 bboxMin = glm::vec3( 1e9f);
    glm::vec3 bboxMax = glm::vec3(-1e9f);

    // Bounding box POR MESH: si el FBX contiene varios meshes separados
    // (ej. arboles.fbx con 3 arboles, arbustos.fbx con 6 arbustos),
    // cada uno tiene su propia caja chica. Se usa para colisiones precisas.
    struct MeshBBox { glm::vec3 mn, mx; };
    std::vector<MeshBBox> meshBoxes;

    // Constructor: carga inmediatamente el modelo desde 'path'.
    ModelStatic(const std::string& path) { loadModel(path); }

    // Dibuja todos los meshes con el shader dado.
    void Draw(Shader shader)
    {
        for (auto& m : meshes) m.Draw(shader);
    }

private:
    void loadModel(const std::string& path)
    {
        // Los .assbin ya fueron procesados (Triangulate + FlipUVs) durante la
        // conversión desde FBX. Cargar con 0 flags evita re-procesar (en
        // particular evita que FlipUVs se aplique dos veces).
        scene = importer.ReadFile(path, 0);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP (" << path << "): " << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of("/\\"));
        std::cout << ">>> [Static] " << path
                  << "  meshes=" << scene->mNumMeshes
                  << "  embTex=" << scene->mNumTextures << std::endl;

        processNode(scene->mRootNode, aiMatrix4x4());

        std::cout << ">>> [Static]   bbox min=(" << bboxMin.x << "," << bboxMin.y << "," << bboxMin.z
                  << ")  max=(" << bboxMax.x << "," << bboxMax.y << "," << bboxMax.z << ")"
                  << "  size=(" << (bboxMax.x-bboxMin.x) << "," << (bboxMax.y-bboxMin.y) << "," << (bboxMax.z-bboxMin.z) << ")"
                  << std::endl;
    }

    // Recorre la jerarquia de nodos de Assimp acumulando la transformacion
    // (parent * node->mTransformation) y procesa cada mesh con el transform
    // resultante. Asi los meshes salen en su posicion "world" relativa al
    // origen del FBX, aplicando rotaciones/escalas heredadas de los padres.
    void processNode(aiNode* node, aiMatrix4x4 parentTransform)
    {
        aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, nodeTransform));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], nodeTransform);
        }
    }

    // Convierte un aiMesh de Assimp a un Mesh propio aplicando el transform
    // del nodo. Tambien acumula los bounding box (global y por mesh).
    Mesh processMesh(aiMesh* mesh, const aiMatrix4x4& nodeTransform)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        // Para las normales se usa la submatriz 3x3 (sin traslacion).
        aiMatrix3x3 normalMatrix(nodeTransform);

        MeshBBox mbb;
        mbb.mn = glm::vec3( 1e9f);
        mbb.mx = glm::vec3(-1e9f);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex v;
            aiVector3D p = nodeTransform * mesh->mVertices[i];
            v.Position = glm::vec3(p.x, p.y, p.z);

            if (mesh->HasNormals()) {
                aiVector3D n = normalMatrix * mesh->mNormals[i];
                v.Normal = glm::normalize(glm::vec3(n.x, n.y, n.z));
            } else {
                v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            v.TexCoords = mesh->mTextureCoords[0]
                ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                : glm::vec2(0.0f, 0.0f);

            bboxMin = glm::min(bboxMin, v.Position);
            bboxMax = glm::max(bboxMax, v.Position);
            mbb.mn  = glm::min(mbb.mn, v.Position);
            mbb.mx  = glm::max(mbb.mx, v.Position);

            vertices.push_back(v);
        }
        meshBoxes.push_back(mbb);

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            auto diffuse  = loadMaterialTextures(material, aiTextureType_DIFFUSE,  "texture_diffuse");
            auto specular = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), diffuse.begin(),  diffuse.end());
            textures.insert(textures.end(), specular.begin(), specular.end());
        }

        return Mesh(vertices, indices, textures);
    }

    // Carga las texturas (diffuse o specular) de un material. Si vienen
    // embebidas en el FBX (referencia "*N") las extrae desde memoria; sino
    // las busca como archivos en el directorio del modelo.
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
    {
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;
            for (auto& loaded : textures_loaded) {
                if (std::strcmp(loaded.path.C_Str(), str.C_Str()) == 0) {
                    textures.push_back(loaded);
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            std::string raw = str.C_Str();
            std::replace(raw.begin(), raw.end(), '\\', '/');
            size_t lastSlash = raw.find_last_of('/');
            std::string base = (lastSlash != std::string::npos) ? raw.substr(lastSlash + 1) : raw;

            Texture texture;
            texture.id = 0;

            // Caso 1: textura embebida con referencia "*N".
            if (!base.empty() && base[0] == '*' && scene != nullptr) {
                int embIdx = std::atoi(base.c_str() + 1);
                if (embIdx >= 0 && embIdx < (int)scene->mNumTextures) {
                    aiTexture* embTex = scene->mTextures[embIdx];
                    if (embTex->mHeight == 0) {
                        texture.id = SOIL_load_OGL_texture_from_memory(
                            (const unsigned char*)embTex->pcData,
                            (int)embTex->mWidth,
                            SOIL_LOAD_AUTO,
                            SOIL_CREATE_NEW_ID,
                            0);
                            if (texture.id) { glBindTexture(GL_TEXTURE_2D, texture.id); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); }
                    } else {
                        GLuint tid;
                        glGenTextures(1, &tid);
                        glBindTexture(GL_TEXTURE_2D, tid);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, embTex->mWidth, embTex->mHeight,
                                     0, GL_BGRA, GL_UNSIGNED_BYTE, embTex->pcData);
                        glGenerateMipmap(GL_TEXTURE_2D);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        texture.id = tid;
                    }
                }
            }

            // Caso 1b: FBX exportado con texturas embebidas pero los materiales
            // mantienen el path original (ej. "Modelo.fbm\Image_0_029") en vez
            // de "*N". Assimp pone los datos en scene->mTextures y guarda el
            // nombre original en aiTexture::mFilename. Buscamos por coincidencia
            // de basename, o por indice secuencial como fallback.
            if (texture.id == 0 && scene != nullptr && scene->mNumTextures > 0) {
                // Intentar coincidencia por mFilename (Assimp >= 4.0)
                for (unsigned int t = 0; t < scene->mNumTextures && texture.id == 0; t++) {
                    const aiTexture* embTex = scene->mTextures[t];
                    std::string embPath = embTex->mFilename.C_Str();
                    std::replace(embPath.begin(), embPath.end(), '\\', '/');
                    size_t ls2 = embPath.find_last_of('/');
                    std::string embBase = (ls2 != std::string::npos) ? embPath.substr(ls2+1) : embPath;
                    // Comparar basenames (con y sin extension)
                    size_t dot = embBase.rfind('.');
                    std::string embStem = (dot != std::string::npos) ? embBase.substr(0, dot) : embBase;
                    bool match = (!embBase.empty() && (embBase == base || embStem == base))
                              || (!embBase.empty() && base.find(embStem) != std::string::npos);
                    if (match) {
                        if (embTex->mHeight == 0) {
                            texture.id = SOIL_load_OGL_texture_from_memory(
                                (const unsigned char*)embTex->pcData,
                                (int)embTex->mWidth,
                                SOIL_LOAD_AUTO,
                                SOIL_CREATE_NEW_ID,
                                0);
                            if (texture.id) { glBindTexture(GL_TEXTURE_2D, texture.id); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); }
                        } else {
                            GLuint tid;
                            glGenTextures(1, &tid);
                            glBindTexture(GL_TEXTURE_2D, tid);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, embTex->mWidth, embTex->mHeight,
                                         0, GL_BGRA, GL_UNSIGNED_BYTE, embTex->pcData);
                            glGenerateMipmap(GL_TEXTURE_2D);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            texture.id = tid;
                        }
                    }
                }
                // Fallback secuencial: si mFilename esta vacio, usar la textura
                // embebida en el mismo orden que las texturas del material.
                if (texture.id == 0) {
                    unsigned int seqIdx = (unsigned int)(textures_loaded.size() % scene->mNumTextures);
                    const aiTexture* embTex = scene->mTextures[seqIdx];
                    if (embTex->mHeight == 0 && embTex->mWidth > 0) {
                        texture.id = SOIL_load_OGL_texture_from_memory(
                            (const unsigned char*)embTex->pcData,
                            (int)embTex->mWidth,
                            SOIL_LOAD_AUTO,
                            SOIL_CREATE_NEW_ID,
                            0);
                            if (texture.id) { glBindTexture(GL_TEXTURE_2D, texture.id); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); }
                    }
                }
            }

            // Caso 2: textura externa. Buscamos por nombre base en la carpeta
            // del modelo probando varias extensiones comunes.
            if (texture.id == 0) {
                const char* exts[] = { "", ".png", ".jpg", ".jpeg", ".tga", ".bmp" };
                for (const char* ext : exts) {
                    std::string candidate = base + ext;
                    std::string full = directory + "/" + candidate;
                    std::ifstream f(full.c_str(), std::ios::binary);
                    if (f.good()) {
                        f.close();
                        texture.id = loadFromFile(candidate, directory);
                        break;
                    }
                }
            }

            std::cout << ">>> [Static] textura '" << str.C_Str()
                      << "' -> id=" << texture.id << std::endl;

            texture.type = typeName;
            texture.path = str;
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
        return textures;
    }

    // Carga una imagen desde disco usando SOIL, genera mipmaps y configura
    // wrap REPEAT + filtros lineales.
    static GLint loadFromFile(const std::string& filename, const std::string& dir)
    {
        std::string full = dir + "/" + filename;
        GLuint textureID;
        glGenTextures(1, &textureID);

        int w, h;
        unsigned char* image = SOIL_load_image(full.c_str(), &w, &h, 0, SOIL_LOAD_RGB);
        if (!image) {
            glDeleteTextures(1, &textureID);
            return 0;
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        SOIL_free_image_data(image);
        return textureID;
    }
};
