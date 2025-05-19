#define GL_SILENCE_DEPRECATION
#define TINYOBJLOADER_IMPLEMENTATION



#include "GLShader.h"
#include "tiny_obj_loader.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>

bool loadObjModel(const std::string& filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    
    // Charger le fichier OBJ
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
    if (!ret) return false;
    
    // Map pour éviter les doublons
    std::unordered_map<VertexIndex, unsigned int> uniqueVertices;
    
    // Parcourir toutes les formes
    for (const auto& shape : shapes) {
        // Parcourir toutes les faces de la forme
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) continue; // Ignorer les faces non triangulaires
            
            // Parcourir tous les sommets de la face
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                
                // Créer un index pour ce vertex
                VertexIndex vertex_index = {idx.vertex_index, idx.texcoord_index, idx.normal_index};
                
                // Si ce vertex existe déjà, on réutilise son index
                if (uniqueVertices.count(vertex_index) == 0) {
                    // Sinon, on crée un nouveau vertex
                    Vertex vertex;
                    
                    // Position
                    vertex.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
                    vertex.position[1] = attrib.vertices[3 * idx.vertex_index + 1];
                    vertex.position[2] = attrib.vertices[3 * idx.vertex_index + 2];
                    
                    // Normales
                    if (idx.normal_index >= 0) {
                        vertex.normal[0] = attrib.normals[3 * idx.normal_index + 0];
                        vertex.normal[1] = attrib.normals[3 * idx.normal_index + 1];
                        vertex.normal[2] = attrib.normals[3 * idx.normal_index + 2];
                    } else {
                        vertex.normal[0] = 0.0f;
                        vertex.normal[1] = 0.0f;
                        vertex.normal[2] = 1.0f;
                    }
                    
                    // Coordonnées de texture
                    if (idx.texcoord_index >= 0) {
                        vertex.texcoord[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                        vertex.texcoord[1] = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]; // Inversion du V
                    } else {
                        vertex.texcoord[0] = 0.0f;
                        vertex.texcoord[1] = 0.0f;
                    }
                    
                    // Ajouter le vertex au tableau et stocker son index
                    uniqueVertices[vertex_index] = static_cast<unsigned int>(vertices.size());
                    vertices.push_back(vertex);
                }
                
                // Ajouter l'index au tableau d'indices
                indices.push_back(uniqueVertices[vertex_index]);
            }
        }
    }
    
    return true;
}