/**
   C++ implementation of the RS algorithm (library)
   Copyright Vera-Licona Research Group (C) 2015
   Author: Andrew Gainer-Dewar, Ph.D. <andrew.gainer.dewar@gmail.com>
**/

#include "rs.hpp"

#include "hypergraph.hpp"
#include "shd-base.hpp"

#include <cassert>
#include <omp.h>

#include <boost/dynamic_bitset.hpp>

#define BOOST_LOG_DYN_LINK 1 // Fix an issue with dynamic library loading
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/dynamic_bitset.hpp>

// TODO: Input specifications with <cassert>
namespace agdmhs {
    static bsqueue HittingSets;

    static bool rs_any_edge_critical_after_i(const hindex& i,
                                   const bitset& S,
                                   const Hypergraph& crit) {
        /*
          Return true if any vertex in S has its first critical edge
          after i.
         */
        bool bad_edge_found = false;
        hindex w = S.find_first();
        while (w != bitset::npos and not bad_edge_found) {
            hindex first_crit_edge = crit[w].find_first();
            if (first_crit_edge >= i) {
                bad_edge_found = true;
            }
            w = S.find_next(w);
        }

        return bad_edge_found;
    };

    static void rs_extend_or_confirm_set(const Hypergraph& H,
                                         const Hypergraph& T,
                                         const bitset& S,
                                         const bitset& CAND,
                                         const Hypergraph& crit,
                                         const bitset& uncov,
                                         const size_t cutoff_size) {
        // Input specification
        assert(uncov.any()); // uncov cannot be empty
        assert(CAND.any()); // CAND cannot be empty
        assert(cutoff_size == 0 or S.count() < cutoff_size); // If we're using a cutoff, S must not be too large

        // Otherwise, get an uncovered edge
        hindex search_edge = uncov.find_first(); // Per M+U, we use the first edge (to ensure proper search order)
        bitset e = H[search_edge];
        while (search_edge != bitset::npos) {
            if ((H[search_edge] & CAND).count() < (e & CAND).count()) {
                e = H[search_edge];
            }
            search_edge = uncov.find_next(search_edge);
        }

        // Then consider vertices lying in the intersection of e with CAND
        bitset C = CAND & e; // intersection
        bitset newCAND = CAND & (~e); // difference

        // Store the indices in C in descending order for iteration
        std::deque<hindex> Cindices;
        hindex v = C.find_first();
        while (v != bitset::npos) {
            Cindices.push_front(v);
            v = C.find_next(v);
        }

        // Test all the vertices in C (in descending order)
        for (auto& v: Cindices) {
            // Check preconditions
            Hypergraph new_crit = crit;
            bitset new_uncov = uncov;
            try {
                update_crit_and_uncov(new_crit, new_uncov, H, T, S, v);
            }
            catch (vertex_violating_exception& e) {
                newCAND.set(v);
                continue;
            }

            if (rs_any_edge_critical_after_i(search_edge, S, new_crit)) {
                newCAND.set(v);
                continue;
            }

            // if new_uncov is empty, adding v to S makes a hitting set
            bitset newS = S;
            newS.set(v);

            if (new_uncov.none()) {
                HittingSets.enqueue(newS);
            } else if (cutoff_size == 0 or newS.count() < cutoff_size) {
            // After this point, we'll be considering extending newS even more.
            // If we're using a cutoff, this requires more room.
#pragma omp task untied shared(H, T)
                rs_extend_or_confirm_set(H, T, newS, newCAND, new_crit, new_uncov, cutoff_size);
            }

            // Update newCAND and proceed to new vertex
            newCAND.set(v);
        }
    };

    Hypergraph rs_transversal(const Hypergraph& H,
                              const size_t num_threads,
                              const size_t cutoff_size) {
        // SET UP INTERNAL VARIABLES
        // Number of threads for parallelization
        omp_set_num_threads(num_threads);

        // Candidate hitting set
        bitset S (H.num_verts());
        S.reset(); // Initially empty

        // Eligible vertices
        bitset CAND (H.num_verts());
        CAND.set(); // Initially full

        // Which edges each vertex is critical for
        Hypergraph crit (H.num_edges(), H.num_verts());

        // Which edges are uncovered
        bitset uncov (H.num_edges());
        uncov.set(); // Initially full

        // Tranpose of H
        Hypergraph T = H.transpose();

        // RUN ALGORITHM
        {
#pragma omp parallel shared(H, T)
#pragma omp single
            rs_extend_or_confirm_set(H, T, S, CAND, crit, uncov, cutoff_size);
#pragma omp taskwait
        }

        // Gather results
        Hypergraph Htrans(H.num_verts());
        bitset result;
        while (HittingSets.try_dequeue(result)) {
            Htrans.add_edge(result);
        }

        return Htrans;
    };
}
