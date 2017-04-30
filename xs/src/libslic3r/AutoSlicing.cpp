#include "libslic3r.h"
#include "TriangleMesh.hpp"
#include "AutoSlicing.hpp"
#include "Model.hpp"

namespace Slic3r
{

void AutoSlicing::clear()
{
    m_vertex.clear();
   
}

bool compareVertex(stl_vertex &v1, stl_vertex &v2) {
	//std::cout << v1.z << std::endl;
	if (v1.z < v2.z) return true;
	if (v1.z > v2.z) return false;
	if (v1.x < v2.x) return true;
	if (v1.x > v2.x) return false;
	if (v1.y <v1.y)  return true;

	return false;
}


void AutoSlicing::prepare(const ModelVolumePtrs		&volumes){ 
	std::cout << "Prepare Begining" << std::endl;
	std::vector<const TriangleMesh*>	m_meshes;
	std::vector<const stl_facet*>		m_faces;
 // 1) Collect all meshes.
	for (ModelVolumePtrs::const_iterator it = volumes.begin(); it != volumes.end(); ++it)
		if (!(*it)->modifier)
			m_meshes.push_back(&(*it)->mesh);

// 2) Collect faces of all meshes.
    int nfaces_total = 0;
	for (std::vector<const TriangleMesh*>::const_iterator it_mesh = m_meshes.begin(); it_mesh != m_meshes.end(); ++it_mesh)
		nfaces_total += (*it_mesh)->stl.stats.number_of_facets;
	m_faces.reserve(nfaces_total);
	for (std::vector<const TriangleMesh*>::const_iterator it_mesh = m_meshes.begin(); it_mesh != m_meshes.end(); ++it_mesh)
		for (int i = 0; i < (*it_mesh)->stl.stats.number_of_facets; ++i)
			m_faces.push_back((*it_mesh)->stl.facet_start + i);

// 3) Collect all Vertex.
	std::cout << "Collect All Vertex" << std::endl;
	for (int iface = 0; iface < m_faces.size(); iface++){
        m_vertex.push_back(m_faces[iface]->vertex[0]);
	    m_vertex.push_back(m_faces[iface]->vertex[1]);
	    m_vertex.push_back(m_faces[iface]->vertex[2]);

    }		
    return;
}
	       

void AutoSlicing::sort_vertex()
{	
    //4 Sort Vertex
	std::cout << "Before Sort" << std::endl;
    std::sort(m_vertex.begin(), m_vertex.end(), compareVertex);
	std::cout << "After Sort" << std::endl;
	//5 Remove Duplicated Vertex
	for (int j = 0; j < (m_vertex.size() - 1); j++){
		if ((m_vertex[j].x == m_vertex[j + 1].x)
			&& (m_vertex[j].y == m_vertex[j + 1].y)
			&& (m_vertex[j].z == m_vertex[j + 1].z)){
			m_vertex.erase(m_vertex.begin() + j);
		}
	}
	std::cout << "After Duplicate" << std::endl;
    return;
}

// Breaks down the figure into slices of user-defined size
// and calculates its complexity (number of vertex contained per slice)
std::vector<int> pre_slicing(coordf_t r_size, coordf_t object_height,
    std::vector<stl_vertex> &vertex_array)
{
	std::cout << "Pre Slice" << std::endl;
    const coordf_t r_band = r_size / 2;
    int r = 0;
    int r_min = 0;
    
	std::cout << "Before Reserve?" << std::endl;
	int r_space;
	//FIXME HARDCODED
	object_height = 20;
	if (r_size = 0) r_size = 1;
	std::cout << "object_height " << object_height << std::endl;
	std::cout << "r_size " << r_size << std::endl;
		//FIXME NEED TO VALIDATE THIS
	r_space = int(std::ceil( object_height / r_size))+1;
	std::cout << "r_space " << r_space << std::endl;
	//Hardcoded
	r_space = 21;
	std::cout << "r_space " << r_space << std::endl;
	std::cout << "Init r_complexity" << std::endl;
	std::vector<int> r_complexity(r_space, 0);
	
	std::cout << "After Reserve?" << std::endl;
    // Get the number of vertex per slice "r". The more vertex a slice
    // has, the higher the complexity.
	std::cout << "Get the number of vertex per slice r" << std::endl;
	for (int r = 0; r < r_space; r++)
    {
        coordf_t current_z = coordf_t(r) * r_size;
        // Get the index of the first vertex that has a z component
        // greater than (r*r_size - r_band)
        for (int i = 0; i < vertex_array.size(); i++)
        {
            if (vertex_array[i].z >= current_z - r_band)
            {
                r_min = i;
                break;
            }
        }

        // Count the number of vertex whose z component falls in the
        // range [r*r_size - r_band, r*r_size + r_band]
        // FIXME vertex are being count twice!
        int j = r_min;
		std::cout << "Count" << std::endl;
        while (vertex_array[j].z <= (current_z + r_band))
        {
            r_complexity[r] += 1;
            if (j == vertex_array.size()-1)
                break;
            else
                j++;
        }
    }
	std::cout << "Return Complexity" << std::endl;
    return r_complexity;
}

// Performs a linear interpolation using the equation:
// y = y0 + (x - x0) * (y1-y0)/(x1-x0)
coordf_t lerp(coordf_t x, coordf_t x0, coordf_t y0, coordf_t x1, coordf_t y1)
{
    return (y0 + (x - x0) * (y1 - y0) / (x1 - x0));
}

// Convert slice complexity of each r_size-slice to layer complexity
// using a linear interpolation


std::vector<coordf_t> convert_complexity_to_layer_height(
    std::vector<int> &r_complexity,
    int number_of_vertex,
    coordf_t min_layer_height,
    coordf_t max_layer_height)
{
	std::vector<coordf_t> layer_height_complexity(r_complexity.size(), min_layer_height);
   

    // Get max/min complexity (from number of vertex)
    int min_complexity = number_of_vertex;
    int max_complexity = 0;
    for (int i = 0; i < r_complexity.size(); i++)
    {
        if (r_complexity[i] > max_complexity)
            max_complexity = r_complexity[i];
        if (r_complexity[i] < min_complexity)
            min_complexity = r_complexity[i];
    }

    // Convert complexity to layer height using linear interpolation
    for (int i = 0; i < layer_height_complexity.size(); i++)
    {
        layer_height_complexity[i] = lerp(
            coordf_t(r_complexity[i]), // x
            coordf_t(max_complexity),  // x0
            min_layer_height,          // y0
            coordf_t(min_complexity),  // x1
            max_layer_height           // y1
            );
    }
    return layer_height_complexity;
}

// Calculate layer height based on the layer complexity
std::vector<coordf_t> get_auto_layer_height_profile(
    std::vector<coordf_t> layer_height_complexity,
    coordf_t object_height,
    coordf_t first_object_layer_height,
    coordf_t r_size)
{
    std::vector<coordf_t> layer_height_profile;
    layer_height_profile.push_back(0.);
    layer_height_profile.push_back(first_object_layer_height);
    coordf_t current_z = first_object_layer_height;
    int r = 0;
    // Loop through the entire object height while creating slices
    // from a linear interpolation of the current height and the
    // complexity of its adjacent r_size-layers.
    while (current_z < object_height)
    {
        if (current_z > (r + 1) * r_size)
            r++;
        coordf_t height = lerp(
            current_z,                     // x
            r*r_size,                      // x0
            layer_height_complexity[r],    // y0
            ((r + 1) * r_size),            // x1
            layer_height_complexity[r + 1] // y1
            );
        layer_height_profile.push_back(height);
        current_z += height;
    }
    return layer_height_profile;
}

std::vector<coordf_t> AutoSlicing::auto_slice()
{
	std::cout << "1S" << std::endl;
    // 1) Get the complexity of each r_size-layer
    std::vector<int> r_complexity;
    r_complexity = pre_slicing(
        m_slicing_params.r_size,
        m_slicing_params.object_print_z_height(),
        m_vertex);
	std::cout << "2S" << std::endl;
    // 2) Transform complexity to layer height
    std::vector<coordf_t> layer_height_complexity;
    layer_height_complexity = convert_complexity_to_layer_height(
        r_complexity,
        m_vertex.size(),
        m_slicing_params.min_layer_height,
        m_slicing_params.max_layer_height);
	std::cout << "3S" << std::endl;
    // 3) Get layer profile based on the layer height complexity
    std::vector<coordf_t> layer_height_profile;
    layer_height_profile =  get_auto_layer_height_profile(
        layer_height_complexity,
        m_slicing_params.object_print_z_height(),
        m_slicing_params.first_object_layer_height,
        m_slicing_params.r_size);
	std::cout << "3E" << std::endl;
    return layer_height_profile;
}

}; // namespace Slic3r
