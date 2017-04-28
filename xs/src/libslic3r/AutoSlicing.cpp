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
std::vector<int> pre_slicing()
{
    int r = 0;
    int r_min = 0;
    std::vector<int> r_complexity;
    r_complexity.reserve(std::ceil(m_slicing_params.object_print_z_height() /
        m_slicing_params.r_size));
    // Get the number of vertex per slice "r". The more vertex a slice
    // has, the higher the complexity.
    for (int r = 0; r < r_complexity.size(); r++)
    {
        coordf_t current_z = coordf_t(r) * m_slicing_params.r_size;
        // Get the index of the first vertex that has a z component
        // greater than (r*r_size - r_band)
        for (int i = 0; i < m_vertex.size(); i++)
        {
            if (m_vertex[i]->z >= current_z - m_slicing_params.r_band)
            {
                r_min = i;
                break;
            }
        }

        // Count the number of vertex whose z component falls in the
        // range [r*r_size - r_band, r*r_size + r_band]
        // FIXME vertex are being count twice!
        int j = r_min;
        while (m_vertex[j]->z <= (current_z + m_slicing_params.r_band))
        {
            r_complexity[r] += 1;
            if (j == m_vertex.size()-1)
                break;
            else
                j++;
        }
    }
    return r_complexity;
}

}; // namespace Slic3r
