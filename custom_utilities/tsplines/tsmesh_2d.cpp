#include "tsmesh_2d.h"
#include "custom_utilities/bezier_utils.h"

namespace Kratos
{

    typedef TsMesh2D::knot_t knot_t;

    /*****************************************************************************/
    /* BEGIN SUBROUTINES TO CONSTRUCT THE T-MESH */
    /*****************************************************************************/

    /// Unlock the T-splines construction
    void TsMesh2D::BeginConstruct()
    {
        mLockConstruct = false;
    }

    /// Set the order of T-splines
    void TsMesh2D::SetOrder(int Dim, int Order)
    {
        LockQuery();
        if(Dim == 0)
            mOrder1 = Order;
        else if(Dim == 1)
            mOrder2 = Order;
        else
            KRATOS_THROW_ERROR(std::logic_error, "T-splines 2D mesh does not support order > 2", "")
    }

    /// Insert knot into the knot vectors
    knot_t TsMesh2D::InsertKnot(int Dim, double Value)
    {
        LockQuery();
        knot_t pKnot = knot_t(new Knot<double>(Value));
        if(Dim == 0)
            mKnots1.push_back(pKnot);
        else if(Dim == 1)
            mKnots2.push_back(pKnot);
        else
            KRATOS_THROW_ERROR(std::logic_error, "The 2D T-splines does not support for higher dimension", "")
        return pKnot;
    }

    /// Add a vertex to the topology mesh
    TsVertex::Pointer TsMesh2D::AddVertex(knot_t pXi, knot_t pEta)
    {
        LockQuery();
        TsVertex::Pointer pV = TsVertex::Pointer(new TsVertex(++mLastVertex, pXi, pEta));
        mVertices.push_back(pV);
        return pV;
    }

    /// Add a horizontal edge to the topology mesh. User must be responsible for the correctness of the underlying topology since no internal check is performed. However, horizontalness of the edge will be validated in EndConstruct()
    TsEdge::Pointer TsMesh2D::AddHEdge(TsVertex::Pointer pV1, TsVertex::Pointer pV2)
    {
        TsEdge::Pointer pE = TsEdge::Pointer(new TsHEdge(++mLastEdge, pV1, pV2));
        mEdges.push_back(pE);
//        std::cout << "add a horizontal edge " << pV1->Id() << " " << pV2->Id() << std::endl;
        return pE;
    }

    /// Add a vertical edge to the topology mesh. User must be responsible for the correctness of the underlying topology since no internal check is performed. However, verticalness of the edge will be validated in EndConstruct()
    TsEdge::Pointer TsMesh2D::AddVEdge(TsVertex::Pointer pV1, TsVertex::Pointer pV2)
    {
        TsEdge::Pointer pE = TsEdge::Pointer(new TsVEdge(++mLastEdge, pV1, pV2));
        mEdges.push_back(pE);
//        std::cout << "add a vertical edge " << pV1->Id() << " " << pV2->Id() << std::endl;
        return pE;
    }

    /// Read the T-splined mesh from file
    /// Remarks: need to be called after BeginConstruct() and before EndConstruct()
    void TsMesh2D::ReadFromFile(std::string fn)
    {
        std::ifstream infile(fn.c_str());
        
        std::string line;
        std::vector<std::string> words;
        int ReadMode = NO_READ;
        int Dim = 0;
        std::vector<std::pair<int, std::pair<int, int> > > HEdgeList;
        std::vector<std::pair<int, std::pair<int, int> > > VEdgeList;
        while(!infile.eof())
        {
            std::getline(infile, line);
            boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);

//            for(std::size_t i = 0; i < words.size(); ++i)
//                std::cout << " " << words[i];
//            std::cout << std::endl;

            if(words.size() != 0)
            {
                if(words[0] == std::string("Begin"))
                {
                    if(words.size() < 2)
                        KRATOS_THROW_ERROR(std::logic_error, "Missing statement for Begin", "")
                    if(words[1] == "Order")
                        ReadMode = READ_ORDER;
                    else if(words[1] == std::string("Knots"))
                        ReadMode = READ_KNOTS;
                    else if(words[1] == std::string("H-edges"))
                        ReadMode = READ_H_EDGES;
                    else if(words[1] == std::string("V-edges"))
                        ReadMode = READ_V_EDGES;
                    continue;
                }
                
                if(words[0] == std::string("End"))
                {
                    Dim = 0;
                    ReadMode = NO_READ;
                    continue;
                }
                
                if(ReadMode == READ_ORDER)
                {
                    int Order = atoi(words[0].c_str());
                    this->SetOrder(Dim, Order);
                    ++Dim;
                }
                else if(ReadMode == READ_KNOTS)
                {
                    for(std::size_t i = 0; i < words.size(); ++i)
                        this->InsertKnot(Dim, atof(words[i].c_str()));
                    ++Dim;
                }
                else if(ReadMode == READ_H_EDGES)
                {
                    int v  = atoi(words[0].c_str());
                    int u1 = atoi(words[1].c_str());
                    int u2 = atoi(words[2].c_str());
                    HEdgeList.push_back(std::pair<int, std::pair<int, int> >(v, std::pair<int, int>(u1, u2)));
                }
                else if(ReadMode == READ_V_EDGES)
                {
                    int u  = atoi(words[0].c_str());
                    int v1 = atoi(words[1].c_str());
                    int v2 = atoi(words[2].c_str());
                    VEdgeList.push_back(std::pair<int, std::pair<int, int> >(u, std::pair<int, int>(v1, v2)));
                }
            }
        }
        
        infile.close();

        // assign the index to the knot vectors
        int cnt = -1;
        for(std::size_t i = 0; i < mKnots1.size(); ++i)
            mKnots1[i]->UpdateIndex(++cnt);
        cnt = -1;
        for(std::size_t i = 0; i < mKnots2.size(); ++i)
            mKnots2[i]->UpdateIndex(++cnt);
        
        // create the vertex list
        std::set<std::pair<int, int> > VertexList;
        for(std::size_t i = 0; i < HEdgeList.size(); ++i)
        {
            VertexList.insert(std::pair<int, int>(HEdgeList[i].second.first, HEdgeList[i].first));
            VertexList.insert(std::pair<int, int>(HEdgeList[i].second.second, HEdgeList[i].first));
        }
        for(std::size_t i = 0; i < VEdgeList.size(); ++i)
        {
            VertexList.insert(std::pair<int, int>(VEdgeList[i].first, VEdgeList[i].second.first));
            VertexList.insert(std::pair<int, int>(VEdgeList[i].first, VEdgeList[i].second.second));
        }

        // insert the vertices to the T-splines mesh
        std::map<std::pair<int, int>, TsVertex::Pointer> VertexMap;
        for(std::set<std::pair<int, int> >::iterator it = VertexList.begin(); it != VertexList.end(); ++it)
        {
            TsVertex::Pointer pV = this->AddVertex(mKnots1[it->first], mKnots2[it->second]);
            VertexMap[std::pair<int, int>(it->first, it->second)] = pV;
        }

        // insert the edges to the T-splines mesh
        for(std::size_t i = 0; i < HEdgeList.size(); ++i)
        {
            TsVertex::Pointer pV1 = VertexMap[std::pair<int, int>(HEdgeList[i].second.first, HEdgeList[i].first)];
            TsVertex::Pointer pV2 = VertexMap[std::pair<int, int>(HEdgeList[i].second.second, HEdgeList[i].first)];
            this->AddHEdge(pV1, pV2);
        }
        for(std::size_t i = 0; i < VEdgeList.size(); ++i)
        {
            TsVertex::Pointer pV1 = VertexMap[std::pair<int, int>(VEdgeList[i].first, VEdgeList[i].second.first)];
            TsVertex::Pointer pV2 = VertexMap[std::pair<int, int>(VEdgeList[i].first, VEdgeList[i].second.second)];
            this->AddVEdge(pV1, pV2);
        }
    }

    /// Lock T-splines mesh construction and perform validity check for the T-splines mesh
    void TsMesh2D::EndConstruct()
    {
        mLockConstruct = true;

        // check the sequence of knot vectors if it is arranged in ascending order
        if(mKnots1.size() != 0)
            for(std::size_t i = 0; i < mKnots1.size() - 1; ++i)
                if(mKnots1[i+1]->Value() < mKnots1[i]->Value())
                    KRATOS_THROW_ERROR(std::logic_error, "The knot vector in u-direction is not ascending at i =", i)
        std::cout << "Check OK! The knot vector in u-direction is in ascending order" << std::endl;
        if(mKnots2.size() != 0)
            for(std::size_t i = 0; i < mKnots2.size() - 1; ++i)
                if(mKnots2[i+1]->Value() < mKnots2[i]->Value())
                    KRATOS_THROW_ERROR(std::logic_error, "The knot vector in v-direction is not ascending at i =", i)
        std::cout << "Check OK! The knot vector in v-direction is in ascending order" << std::endl;
        
        // check for the repetition of knots at first p values and last p values
        mKnots1Min = mKnots1.front()->Value();
        mKnots1Max = mKnots1.back()->Value();
        mKnots2Min = mKnots2.front()->Value();
        mKnots2Max = mKnots2.back()->Value();
//        KRATOS_WATCH(mKnots1Min)
//        KRATOS_WATCH(mKnots1Max)
//        KRATOS_WATCH(mKnots2Min)
//        KRATOS_WATCH(mKnots2Max)
        for(std::size_t i = 0; i < mOrder1; ++i)
        {
            if(mKnots1[i]->Value() != mKnots1Min)
                KRATOS_THROW_ERROR(std::logic_error, "Knots1 does not repeat at begin. Error at knot", i)
            if(mKnots1[mKnots1.size() - i - 1]->Value() != mKnots1Max)
                KRATOS_THROW_ERROR(std::logic_error, "Knots1 does not repeat at end. Error at knot", i)
        }
        for(std::size_t i = 0; i < mOrder2; ++i)
        {
            if(mKnots2[i]->Value() != mKnots2Min)
                KRATOS_THROW_ERROR(std::logic_error, "Knots2 does not repeat at begin. Error at knot", i)
            if(mKnots2[mKnots2.size() - i - 1]->Value() != mKnots2Max)
                KRATOS_THROW_ERROR(std::logic_error, "Knots2 does not repeat at end. Error at knot", i)
        }
        std::cout << "Check OK! The knot vector satisfies repetitiveness condition" << std::endl;
        
        // update the indexing of knot vectors
        int cnt = -1;
        for(std::size_t i = 0; i < mKnots1.size(); ++i)
            mKnots1[i]->UpdateIndex(++cnt);
        cnt = -1;
        for(std::size_t i = 0; i < mKnots2.size(); ++i)
            mKnots2[i]->UpdateIndex(++cnt);
        std::cout << "The indexing for knots is updated" << std::endl;
        
        // By default, set first p knots and last p knots to inactive state
        for(std::size_t i = 0; i < mKnots1.size(); ++i)
        {
            if((i < mOrder1) or (i > mKnots1.size() - mOrder1 - 1))
                mKnots1[i]->SetActive(false);
            else
                mKnots1[i]->SetActive(true);
        }
        for(std::size_t i = 0; i < mKnots2.size(); ++i)
        {
            if((i < mOrder2) or (i > mKnots2.size() - mOrder2 - 1))
                mKnots2[i]->SetActive(false);
            else
                mKnots2[i]->SetActive(true);
        }
        
        // check if all vertices contain the knots in the knot vector
        // If one vertex contain a knot that is not in the knot vectors of the T-splines mesh, then a compatibility error should happen
        for(vertex_container_t::iterator it = mVertices.begin(); it != mVertices.end(); ++it)
        {
            if(std::find(mKnots1.begin(), mKnots1.end(), (*it)->pXi()) == mKnots1.end())
                KRATOS_THROW_ERROR(std::logic_error, "The u-knot vector does not contain knot at", *(*it))
            if(std::find(mKnots2.begin(), mKnots2.end(), (*it)->pEta()) == mKnots2.end())
                KRATOS_THROW_ERROR(std::logic_error, "The v-knot vector does not contain knot at", *(*it))
        }
        std::cout << "Check OK! All vertices contain knots in knot vectors" << std::endl;
        
        // check if all edges contain the vertices in the T-splines mesh
        for(edge_container_t::const_iterator it = mEdges.begin(); it != mEdges.end(); ++it)
        {
            if(std::find(mVertices.begin(), mVertices.end(), (*it)->pV1()) == mVertices.end()
               or std::find(mVertices.begin(), mVertices.end(), (*it)->pV2()) == mVertices.end())
                KRATOS_THROW_ERROR(std::logic_error, "The edge does not contain a vertex in the vertex list, wrong edge is", (*it)->Id())
        }
        std::cout << "Check OK! All edges contain vertices in the vertex list" << std::endl;
        
        // check for the horizontalness and verticalness of the edges
        for(edge_container_t::const_iterator it = mEdges.begin(); it != mEdges.end(); ++it)
        {
            if((*it)->EdgeType() == TsEdge::VERTICAL_EDGE) //vertical edge
            {
                if((*it)->pV1()->Index1() != (*it)->pV2()->Index1())
                    KRATOS_THROW_ERROR(std::logic_error, "An incompatible horizontal edge is found:", *(*it))
            }
            else if((*it)->EdgeType() == TsEdge::HORIZONTAL_EDGE) //horizontal edge
            {
                if((*it)->pV1()->Index2() != (*it)->pV2()->Index2())
                    KRATOS_THROW_ERROR(std::logic_error, "An incompatible vertical edge is found:", *(*it))
            }
            else
                KRATOS_THROW_ERROR(std::logic_error, "An unknown edge is found", *(*it))
        }
        std::cout << "Check OK! All edge vertical/horizontal configurations are valid" << std::endl;
        
        // set the type for vertex
        typedef std::map<TsVertex::Pointer, std::vector<TsEdge::Pointer> > vertex_neighbour_type;
        vertex_neighbour_type VertexNeighbours;
        for(edge_container_t::const_iterator it = mEdges.begin(); it != mEdges.end(); ++it)
        {
            VertexNeighbours[(*it)->pV1()].push_back(*it);
            VertexNeighbours[(*it)->pV2()].push_back(*it);
        }
        std::cout << "Detect vertex neighbours completed" << std::endl;
        int num_t_joints = 0;
        for(vertex_neighbour_type::iterator it = VertexNeighbours.begin(); it != VertexNeighbours.end(); ++it)
        {
            // check for border vertex
            if((it->first->pXi()->Value() == mKnots1Min) or (it->first->pXi()->Value() == mKnots1Max)
                or (it->first->pEta()->Value() == mKnots2Min) or (it->first->pEta()->Value() == mKnots2Max))
            {
                it->first->SetType(TsVertex::BORDER_JOINT);
//                std::cout << "Border joint is detected at " << it->first->Index1() << " " << it->first->Index2() << std::endl;
                continue;
            }

            // if not border vertex, then check for joint type
            if(it->first->IsActive())
            {
//                std::cout << *(it->first) << " has " << it->second.size() << " neighbours" << std::endl;
                if(it->second.size() == 4) // a normal joint
                    it->first->SetType(TsVertex::NORMAL_JOINT);
                else if(it->second.size() == 3) // a T joint
                {
                    int num_horizontal_edges = 0;
                    int num_vertical_edges = 0;
                    for(std::size_t i = 0; i < it->second.size(); ++i)
                    {
                        if(it->second[i]->EdgeType() == TsEdge::HORIZONTAL_EDGE)
                            ++num_horizontal_edges;
                        else if(it->second[i]->EdgeType() == TsEdge::VERTICAL_EDGE)
                            ++num_vertical_edges;
                        else
                            KRATOS_THROW_ERROR(std::logic_error, "An incompatible edge was found in neighbours set at vertex", *(it->first))
                    }
//                    KRATOS_WATCH(num_vertical_edges)
//                    KRATOS_WATCH(num_horizontal_edges)
                    
                    if(num_horizontal_edges > num_vertical_edges)
                    {
                        // detect T-joint UP/DOWN, check for the vertical edge
                        for(std::size_t i = 0; i < it->second.size(); ++i)
                            if(it->second[i]->EdgeType() == TsEdge::VERTICAL_EDGE)
                            {
                                int sum = it->second[i]->pV1()->Index2() + it->second[i]->pV2()->Index2();
                                if(sum > (2 * it->first->Index2())) // face downward
                                {
                                    it->first->SetType(TsVertex::T_JOINT_DOWN);
                                    std::cout << *(it->first) << " is set to T_JOINT_DOWN" << std::endl;
                                }
                                else // face upward
                                {
                                    it->first->SetType(TsVertex::T_JOINT_UP);
                                    std::cout << *(it->first) << " is set to T_JOINT_UP" << std::endl;
                                }
                                break;
                            }
                    }
                    else if(num_horizontal_edges < num_vertical_edges)
                    {
                        // detect T-joint LEFT/RIGHT, check for the horizontal edge
                        for(std::size_t i = 0; i < it->second.size(); ++i)
                            if(it->second[i]->EdgeType() == TsEdge::HORIZONTAL_EDGE)
                            {
                                int sum = it->second[i]->pV1()->Index1() + it->second[i]->pV2()->Index1();
                                if(sum > (2 * it->first->Index1())) // face to the left
                                {
                                    it->first->SetType(TsVertex::T_JOINT_LEFT);
                                    std::cout << *(it->first) << " is set to T_JOINT_LEFT" << std::endl;
                                }
                                else // face to the right
                                {
                                    it->first->SetType(TsVertex::T_JOINT_RIGHT);
                                    std::cout << *(it->first) << " is set to T_JOINT_RIGHT" << std::endl;
                                }
                                break;
                            }
                    }
                    else
                        KRATOS_THROW_ERROR(std::logic_error, "Error detecting T-joint at vertex", *(it->first))

                    ++num_t_joints;
                }
                else if(it->second.size() == 2)
                {
                    KRATOS_THROW_ERROR(std::logic_error, "L-joint and I-joint is not supported yet. Error found at vertex", *(it->first))
                }
                else
                    KRATOS_THROW_ERROR(std::logic_error, "Error finding neighbour at vertex", *(it->first))
            }
        }
        std::cout << "Check joint type successfully. There are " << num_t_joints << " T-joints in the T-splines topology mesh" << std::endl;
    }

    /*****************************************************************************/
    /* END SUBROUTINES TO CONSTRUCT THE T-MESH */
    /*****************************************************************************/




    /*****************************************************************************/
    /* BEGIN SUBROUTINES TO QUERY THE T-MESH */
    /*****************************************************************************/

    /// Find all cells in the T-splines topology mesh. If extend is true, it will find all cells with account to virtual edges
    /// Algorithm: scanning algorithm
    /// Remarks: this algorithm cannot work if the T-splines topology mesh contain L-joint.
    ///          It assumes that every cell is a rectangular cell. However, it will always
    ///          work with the extended T-splines mesh since extended T-splines mesh contains
    ///          no L-joints.
    void TsMesh2D::FindCells(std::set<cell_t>& rCells, bool _extend) const
    {
        // empty the cells
        rCells.clear();
    
        // firstly make a vertical scanning to identify the horizontal segment
        std::vector<std::pair<double, std::set<int> > > HorizontalSegments;
        bool is_active_edge;
        for(std::size_t i = 0; i < mKnots2.size() - 1; ++i)
        {
            int index_low  = mKnots2[i]->Index();
            int index_high = mKnots2[i+1]->Index();
            double index_eta = 0.5 * (double)(index_low + index_high);
            
            std::set<int> Segments;
            for(edge_container_t::const_iterator it_edge = mEdges.begin(); it_edge != mEdges.end(); ++it_edge)
            {
                if(_extend)
                    is_active_edge = (*it_edge)->EdgeType() == TsEdge::VERTICAL_EDGE or (*it_edge)->EdgeType() == TsEdge::VIRTUAL_VERTICAL_EDGE;
                else
                    is_active_edge = (*it_edge)->EdgeType() == TsEdge::VERTICAL_EDGE;

                is_active_edge = is_active_edge and (*it_edge)->IsActive();

                if(is_active_edge) //active vertical edge
                    if((*it_edge)->IsCut(index_eta))
                        Segments.insert((*it_edge)->Index());
            }
            if(!Segments.empty())
                HorizontalSegments.push_back(std::pair<double, std::set<int> >(index_eta, Segments));
        }

//        std::cout << "HorizontalSegments:" << std::endl;
//        for(std::size_t i = 0; i < HorizontalSegments.size(); ++i)
//        {
//            for(std::set<int>::iterator it = HorizontalSegments[i].second.begin(); it != HorizontalSegments[i].second.end(); ++it)
//                std::cout << " " << *it;
//            std::cout << std::endl;
//        }

        // secondly make a horizontal scanning and identify possible intersection
        for(std::size_t i = 0; i < mKnots1.size() - 1; ++i)
        {
            int index_low  = mKnots1[i]->Index();
            int index_high = mKnots1[i+1]->Index();
            double index_xi = 0.5 * (double)(index_low + index_high);
            
            std::set<int> Segments;
            for(edge_container_t::const_iterator it_edge = mEdges.begin(); it_edge != mEdges.end(); ++it_edge)
            {
                if(_extend)
                    is_active_edge = (*it_edge)->EdgeType() == TsEdge::HORIZONTAL_EDGE or (*it_edge)->EdgeType() == TsEdge::VIRTUAL_HORIZONTAL_EDGE;
                else
                    is_active_edge = (*it_edge)->EdgeType() == TsEdge::HORIZONTAL_EDGE;

                is_active_edge = is_active_edge and (*it_edge)->IsActive();

                if(is_active_edge) //active horizontal edge
                    if((*it_edge)->IsCut(index_xi))
                        Segments.insert((*it_edge)->Index());
            }
            if(!Segments.empty())
            {
                // identify which segment in every row of horizontal segments this vertical ray cut
                std::vector<std::pair<int, int> > cut_segments;
                for(std::size_t j = 0; j < HorizontalSegments.size(); ++j)
                {
                    bool detect = false;
                    std::set<int>::iterator it_old = HorizontalSegments[j].second.begin();
                    std::set<int>::iterator it_begin = it_old;
                    for(std::set<int>::iterator it = it_begin; it != HorizontalSegments[j].second.end(); ++it)
                    {
                        if(*it > index_xi)
                        {
                            cut_segments.push_back(std::pair<int, int>(*it_old, *it));
                            detect = true;
                            break;
                        }
                        it_old = it;
                    }
                    if(detect == false)
                        KRATOS_THROW_ERROR(std::logic_error, "ERROR: cannot detect the intersection", "")
                }
                
                // now we make the box intersection
                std::vector<int> Temp(Segments.begin(), Segments.end());
                for(std::size_t j = 0; j < Temp.size() - 1; ++j)
                {
                    for(std::size_t k = 0; k < HorizontalSegments.size(); ++k)
                    {
                        if((HorizontalSegments[k].first > Temp[j]) and (HorizontalSegments[k].first < Temp[j+1]))
                        {
//                            std::cout << "Found box " << cut_segments[k].first << " " << cut_segments[k].second
//                                      << " " << Temp[j] << " " << Temp[j+1] << std::endl;
                            rCells.insert(cell_t(std::pair<int, int>(cut_segments[k].first, cut_segments[k].second),
                                                    std::pair<int, int>(Temp[j], Temp[j+1])));
                        }
                    }
                }
            }
        }
        
//        std::cout << "Found " << rCells.size() << " cells in the T-splines topology mesh" << std::endl;
//        for(std::set<cell_t>::iterator it = rCells.begin(); it != rCells.end(); ++it)
//            std::cout << "cell " << it->first.first << " " << it->first.second
//                          << " " << it->second.first << " " << it->second.second << std::endl;
    }

    /// Check for the analysis-suitable property by checking the intersection of virtual edges
    bool TsMesh2D::IsAnalysisSuitable()
    {
        bool isAnalysisSuitable = true;
//        std::vector<std::pair<TsEdge::Pointer, TsEdge::Pointer> > CuttingCouples;
        
        // firstly seperate out the virtual horizontal edges and virtual vertical edges
        std::vector<TsEdge::Pointer> VirtualHorizontalEdges;
        std::vector<TsEdge::Pointer> VirtualVerticalEdges;
        for(edge_container_t::iterator it = mEdges.begin(); it != mEdges.end(); ++it)
        {
            if((*it)->EdgeType() == TsEdge::VIRTUAL_HORIZONTAL_EDGE)
                VirtualHorizontalEdges.push_back(*it);
            else if((*it)->EdgeType() == TsEdge::VIRTUAL_VERTICAL_EDGE)
                VirtualVerticalEdges.push_back(*it);
        }
        
        // secondly check for each horizontal edges if it was cut by any virtual vertical edges
        for(std::size_t i = 0; i < VirtualHorizontalEdges.size(); ++i)
        {
            for(std::size_t j = 0; j < VirtualVerticalEdges.size(); ++j)
            {
                if(VirtualHorizontalEdges[i]->IsCut(VirtualVerticalEdges[j]->Index())
                   and VirtualVerticalEdges[j]->IsCut(VirtualHorizontalEdges[i]->Index()))
                {
                    isAnalysisSuitable = false;
                    return isAnalysisSuitable;
//                    CuttingCouples.push_back(std::pair<TsEdge::Pointer, TsEdge::Pointer>(VirtualHorizontalEdges[i], VirtualVerticalEdges[j]));
                }
            }
        }
        
        return isAnalysisSuitable;
    }

    /*****************************************************************************/
    /* END SUBROUTINES TO QUERY THE T-MESH */
    /*****************************************************************************/

    /*****************************************************************************/
    /* BEGIN SUBROUTINES TO MODIFY THE T-MESH */
    /*****************************************************************************/

    /// Renumber all edges and vertices in the Tsplines mesh. This is used after mesh refinement to ensure unique edge ids.
    void TsMesh2D::RenumberMesh()
    {
        mLastVertex = 0;
        for(vertex_container_t::iterator it = mVertices.begin(); it != mVertices.end(); ++it)
            (*it)->SetId(++mLastVertex);
        mLastEdge = 0;
        for(edge_container_t::iterator it = mEdges.begin(); it != mEdges.end(); ++it)
            (*it)->SetId(++mLastEdge);
    }
    
    /*****************************************************************************/
    /* END SUBROUTINES TO MODIFY THE T-MESH */
    /*****************************************************************************/


    /*****************************************************************************/
    /* AUXILLIARY SUBROUTINES */
    /*****************************************************************************/

    /// Clear the extension part of the T-splines topology mesh. This includes virtual edges and virtual vertices
    void TsMesh2D::ClearExtendedTmesh()
    {
        // firstly clear all existing virtual edges
        for(edge_container_t::iterator it = mEdges.begin(); it != mEdges.end();)
            if((*it)->EdgeType() == TsEdge::VIRTUAL_HORIZONTAL_EDGE or (*it)->EdgeType() == TsEdge::VIRTUAL_VERTICAL_EDGE)
                mEdges.erase(it++);
            else
                ++it;

        // clear all virtual vertices
        mVirtualVertices.clear();
        
        // reset the flag
        mIsExtended = false;
    }

    /// Build the extended T-splines topology mesh by extending the T-joints
    void TsMesh2D::BuildExtendedTmesh()
    {
        // firstly clean the extended T-splines topology mesh
        if(mIsExtended == true)
            this->ClearExtendedTmesh();
        
        // iterate through all vertices to check for T-joint and add the virtual entities
        for(vertex_container_t::iterator it = mVertices.begin(); it != mVertices.end(); ++it)
        {
            int xi_index = (*it)->Index1();
            int eta_index = (*it)->Index2();
            if((*it)->Type() == TsVertex::T_JOINT_LEFT)
            {
//                std::cout << "start adding virtual entities for " << *(*it) << std::endl;
                // marching to the left
                std::set<int> tmp_knot_index_left;
                for(edge_container_t::iterator it_edge = mEdges.begin(); it_edge != mEdges.end(); ++it_edge)
                {
                    if((*it_edge)->EdgeType() == TsEdge::VERTICAL_EDGE) //vertical edge
                    {
                        int edge_xi_index = (*it_edge)->Index();
                        if((*it_edge)->IsCut(eta_index) and edge_xi_index < xi_index)
                            tmp_knot_index_left.insert(edge_xi_index);
                    }
                }
//                std::cout << *(*it) << " marching completed" << std::endl;

                // insert virtual vertices
                std::vector<int> tmp_left(tmp_knot_index_left.begin(), tmp_knot_index_left.end());
                int span = (mOrder1 % 2 == 0) ? (mOrder1 / 2 + 1) : (mOrder1 + 1) / 2;
                std::vector<TsVertex::Pointer> new_virtual_vertices;
                TsVertex::Pointer p_vertex;
                for(std::size_t i = 0; i < span; ++i)
                {
                    int new_xi_index = *(tmp_left.end() - span + i);
                    p_vertex = TsVertex::Pointer(new TsVertex(++mLastVertex, mKnots1[new_xi_index], mKnots2[eta_index]));
                    new_virtual_vertices.push_back(p_vertex);
                }
                mVirtualVertices.insert(mVirtualVertices.end(), new_virtual_vertices.begin(), new_virtual_vertices.end());
//                std::cout << *(*it) << " insert virtual vertices completed" << std::endl;

                // insert virtual edges
                TsEdge::Pointer p_edge;
                p_edge = TsEdge::Pointer(new TsVirtualHEdge(++mLastEdge, *it, new_virtual_vertices[0]));
                mEdges.push_back(p_edge);
                for(std::size_t i = 0; i < new_virtual_vertices.size() - 1; ++i)
                {
                    p_edge = TsEdge::Pointer(new TsVirtualHEdge(++mLastEdge, new_virtual_vertices[i], new_virtual_vertices[i+1]));
                    mEdges.push_back(p_edge);
                }
//                std::cout << *(*it) << " insert virtual edges completed" << std::endl;
            }
            else if((*it)->Type() == TsVertex::T_JOINT_RIGHT)
            {
//                std::cout << "start adding virtual entities for " << *(*it) << std::endl;
                // marching to the left
                std::set<int> tmp_knot_index_right;
                for(edge_container_t::iterator it_edge = mEdges.begin(); it_edge != mEdges.end(); ++it_edge)
                {
                    if((*it_edge)->EdgeType() == TsEdge::VERTICAL_EDGE) //vertical edge
                    {
                        int edge_xi_index = (*it_edge)->Index();
                        if((*it_edge)->IsCut(eta_index) and edge_xi_index > xi_index)
                            tmp_knot_index_right.insert(edge_xi_index);
                    }
                }
//                std::cout << *(*it) << " marching completed" << std::endl;

                // insert virtual vertices
                std::vector<int> tmp_right(tmp_knot_index_right.begin(), tmp_knot_index_right.end());
                int span = (mOrder1 % 2 == 0) ? (mOrder1 / 2 + 1) : (mOrder1 + 1) / 2;
                std::vector<TsVertex::Pointer> new_virtual_vertices;
                TsVertex::Pointer p_vertex;
                for(std::size_t i = 0; i < span; ++i)
                {
                    int new_xi_index = *(tmp_right.begin() + i);
                    p_vertex = TsVertex::Pointer(new TsVertex(++mLastVertex, mKnots1[new_xi_index], mKnots2[eta_index]));
                    new_virtual_vertices.push_back(p_vertex);
                }
                mVirtualVertices.insert(mVirtualVertices.end(), new_virtual_vertices.begin(), new_virtual_vertices.end());
//                std::cout << *(*it) << " insert virtual vertices completed" << std::endl;

                // insert virtual edges
                TsEdge::Pointer p_edge;
                p_edge = TsEdge::Pointer(new TsVirtualHEdge(++mLastEdge, *it, new_virtual_vertices[0]));
                mEdges.push_back(p_edge);
                for(std::size_t i = 0; i < new_virtual_vertices.size() - 1; ++i)
                {
                    p_edge = TsEdge::Pointer(new TsVirtualHEdge(++mLastEdge, new_virtual_vertices[i], new_virtual_vertices[i+1]));
                    mEdges.push_back(p_edge);
                }
//                std::cout << *(*it) << " insert virtual edges completed" << std::endl;
            }
            else if((*it)->Type() == TsVertex::T_JOINT_UP)
            {
//                std::cout << "start adding virtual entities for " << *(*it) << std::endl;
                // marching to the left
                std::set<int> tmp_knot_index_up;
                for(edge_container_t::iterator it_edge = mEdges.begin(); it_edge != mEdges.end(); ++it_edge)
                {
                    if((*it_edge)->EdgeType() == TsEdge::HORIZONTAL_EDGE) //horizontal edge
                    {
                        int edge_eta_index = (*it_edge)->Index();
                        if((*it_edge)->IsCut(xi_index) and edge_eta_index > eta_index)
                            tmp_knot_index_up.insert(edge_eta_index);
                    }
                }
//                std::cout << *(*it) << " marching completed" << std::endl;

                // insert virtual vertices
                std::vector<int> tmp_up(tmp_knot_index_up.begin(), tmp_knot_index_up.end());
                int span = (mOrder2 % 2 == 0) ? (mOrder2 / 2 + 1) : (mOrder2 + 1) / 2;
                std::vector<TsVertex::Pointer> new_virtual_vertices;
                TsVertex::Pointer p_vertex;
                for(std::size_t i = 0; i < span; ++i)
                {
                    int new_eta_index = *(tmp_up.begin() + i);
                    p_vertex = TsVertex::Pointer(new TsVertex(++mLastVertex, mKnots2[xi_index], mKnots2[new_eta_index]));
                    new_virtual_vertices.push_back(p_vertex);
                }
                mVirtualVertices.insert(mVirtualVertices.end(), new_virtual_vertices.begin(), new_virtual_vertices.end());
//                std::cout << *(*it) << " insert virtual vertices completed" << std::endl;

                // insert virtual edges
                TsEdge::Pointer p_edge;
                p_edge = TsEdge::Pointer(new TsVirtualVEdge(++mLastEdge, *it, new_virtual_vertices[0]));
                mEdges.push_back(p_edge);
                for(std::size_t i = 0; i < new_virtual_vertices.size() - 1; ++i)
                {
                    p_edge = TsEdge::Pointer(new TsVirtualVEdge(++mLastEdge, new_virtual_vertices[i], new_virtual_vertices[i+1]));
                    mEdges.push_back(p_edge);
                }
//                std::cout << *(*it) << " insert virtual edges completed" << std::endl;
            }
            else if((*it)->Type() == TsVertex::T_JOINT_DOWN)
            {
//                std::cout << "start adding virtual entities for " << *(*it) << std::endl;
                // marching to the left
                std::set<int> tmp_knot_index_down;
                for(edge_container_t::iterator it_edge = mEdges.begin(); it_edge != mEdges.end(); ++it_edge)
                {
                    if((*it_edge)->EdgeType() == TsEdge::HORIZONTAL_EDGE) //horizontal edge
                    {
                        int edge_eta_index = (*it_edge)->Index();
                        if((*it_edge)->IsCut(xi_index) and edge_eta_index < eta_index)
                            tmp_knot_index_down.insert(edge_eta_index);
                    }
                }
//                std::cout << *(*it) << " marching completed" << std::endl;

                // insert virtual vertices
                std::vector<int> tmp_down(tmp_knot_index_down.begin(), tmp_knot_index_down.end());
                int span = (mOrder2 % 2 == 0) ? (mOrder2 / 2 + 1) : (mOrder2 + 1) / 2;
                std::vector<TsVertex::Pointer> new_virtual_vertices;
                TsVertex::Pointer p_vertex;
                for(std::size_t i = 0; i < span; ++i)
                {
                    int new_eta_index = *(tmp_down.end() - span + i);
                    p_vertex = TsVertex::Pointer(new TsVertex(++mLastVertex, mKnots2[xi_index], mKnots2[new_eta_index]));
                    new_virtual_vertices.push_back(p_vertex);
                }
                mVirtualVertices.insert(mVirtualVertices.end(), new_virtual_vertices.begin(), new_virtual_vertices.end());
//                std::cout << *(*it) << " insert virtual vertices completed" << std::endl;

                // insert virtual edges
                TsEdge::Pointer p_edge;
                p_edge = TsEdge::Pointer(new TsVirtualVEdge(++mLastEdge, *it, new_virtual_vertices[0]));
                mEdges.push_back(p_edge);
                for(std::size_t i = 0; i < new_virtual_vertices.size() - 1; ++i)
                {
                    p_edge = TsEdge::Pointer(new TsVirtualVEdge(++mLastEdge, new_virtual_vertices[i], new_virtual_vertices[i+1]));
                    mEdges.push_back(p_edge);
                }
//                std::cout << *(*it) << " insert virtual edges completed" << std::endl;
            }
        }
        
        // set the flag
        mIsExtended = true;
    }

    /// Build the anchors structure, given the file to provide coordinates and Id of the anchors
    void TsMesh2D::BuildAnchors(std::string fn)
    {
        // clear all existing anchors
        mAnchors.clear();
    
        // firstly find all anchors in the T-splines topology mesh
        std::vector<anchor_t> Anchors;
        this->FindAnchors(Anchors);
        std::cout << "Find anchors completed, number of anchors = " << Anchors.size() << std::endl;

        // secondly read from file and extract coordinates and Id
        std::ifstream infile(fn.c_str());
        std::string line;
        std::vector<std::string> words;
        int ReadMode = NO_READ;
        double tol = 1.0e-6;
        TsAnchor::Pointer pAnchor;
        int Id;
        double dist, Xi, Eta, Zeta, X, Y, Z, W;
        bool found;
        int num_lines = 0;
        while(!infile.eof())
        {
            std::getline(infile, line);
            ++num_lines;
            boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);

//            for(std::size_t i = 0; i < words.size(); ++i)
//                std::cout << " " << words[i];
//            std::cout << std::endl;

            if(words.size() != 0)
            {
                if(words[0] == std::string("Begin"))
                {
                    if(words.size() < 2)
                        KRATOS_THROW_ERROR(std::logic_error, "Missing statement for Begin", "")
                    if(words[1] == "Anchors")
                        ReadMode = READ_ANCHORS;
                    continue;
                }
                
                if(words[0] == std::string("End"))
                {
                    ReadMode = NO_READ;
                    continue;
                }

                if(ReadMode == READ_ANCHORS)
                {
                    if(words.size() < 8)
                        KRATOS_THROW_ERROR(std::logic_error, "There are not enough number of parameters at line", num_lines)

                    Id   = atoi(words[0].c_str());
                    Xi   = atof(words[1].c_str());
                    Eta  = atof(words[2].c_str());
                    Zeta = atof(words[3].c_str());
                    X    = atof(words[4].c_str());
                    Y    = atof(words[5].c_str());
                    Z    = atof(words[6].c_str());
                    W    = atof(words[7].c_str());
                    
                    // find if the provided anchor exist in the anchor list
                    found = false;
                    for(std::vector<anchor_t>::iterator it = Anchors.begin(); it != Anchors.end(); ++it)
                    {
                        dist = sqrt(pow(Xi - (*it).first, 2) + pow(Eta - (*it).second, 2));
                        if(dist < tol)
                        {
                            pAnchor = TsAnchor::Pointer(new TsAnchor(Id, (*it).first, (*it).second, X, Y, W));
                            mAnchors.push_back(pAnchor);
                            found = true;
                            break;
                        }
                    }

                    // if there is no matching, give a warning
                    if(!found)
                        std::cout << "WARNING!!! Anchor (" << Xi << ", " << Eta << ") does not exist in the T-splines topology mesh" << std::endl;
                }
            }
        }
        
        infile.close();
        std::cout << "Build anchors completed, " << mAnchors.size() << " is read" << std::endl;
    }

    /// Construct the internal data for cells (including the supported anchors)
    /// Remarks: the T-splines topology mesh must be extended before calling this function
    void TsMesh2D::BuildCells()
    {
        if(!mIsExtended)
            KRATOS_THROW_ERROR(std::logic_error, "Extended T-splines mesh is not constructed yet", "")
    
        // firstly check if anchors has been found
        if(mAnchors.size() == 0)
            KRATOS_THROW_ERROR(std::logic_error, "The anchors size is zero", "")

        // clear the cell container
        if(!mCells.empty())
            mCells.clear();

        // secondly find all the cells of the extended T-splines topology mesh
        std::set<cell_t> cell_covers;
        this->FindCells(cell_covers, true);
        Cell::Pointer pCell;
        double tol = 1.0e-6;
        int LastCell = 0;
        for(std::set<cell_t>::iterator it = cell_covers.begin(); it != cell_covers.end(); ++it)
        {
            // check if the cell area is larger than zero. This to prevent the case of repetitive knot values, then
            // the cell will be degenerated. In this case, the degenerated cells will be ignored.
            double area = fabs(mKnots1[(*it).first.first]->Value() - mKnots1[(*it).first.second]->Value()) *
                          fabs(mKnots2[(*it).second.first]->Value() - mKnots2[(*it).second.second]->Value());
            if(area > tol)
            {
                pCell = Cell::Pointer(new Cell(++LastCell,
                                               mKnots1[(*it).first.first],
                                               mKnots1[(*it).first.second],
                                               mKnots2[(*it).second.first],
                                               mKnots2[(*it).second.second]));
                mCells.push_back(pCell);
            }
        }
        std::cout << "Create cells completed, " << mCells.size() << " was created" << std::endl;
        
        // for each anchors search for the supported cells
        std::vector<int> KnotsIndex1;
        std::vector<int> KnotsIndex2;
        std::vector<double> Knots1;
        std::vector<double> Knots2;
        std::vector<Vector> Crows;
        int nb_xi, nb_eta;
        Vector Ubar_xi;
        Vector Ubar_eta;
        std::vector<double> Uxi, Ueta;
        std::vector<int> spans_xi, spans_eta;
        int temp, span_xi_after, span_eta_after;
        for(anchor_container_t::iterator it = mAnchors.begin(); it != mAnchors.end(); ++it)
        {
            double anchor_xi_index = (*it)->Xi();
            double anchor_eta_index = (*it)->Eta();
            
            // find the knot span supported by the anchor
            this->FindKnots<2, int>(anchor_xi_index, anchor_eta_index, KnotsIndex1, KnotsIndex2);

            // find the local knot vector of the anchor
            this->FindKnots<1, double>(anchor_xi_index, anchor_eta_index, Knots1, Knots2);
            
            // check if the knot span cover any cell
            for(cell_container_t::iterator it2 = mCells.begin(); it2 != mCells.end(); ++it2)
            {
                if((*it2)->IsCoverred(KnotsIndex1, KnotsIndex2))
                {
                    Uxi.clear();
                    Ueta.clear();
                    spans_xi.clear();
                    spans_eta.clear();
                
                    // compute the Bezier extraction operator of the anchor w.r.t the cell
                    // Remarks: right now, I don't know the method to articulate two Bezier extraction on two
                    //          consecutive knot spans, I have to compute the Bezier extraction operator at each
                    //          anchor w.r.t any cell sequentially. I know it is repetitive and expensive. I know it is approximately (mOrder1+1)(mOrder2+1) times more expensive than computing the extraction operator once for each anchor.
                    // TODO: to improve the algorithm of this method
                    // firstly we know the knot span of this cell
                    double left  = (*it2)->LeftValue();
                    double right = (*it2)->RightValue();
                    double up    = (*it2)->UpValue();
                    double down  = (*it2)->DownValue();
                    std::cout << "At anchor " << *(*it) << ", found cell" << *(*it2) << " with spans = (";
                    std::cout << mKnots1[left]->Value() << ", " << mKnots1[right]->Value() << ", ";
                    std::cout << mKnots2[down]->Value() << ", " << mKnots2[up]->Value() << ")" << std::endl;

                    // secondly we figure out at which knot span in the local knot vectors it covers
                    if(std::find(KnotsIndex1.begin(), KnotsIndex1.end(), left) == KnotsIndex1.end())
                    {
                        Uxi.push_back(mKnots1[left]->Value());
                        temp = this->FindSpanLocal(mKnots1[left]->Value(), Knots1);
                        spans_xi.push_back(temp);
                    }
                    if(std::find(KnotsIndex1.begin(), KnotsIndex1.end(), right) == KnotsIndex1.end())
                    {
                        Uxi.push_back(mKnots1[right]->Value());
                        temp = this->FindSpanLocal(mKnots1[right]->Value(), Knots1);
                        spans_xi.push_back(temp);
                    }
                    if(spans_xi.size() > 1)
                        KRATOS_THROW_ERROR(std::logic_error, "The cell must not terminate at more than one virtual vertex in u-direction", "")
                    std::cout << "Uxi:";
                    for(std::size_t i = 0; i < Uxi.size(); ++i)
                        std::cout << " " << Uxi[i];
                    std::cout << std::endl;
                    std::cout << "spans_xi:";
                    for(std::size_t i = 0; i < spans_xi.size(); ++i)
                        std::cout << " " << spans_xi[i];
                    std::cout << std::endl;

                    if(std::find(KnotsIndex2.begin(), KnotsIndex2.end(), down) == KnotsIndex2.end())
                    {
                        Ueta.push_back(mKnots2[down]->Value());
                        temp = this->FindSpanLocal(mKnots2[down]->Value(), Knots2);
                        spans_eta.push_back(temp);
                    }
                    if(std::find(KnotsIndex2.begin(), KnotsIndex2.end(), up) == KnotsIndex2.end())
                    {
                        Ueta.push_back(mKnots2[up]->Value());
                        temp = this->FindSpanLocal(mKnots2[up]->Value(), Knots2);
                        spans_eta.push_back(temp);
                    }
                    if(spans_eta.size() > 1)
                        KRATOS_THROW_ERROR(std::logic_error, "The cell must not terminate at more than one virtual vertex in v-direction", "")
                    std::cout << "Ueta:";
                    for(std::size_t i = 0; i < Ueta.size(); ++i)
                        std::cout << " " << Ueta[i];
                    std::cout << std::endl;
                    std::cout << "spans_eta:";
                    for(std::size_t i = 0; i < spans_eta.size(); ++i)
                        std::cout << " " << spans_eta[i];
                    std::cout << std::endl;

                    // compute the 2d bezier extraction operator
                    std::cout << "Knots1:";
                    for(std::size_t i = 0; i < Knots1.size(); ++i)
                        std::cout << " " << Knots1[i];
                    std::cout << std::endl;
                    std::cout << "Knots2:";
                    for(std::size_t i = 0; i < Knots2.size(); ++i)
                        std::cout << " " << Knots2[i];
                    std::cout << std::endl;
                    BezierUtils::bezier_extraction_tsplines_2d(Crows, nb_xi, nb_eta, Ubar_xi, Ubar_eta,
                                                               Knots1, Knots2, Uxi, Ueta, spans_xi, spans_eta,
                                                               mOrder1, mOrder2);
                    KRATOS_WATCH(nb_xi)
                    KRATOS_WATCH(nb_eta)
                    KRATOS_WATCH(Ubar_xi)
                    KRATOS_WATCH(Ubar_eta)
                    KRATOS_WATCH(Crows.size())
                    KRATOS_WATCH(Crows[0].size())

                    // find the knot span of the cell in the filled extended knot vector
                    std::set<double> Ubar_xi_set(Ubar_xi.begin(), Ubar_xi.end());
                    std::set<double> Ubar_eta_set(Ubar_eta.begin(), Ubar_eta.end());
                    std::vector<double> Ubar_xi_unique(Ubar_xi_set.begin(), Ubar_xi_set.end());
                    std::vector<double> Ubar_eta_unique(Ubar_eta_set.begin(), Ubar_eta_set.end());
                    span_xi_after = this->FindSpanLocal(0.5 * (mKnots1[left]->Value() + mKnots1[right]->Value()), Ubar_xi_unique);
                    span_eta_after = this->FindSpanLocal(0.5 * (mKnots2[down]->Value() + mKnots2[up]->Value()), Ubar_eta_unique);
                    KRATOS_WATCH(span_xi_after)
                    KRATOS_WATCH(span_eta_after)
                    
                    // add the Id of the anchor and the bezier extraction operator of the cell to the anchor to the internal data of the cell
                    (*it2)->AddAnchor((*it)->Id(), (*it)->W(), Crows[(span_xi_after - 1) * nb_eta + span_eta_after - 1]);
                    
                    std::cout << "---------------------------------------" << std::endl;
                }
            }
        }
        std::cout << "Find supported cell domain completed" << std::endl;

        for(cell_container_t::iterator it = mCells.begin(); it != mCells.end(); ++it)
        {
            std::cout << "cell " << *(*it) << std::endl;
        }
    }

    void TsMesh2D::PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "Tmesh details:" << std::endl;

        rOStream << "Order 1: " << mOrder1 << std::endl;
        rOStream << "Order 2: " << mOrder2 << std::endl;

        rOStream << "Knot vector 1:" << std::endl;
        for(std::size_t i = 0; i < mKnots1.size(); ++i)
            rOStream << " " << *(mKnots1[i]);
        rOStream << std::endl;
        rOStream << "Knot vector 2:" << std::endl;
        for(std::size_t i = 0; i < mKnots2.size(); ++i)
            rOStream << " " << *(mKnots2[i]);
        rOStream << std::endl;
        
        rOStream << "Vertex List:" << std::endl;
        for(vertex_container_t::const_iterator it = mVertices.begin(); it != mVertices.end(); ++it)
            rOStream << *(*it) << std::endl;
        
        rOStream << "Edge List:" << std::endl;
        for(edge_container_t::const_iterator it = mEdges.begin(); it != mEdges.end(); ++it)
            rOStream << *(*it) << std::endl;
    }

    /// Export the topology mesh/knot coordinates mesh to matlab
    void TsMesh2D::ExportMatlab(std::string fn, std::string mesh_type) const
    {
        std::ofstream outfile(fn.c_str());
        
        outfile << "axis equal" << std::endl;
        outfile << "close all" << std::endl;
        outfile << "hold on" << std::endl << std::endl;
        
        // plot edges
        if(mesh_type == std::string("topology"))
        {
            for(edge_container_t::const_iterator it = mEdges.begin(); it != mEdges.end(); ++it)
            {
                if((*it)->EdgeType() == TsEdge::VIRTUAL_HORIZONTAL_EDGE or (*it)->EdgeType() == TsEdge::VIRTUAL_VERTICAL_EDGE)
                {
                    outfile << "line([" << (*it)->pV1()->Index1() << " " << (*it)->pV2()->Index1() << "],";
                    outfile << "[" << (*it)->pV1()->Index2() << " " << (*it)->pV2()->Index2() << "],'LineStyle',':');" << std::endl;
                }
                else
                {
                    outfile << "line([" << (*it)->pV1()->Index1() << " " << (*it)->pV2()->Index1() << "],";
                    outfile << "[" << (*it)->pV1()->Index2() << " " << (*it)->pV2()->Index2() << "]);" << std::endl;
                }
            }
        }
        else if(mesh_type == std::string("knots"))
        {
            for(edge_container_t::const_iterator it = mEdges.begin(); it != mEdges.end(); ++it)
            {
                if((*it)->EdgeType() == TsEdge::VIRTUAL_HORIZONTAL_EDGE or (*it)->EdgeType() == TsEdge::VIRTUAL_VERTICAL_EDGE)
                {
                    outfile << "line([" << (*it)->pV1()->pXi()->Value() << " " << (*it)->pV2()->pXi()->Value() << "],";
                    outfile << "[" << (*it)->pV1()->pEta()->Value() << " " << (*it)->pV2()->pEta()->Value() << "],'LineStyle',':');" << std::endl;
                }
                else
                {
                    outfile << "line([" << (*it)->pV1()->pXi()->Value() << " " << (*it)->pV2()->pXi()->Value() << "],";
                    outfile << "[" << (*it)->pV1()->pEta()->Value() << " " << (*it)->pV2()->pEta()->Value() << "]);" << std::endl;
                }
            }
        }
        outfile << std::endl;
        
        // find all anchors in the current topology mesh
        std::vector<anchor_t> Anchors;
        this->FindAnchors(Anchors);
        
        // export the knot vectors for each anchors
        std::vector<double> Knots1;
        std::vector<double> Knots2;
        int cnt = 0;
        for(std::size_t i = 0; i < Anchors.size(); ++i)
        {
            this->FindKnots<1, double>(Anchors[i].first, Anchors[i].second, Knots1, Knots2);
            outfile << "local_knots(" << ++cnt << ",:,:) = [";
            for(std::size_t i = 0; i < Knots1.size(); ++i)
                outfile << " " << Knots1[i];
            outfile << std::endl;
            for(std::size_t i = 0; i < Knots2.size(); ++i)
                outfile << " " << Knots2[i];
            outfile << "];" << std::endl;
        }

        outfile.close();
        std::cout << "Exported to " << fn << " completed!" << std::endl;
        
        std::cout << "Find cells in the T-splines topology mesh..." << std::endl;
        std::set<cell_t> cells;
        this->FindCells(cells);
        
        std::cout << "Find cells in the extended T-splines topology mesh..." << std::endl;
        cells.clear();
        this->FindCells(cells, true);
    }

    /// Export the cells to mdpa format
    void TsMesh2D::ExportMDPA(std::string fn, int Division1, int Division2) const
    {
        std::ofstream outfile(fn.c_str());

        // write header
        outfile << "//KRATOS isogeometric application data file for T-splines\n";
        outfile << "//(c) 2015 Hoang Giang Bui, Ruhr-University Bochum\n";
        
        std::time_t t = time(0);
        struct tm* now = std::localtime(&t);
        outfile << "//This file is created on " << now->tm_mday << "/" << now->tm_mon << "/" << (now->tm_year + 1900)
                << " " << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << std::endl << std::endl;

        // write model_part data section
        outfile << "Begin ModelPartData\n";
        outfile << "End ModelPartData\n\n";

        // write properties
        outfile << "Begin Properties 1\n";
        outfile << "End Properties\n\n";

        // write nodes
        outfile << "Begin Nodes\n";
        for(anchor_container_t::const_iterator it = mAnchors.begin(); it != mAnchors.end(); ++it)
            outfile << (*it)->Id() << "\t" << (*it)->X() << "\t" << (*it)->Y() << "\t" << (*it)->Z() << std::endl;
        outfile << "End Nodes\n\n";

        // write elements
        outfile << "Begin Elements KinematicLinearGeo2dBezier\n";
        std::vector<unsigned int> AnchorIds;
        for(cell_container_t::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
        {
            AnchorIds = (*it)->GetSupportedAnchors();
            outfile << (*it)->Id() << " 1";
            for(std::vector<unsigned int>::iterator it2 = AnchorIds.begin(); it2 != AnchorIds.end(); ++it2)
                outfile << " " << (*it2);
            outfile << std::endl;
        }
        outfile << "End Elements\n\n";

        // write weights
        outfile << "Begin ElementalData NURBS_WEIGHT\n";
        std::vector<double> AnchorWeights;
        for(cell_container_t::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
        {
            AnchorWeights = (*it)->GetAnchorWeights();
            outfile << (*it)->Id() << " [" << AnchorWeights.size() << "] (";
            for(std::size_t i = 0; i < AnchorWeights.size() - 1; ++i)
                outfile << AnchorWeights[i] << ",";
            outfile << AnchorWeights.back() << ")\n";
        }
        outfile << "End ElementalData\n\n";

        // read extraction operator from element in the form of triplet CSR
        // this requires that the Id of the cell must be unique
        std::vector<int> rowPtr;
        std::vector<int> colInd;
        std::vector<double> values;
        std::map<int, std::vector<int> > rowPtrMap;
        std::map<int, std::vector<int> > colIndMap;
        std::map<int, std::vector<double> > valuesMap;
        for(cell_container_t::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
        {
            rowPtr.clear();
            colInd.clear();
            values.clear();
            (*it)->GetExtractionOperator(rowPtr, colInd, values);
            rowPtrMap[(*it)->Id()] = rowPtr;
            colIndMap[(*it)->Id()] = colInd;
            valuesMap[(*it)->Id()] = values;
        }

        // write extraction operator
        outfile << "Begin ElementalData EXTRACTION_OPERATOR_CSR_ROWPTR\n";
        for(std::map<int, std::vector<int> >::iterator it = rowPtrMap.begin(); it != rowPtrMap.end(); ++it)
        {
            outfile << (*it).first << " [" << (*it).second.size() << "] (";
            for(std::size_t i = 0; i < (*it).second.size() - 1; ++i)
                outfile << (*it).second[i] << ",";
            outfile << (*it).second.back() << ")\n";
        }
        outfile << "End ElementalData\n\n";

        outfile << "Begin ElementalData EXTRACTION_OPERATOR_CSR_COLIND\n";
        for(std::map<int, std::vector<int> >::iterator it = colIndMap.begin(); it != colIndMap.end(); ++it)
        {
            outfile << (*it).first << " [" << (*it).second.size() << "] (";
            for(std::size_t i = 0; i < (*it).second.size() - 1; ++i)
                outfile << (*it).second[i] << ",";
            outfile << (*it).second.back() << ")\n";
        }
        outfile << "End ElementalData\n\n";

        outfile << "Begin ElementalData EXTRACTION_OPERATOR_CSR_VALUES\n";
        for(std::map<int, std::vector<double> >::iterator it = valuesMap.begin(); it != valuesMap.end(); ++it)
        {
            outfile << (*it).first << " [" << (*it).second.size() << "] (";
            for(std::size_t i = 0; i < (*it).second.size() - 1; ++i)
                outfile << (*it).second[i] << ",";
            outfile << (*it).second.back() << ")\n";
        }
        outfile << "End ElementalData\n\n";

        // write the degree
        outfile << "Begin ElementalData NURBS_DEGREE_1\n";
        for(cell_container_t::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
            outfile << (*it)->Id() << " " << mOrder1 << std::endl;
        outfile << "End ElementalData\n\n";

        outfile << "Begin ElementalData NURBS_DEGREE_2\n";
        for(cell_container_t::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
            outfile << (*it)->Id() << " " << mOrder2 << std::endl;
        outfile << "End ElementalData\n\n";

        // write the division
        outfile << "Begin ElementalData NUM_DIVISION_1\n";
        for(cell_container_t::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
            outfile << (*it)->Id() << " " << Division1 << std::endl;
        outfile << "End ElementalData\n\n";

        outfile << "Begin ElementalData NUM_DIVISION_2\n";
        for(cell_container_t::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
            outfile << (*it)->Id() << " " << Division2 << std::endl;
        outfile << "End ElementalData\n\n";
        
        outfile.close();
        std::cout << "ExportMDPA completed" << std::endl;
    }
    
} // end namespace Kratos

