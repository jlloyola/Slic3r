#ifndef slic3r_AutoSlicing_hpp_
#define slic3r_AutoSlicing_hpp_

#include "Slicing.hpp"
#include "admesh/stl.h"

namespace Slic3r
{

class AutoSlicing
{
public:
    void clear();
    void set_slicing_parameters(SlicingParameters params) { m_slicing_params = params; }
    // void add_vertex(const stl_vertex vertex) { m_vertex.push_back(vertex); }
    void prepare(const ModelVolumePtrs	&volumes);
    void sort_vertex();
    std::vector<coordf_t> auto_slice();
	std::vector<stl_vertex>      m_vertex;

protected:
    SlicingParameters                   m_slicing_params;
    // Collected vertex of all faces, sorted by raising Z.
    
};

}; // namespace Slic3r

#endif /* slic3r_AutoSlicing_hpp_ */