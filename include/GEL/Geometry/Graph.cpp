//
//  Graph.cpp
//  GEL
//
//  Created by J. Andreas Bærentzen on 09/12/2015.
//  Copyright © 2015 J. Andreas Bærentzen. All rights reserved.
//

#include <map>
#include <queue>
#include "Graph.h"

namespace Geometry {
    
    using namespace CGLA;
    using namespace std;
    
    void AMGraph::reassign_node_id(NodeID n_src, NodeID n_dst, bool merge) {
        
        // If we are not merging, clear the destination node. Otherwise remove
        // just the edge connecting dst to src.
        if(!merge)
            isolate_node(n_dst);
        else
            edge_map[n_dst].erase(n_src);
        
        // Remove edge connecting src to dst.
        edge_map[n_src].erase(n_dst);
        
        // For all edges in src
        for(auto& edge: edge_map[n_src]) {
            // Retrieve edge id of neighbor to src
            NodeID n = edge.first;
            
            // Remove edge going from n back to src
            edge_map[n].erase(n_src);
            
            // If we DO NOT find an edge from n to dst, we
            // create that edge giving it the old ID from the
            // edge connecting src to n.
            auto itr = edge_map[n_dst].find(n);
            if (itr == edge_map[n_dst].end()) {
                EdgeID e = edge.second;
                edge_map[n_dst][n] = e;
                edge_map[n][n_dst] = e;
            }
        }
        // Clear all edges going out from src which is now isolated.
        edge_map[n_src].clear();
    }

    
    /// Special ID value for invalid node
    const AMGraph3D::NodeID AMGraph::InvalidNodeID = std::numeric_limits<size_t>::max();
    
    /// Special ID value for invalid edge
    const AMGraph3D::EdgeID AMGraph::InvalidEdgeID = std::numeric_limits<size_t>::max();
    
    AMGraph3D clean_graph(const AMGraph3D& g, double thresh)
    {
        AMGraph3D gn; // new graph
        map<AMGraph::NodeID, AMGraph::NodeID> node_map;
        
        // For all nodes that are not too close to previously visited nodes
        // create a node in the new graph
        for(auto n: g.node_ids())
        {
            bool erased = false;
            if(std::isnan(g.pos[n][0]))
            {
                node_map[n] = AMGraph::InvalidNodeID;
                erased = true;
            }
            else
                for(auto m: Util::Range(0,n))
                {
                    double d = g.sqr_dist(n,m);
                    if(d<thresh) {
                        erased = true;
                        node_map[n] = node_map[m];
                    }
                }
            if(!erased) {
                node_map[n] = gn.add_node(g.pos[n]);
                gn.node_color[node_map[n]] = g.node_color[n];
            }
        }
        
        // For all edges in old graph, create a new edge
        for(auto n: g.node_ids())
            if(node_map[n] != AMGraph::InvalidNodeID)
                for(AMGraph::NodeID& nn: g.neighbors(n)) {
                    AMGraph::EdgeID e = gn.connect_nodes(node_map[n], node_map[nn]);
                    if(gn.valid_edge(e)) {
                        AMGraph::EdgeID e_old = g.find_edge(n, nn);
                        if(g.valid_edge(e_old))
                            gn.edge_color[e] = g.edge_color[e_old];
                        else
                            gn.edge_color[e] = Vec3f(0);
                    }
                }
        
        return gn;
    }
    
    
    void BreadthFirstSearch::init(AMGraph::NodeID n) {
        if(pq.empty()) {
            pq = priority_queue<PrimPQElem>();
            pq.push(PrimPQElem(-0.0, n, AMGraph::InvalidNodeID));

            dist = Util::AttribVec<AMGraph::NodeID, double> (g_ptr->no_nodes(), DBL_MAX);
            dist[n] = 0.0;

            visited.clear();
            front.clear();
        }
    }
    
    bool BreadthFirstSearch::step() {
        bool did_visit = false;
        while(!pq.empty() && !did_visit) {
            last = pq.top();
            auto n = last.node;
            front.erase(n);
            pq.pop();
            if(last.priority == -dist[n]) {
                visited.insert(n);
                did_visit = true;
                for(auto m: g_ptr->neighbors(n)) {
                    double d = sqrt(g_ptr->sqr_dist(n,m)) - last.priority;
                    if(d < dist[m]) {
                        dist[m] = d;
                        pred[m] = n;
                        pq.push(PrimPQElem(-d, m, n));
                        front.insert(m);
                    }
                }
            }
        }
        return did_visit;
    }

    
    AMGraph3D minimum_spanning_tree(const AMGraph3D& g, AMGraph::NodeID root)
    {
        if(root == AMGraph::InvalidNodeID)
            root = 0;
        
        AMGraph3D gn;
        for(auto n: g.node_ids())
            gn.add_node(g.pos[n]);

        BreadthFirstSearch bfs(g);
        bfs.init(root);
        while(bfs.step()) {
            auto last = bfs.get_last();
            gn.connect_nodes(last.node, last.parent);
        }
        
        return gn;
    }
    
    
    
}
