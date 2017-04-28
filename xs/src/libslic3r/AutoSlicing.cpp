#include "libslic3r.h"
#include "TriangleMesh.hpp"
#include "AutoSlicing.hpp"

namespace Slic3r
{

void AutoSlicing::clear()
{
    m_vertex.clear();
}

void AutoSlicing::sort_vertex()
{
    // FIXME
    // sorting algorithm goes here
    return;
}

// Breaks down the figure into slices of user-defined size
// and calculates its complexity (number of vertex contained per slice)
std::vector<int> pre_slicing(coordf_t r_size, coordf_t object_height,
    std::vector<const stl_vertex*> &vertex_array)
{
    const coordf_t r_band = r_size / 2;
    int r = 0;
    int r_min = 0;
    std::vector<int> r_complexity;
    r_complexity.reserve(std::ceil(object_height / r_size));
    // Get the number of vertex per slice "r". The more vertex a slice
    // has, the higher the complexity.
    for (int r = 0; r < r_complexity.size(); r++)
    {
        coordf_t current_z = coordf_t(r) * r_size;
        // Get the index of the first vertex that has a z component
        // greater than (r*r_size - r_band)
        for (int i = 0; i < vertex_array.size(); i++)
        {
            if (vertex_array[i]->z >= current_z - r_band)
            {
                r_min = i;
                break;
            }
        }

        // Count the number of vertex whose z component falls in the
        // range [r*r_size - r_band, r*r_size + r_band]
        // FIXME vertex are being count twice!
        int j = r_min;
        while (vertex_array[j]->z <= (current_z + r_band))
        {
            r_complexity[r] += 1;
            if (j == vertex_array.size()-1)
                break;
            else
                j++;
        }
    }
    return r_complexity;
}

}; // namespace Slic3r
