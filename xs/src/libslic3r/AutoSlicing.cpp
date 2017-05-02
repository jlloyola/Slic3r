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
	//std::cout << "Prepare Begining" << std::endl;
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
	//std::cout << "Collect All Vertex" << std::endl;
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
	//std::cout << "Before Sort" << std::endl;
    std::sort(m_vertex.begin(), m_vertex.end(), compareVertex);
	//std::cout << "After Sort" << std::endl;
	//5 Remove Duplicated Vertex
	for (int j = 0; j < (m_vertex.size() - 1); j++){
		if ((m_vertex[j].x == m_vertex[j + 1].x)
			&& (m_vertex[j].y == m_vertex[j + 1].y)
			&& (m_vertex[j].z == m_vertex[j + 1].z)){
			m_vertex.erase(m_vertex.begin() + j);
		}
	}
	//std::cout << "After Duplicate" << std::endl;
    return;
}

// Breaks down the figure into slices of user-defined size
// and calculates its complexity (number of vertex contained per slice)
std::vector<int> pre_slicing(coordf_t r_size, coordf_t object_height,
    std::vector<stl_vertex> &vertex_array)
{
	//std::cout << "Pre Slice" << std::endl;
	const coordf_t r_band = r_size / 2;
    //const coordf_t r_band = r_size * 2;
    int r = 0;
    int r_min = 0;
    
	//std::cout << "Before Reserve?" << std::endl;
	int r_space;
	//std::cout << "object_height " << object_height << std::endl;
	//std::cout << "r_size " << r_size << std::endl;
	// r_size = 1;
	//std::cout << "r_size " << r_size << std::endl;
	//std::cout << "object_height " << object_height << std::endl;
	//std::cout << "r_size " << r_size << std::endl;
	r_space = int(std::ceil( object_height / r_size))+1;
	//std::cout << "r_space " << r_space << std::endl;

	//std::cout << "Init r_complexity" << std::endl;
	std::vector<int> r_complexity(r_space, 0);
	
	//std::cout << "After Reserve?" << std::endl;
    // Get the number of vertex per slice "r". The more vertex a slice
    // has, the higher the complexity.
	//std::cout << "Get the number of vertex per slice r" << std::endl;
	for (int r = 0; r < r_space; r++)
    {
        coordf_t current_z = coordf_t(r) * r_size;
        //std::cout << "current_z: " << current_z << std::endl;
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
        int j = r_min;
        
        //std::cout << "r_min: " << r_min << std::endl; 
        //std::cout << "vertex_array[j].z: " << vertex_array[j].z << std::endl;
        //std::cout << "r_band: " << r_band << std::endl;
        while (vertex_array[j].z <= (current_z + r_band))
        {
            r_complexity[r] += 1;
            if (j == vertex_array.size()-1) {
                //std::cout << "Count" << r_complexity[r] << std::endl;
                break;
            }
            else
                j++;
        }
    }
    //std::cout << "Return Complexity" << std::endl;
    for (int k = 0; k < r_complexity.size(); k++)
        std::cout << "r_complexity: " << r_complexity[k] << std::endl;


	std::cout << "Average" << std::endl;
	std::vector<int> r_complexity_L(r_complexity.size(), 0);
	int average_neighbors, neighborhood, neighbor_counter, neighborhood_vertex;
	average_neighbors = 1;
	neighborhood = 1 + (average_neighbors * 2);
	//std::cout << "neighborhood" << neighborhood << std::endl;
	for (int i = 0; i < r_complexity.size(); i++) {
		neighbor_counter = 0;
		neighborhood_vertex = 0;

		for (int j = 0; j < neighborhood; j++){
			//std::cout << "(i + j - average_neighbors) " << (i + j - average_neighbors) << std::endl;
			//std::cout << "(i + j  " << (i + j ) << std::endl;
			if ((i + j - average_neighbors) >= 0 && ((i + j - average_neighbors) < r_complexity.size())){
				neighbor_counter++;
				//std::cout << "i + j" << i + j<< std::endl;
				//std::cout << "r_complexity[i + j]" << r_complexity[i + j] << std::endl;
				neighborhood_vertex += r_complexity[i + j - average_neighbors];
			}
		}
		//std::cout << "neighbor_counter" << neighbor_counter << std::endl;
		//std::cout << "neighborhood_vertex" << neighborhood_vertex << std::endl;

		r_complexity_L[i] = round(double(neighborhood_vertex) / double(neighbor_counter));

	}



	

	r_complexity = r_complexity_L;
	std::cout << "r_complexity" << std::endl;
	for (int i = 0; i < r_complexity.size(); i++) {
		std::cout << r_complexity[i] << ", ";
	}
	std::cout << std::endl;





    //std::cout << "Return Complexity" << std::endl;
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
    //std::cout << "before min_complexity: " << min_complexity << std::endl;
    //std::cout << "before max_complexity: " << max_complexity << std::endl;
    for (int i = 0; i < r_complexity.size(); i++)
    {
        if (r_complexity[i] > max_complexity)
            max_complexity = r_complexity[i];
        if (r_complexity[i] < min_complexity)
            min_complexity = r_complexity[i];
    }
    //std::cout << "after min_complexity: " << min_complexity << std::endl;
    //std::cout << "after max_complexity: " << max_complexity << std::endl;

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
        std::cout << "layer_height_complexity: " << layer_height_complexity[i] << std::endl;
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
	std::cout << "z:  " << current_z << std::endl;
    int r = 0;
    // Loop through the entire object height while creating slices
    // from a linear interpolation of the current height and the
    // complexity of its adjacent r_size-layers.
    //std::cout << "before current_z: " << current_z << std::endl;
    //std::cout << "before object_height: " << object_height << std::endl;
    while (current_z < object_height)
    {
		if (current_z > (r + 1) * r_size) {
			while (current_z > (r + 1) * r_size){
				//std::cout << "r in while " << r << std::endl;
				r++;
			}
		}
        coordf_t height = lerp(
            current_z,                     // x
            r*r_size,                      // x0
            layer_height_complexity[r],    // y0
            ((r + 1) * r_size),            // x1
            layer_height_complexity[r + 1] // y1
            );
        layer_height_profile.push_back(current_z);
		layer_height_profile.push_back(height);
        current_z += height;
        layer_height_profile.push_back(current_z);
		std::cout << "Z:  " << current_z << std::endl;
        layer_height_profile.push_back(height);
        //std::cout << "START " << std::endl;
        //std::cout << "height: " << height << std::endl;
        //std::cout << "current_z: " << current_z << std::endl;
        //std::cout << "r: " << r << std::endl;
        //std::cout << "(r+1)*r_size: " << ((r+1)*r_size) << std::endl;
        //std::cout << "layer_height_complexity[r]: " << layer_height_complexity[r] << std::endl;
        //std::cout << "layer_height_complexity[r+1]: " << layer_height_complexity[r+1] << std::endl;
        //std::cout << "END " << std::endl;
    }
    coordf_t last = std::max(first_object_layer_height, layer_height_profile[layer_height_profile.size() - 2]);
    layer_height_profile.push_back(last);
    layer_height_profile.push_back(first_object_layer_height);
    layer_height_profile.push_back(object_height);
    layer_height_profile.push_back(first_object_layer_height);
    //std::cout << "last: " << last << std::endl;
    //std::cout << "first_object_layer_height: " << first_object_layer_height << std::endl;
    //std::cout << "object_height: " << object_height << std::endl;
    //std::cout << "first_object_layer_height: " << first_object_layer_height << std::endl;
    return layer_height_profile;
}

std::vector<coordf_t> AutoSlicing::auto_slice()
{
	//std::cout << "1S" << std::endl;
    // 1) Get the complexity of each r_size-layer
    std::vector<int> r_complexity;
    r_complexity = pre_slicing(
        m_slicing_params.r_size,
        m_slicing_params.object_print_z_height(),
        m_vertex);
	//std::cout << "2S" << std::endl;
    // 2) Transform complexity to layer height
    std::vector<coordf_t> layer_height_complexity;
    layer_height_complexity = convert_complexity_to_layer_height(
        r_complexity,
        m_vertex.size(),
        m_slicing_params.min_layer_height,
        m_slicing_params.max_layer_height);
	//std::cout << "2E" << std::endl;
//	std::cout << "r_complexity" << r_complexity << std::endl;
	//for (int ij = 0; ij < r_complexity; ij++) {
	//	std::cout << r_complexity[ij] << ", ";
	//}

	//std::cout << "3S" << std::endl;
    // 3) Get layer profile based on the layer height complexity
    std::vector<coordf_t> layer_height_profile;
    layer_height_profile =  get_auto_layer_height_profile(
        layer_height_complexity,
        m_slicing_params.object_print_z_height(),
        m_slicing_params.first_object_layer_height,
        m_slicing_params.r_size);
	//std::cout << "3E" << std::endl;
    return layer_height_profile;
}

}; // namespace Slic3r
