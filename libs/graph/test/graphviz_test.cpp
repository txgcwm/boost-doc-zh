// Copyright 2004-5 Trustees of Indiana University

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//
// graphviz_test.cpp - Test cases for the Boost.Spirit implementation of a
// Graphviz DOT Language reader.
//

// Author: Ronald Garcia

#define BOOST_GRAPHVIZ_USE_ISTREAM
#define BOOST_TEST_MODULE TestGraphviz
#include <boost/regex.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/assign/std/map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <algorithm>
#include <string>
#include <iostream>
#include <iterator>
#include <map>
#include <utility>

using namespace std;
using namespace boost;

using namespace boost::assign;

typedef std::string node_t;
typedef std::pair<node_t,node_t> edge_t;

typedef std::map<node_t,float> mass_map_t;
typedef std::map<edge_t,double> weight_map_t;

template <typename Directedness, typename OutEdgeList>
bool test_graph(std::istream& dotfile, std::size_t correct_num_vertices,
                mass_map_t const& masses,
                weight_map_t const& weights,
                std::string const& node_id = "node_id",
                std::string const& g_name = std::string()) {

  typedef property < vertex_name_t, std::string,
            property < vertex_color_t, float > > vertex_p;  
  typedef property < edge_weight_t, double > edge_p;
  typedef property < graph_name_t, std::string > graph_p;
  typedef adjacency_list < OutEdgeList, vecS, Directedness,
    vertex_p, edge_p, graph_p > graph_t;
  typedef typename graph_traits < graph_t >::edge_descriptor edge_t;
  typedef typename graph_traits < graph_t >::vertex_descriptor vertex_t;

  // Construct a graph and set up the dynamic_property_maps.
  graph_t graph(0);
  dynamic_properties dp(ignore_other_properties);
  typename property_map<graph_t, vertex_name_t>::type name =
    get(vertex_name, graph);
  dp.property(node_id,name);
  typename property_map<graph_t, vertex_color_t>::type mass =
    get(vertex_color, graph);
  dp.property("mass",mass);
  typename property_map<graph_t, edge_weight_t>::type weight =
    get(edge_weight, graph);
  dp.property("weight",weight);

  boost::ref_property_map<graph_t*,std::string> gname(
    get_property(graph,graph_name));
  dp.property("name",gname);

  bool result = true;
#ifdef BOOST_GRAPHVIZ_USE_ISTREAM
  if(read_graphviz(dotfile,graph,dp,node_id)) {
#else
  std::string data;
  dotfile >> std::noskipws;
  std::copy(std::istream_iterator<char>(dotfile),
            std::istream_iterator<char>(),
            std::back_inserter(data));
  if(read_graphviz(data.begin(),data.end(),graph,dp,node_id)) {
#endif
    // check correct vertex count
    BOOST_CHECK_EQUAL(num_vertices(graph), correct_num_vertices);
    // check masses
    if(!masses.empty()) {
      // assume that all the masses have been set
      // for each vertex:
      typename graph_traits<graph_t>::vertex_iterator i,j;
      for(boost::tie(i,j) = vertices(graph); i != j; ++i) {
        //  - get its name
        std::string node_name = get(name,*i);
        //  - get its mass
        float node_mass = get(mass,*i);
        float ref_mass = masses.find(node_name)->second;
        //  - compare the mass to the result in the table
        BOOST_CHECK_CLOSE(node_mass, ref_mass, 0.01f);
      }
    }
    // check weights
    if(!weights.empty()) {
      // assume that all weights have been set
      /// for each edge:
      typename graph_traits<graph_t>::edge_iterator i,j;
      for(boost::tie(i,j) = edges(graph); i != j; ++i) {
        //  - get its name
        std::pair<std::string,std::string>
          edge_name = make_pair(get(name, source(*i,graph)),
                                get(name, target(*i,graph)));
        // - get its weight
        double edge_weight = get(weight,*i);
        double ref_weight = weights.find(edge_name)->second;
        // - compare the weight to teh result in the table
        BOOST_CHECK_CLOSE(edge_weight, ref_weight, 0.01);
      }
    }
    if(!g_name.empty()) {
      std::string parsed_name = get_property(graph,graph_name);
      BOOST_CHECK(parsed_name == g_name);
    }


  } else {
    std::cerr << "Parsing Failed!\n";
    result = false;
  }

  return result;
  }

// int test_main(int, char*[]) {

  typedef istringstream gs_t;

  // Basic directed graph tests
  BOOST_AUTO_TEST_CASE (basic_directed_graph_1) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",7.7f) ("e", 6.66f);
    gs_t gs("digraph { a  node [mass = 7.7] c e [mass = 6.66] }");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,3,masses,weight_map_t())));
  }

  BOOST_AUTO_TEST_CASE (basic_directed_graph_2) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("e", 6.66f);
    gs_t gs("digraph { a  node [mass = 7.7] \"a\" e [mass = 6.66] }");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,2,masses,weight_map_t())));
  }

  BOOST_AUTO_TEST_CASE (basic_directed_graph_3) {
    weight_map_t weights;
    insert( weights )(make_pair("a","b"),0.0)
      (make_pair("c","d"),7.7)(make_pair("e","f"),6.66)
      (make_pair("d","e"),0.5)(make_pair("e","a"),0.5);
    gs_t gs("digraph { a -> b eDge [weight = 7.7] "
            "c -> d e-> f [weight = 6.66] "
            "d ->e->a [weight=.5]}");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,6,mass_map_t(),weights)));
  }

  // undirected graph with alternate node_id property name
  BOOST_AUTO_TEST_CASE (undirected_graph_alternate_node_id) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",7.7f) ("e", 6.66f);
    gs_t gs("graph { a  node [mass = 7.7] c e [mass = 6.66] }");
    BOOST_CHECK((test_graph<undirectedS,vecS>(gs,3,masses,weight_map_t(),
                                             "nodenames")));
  }

  // Basic undirected graph tests
  BOOST_AUTO_TEST_CASE (basic_undirected_graph_1) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",7.7f) ("e", 6.66f);
    gs_t gs("graph { a  node [mass = 7.7] c e [mass =\\\n6.66] }");
    BOOST_CHECK((test_graph<undirectedS,vecS>(gs,3,masses,weight_map_t())));
  }

  BOOST_AUTO_TEST_CASE (basic_undirected_graph_2) {
    weight_map_t weights;
    insert( weights )(make_pair("a","b"),0.0)
      (make_pair("c","d"),7.7)(make_pair("e","f"),6.66);
    gs_t gs("graph { a -- b eDge [weight = 7.7] "
            "c -- d e -- f [weight = 6.66] }");
    BOOST_CHECK((test_graph<undirectedS,vecS>(gs,6,mass_map_t(),weights)));
  }

  // Mismatch directed graph test
  BOOST_AUTO_TEST_CASE (mismatch_directed_graph) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",7.7f) ("e", 6.66f);
    gs_t gs("graph { a  nodE [mass = 7.7] c e [mass = 6.66] }");
    try {
      test_graph<directedS,vecS>(gs,3,masses,weight_map_t());
      BOOST_ERROR("Failed to throw boost::undirected_graph_error.");
    } catch (boost::undirected_graph_error&) {
    } catch (boost::directed_graph_error&) {
      BOOST_ERROR("Threw boost::directed_graph_error, should have thrown boost::undirected_graph_error.");
    }
  }

  // Mismatch undirected graph test
  BOOST_AUTO_TEST_CASE (mismatch_undirected_graph) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",7.7f) ("e", 6.66f);
    gs_t gs("digraph { a  node [mass = 7.7] c e [mass = 6.66] }");
    try {
      test_graph<undirectedS,vecS>(gs,3,masses,weight_map_t());
      BOOST_ERROR("Failed to throw boost::directed_graph_error.");
    } catch (boost::directed_graph_error&) {}
  }

  // Complain about parallel edges
  BOOST_AUTO_TEST_CASE (complain_about_parallel_edges) {
    BOOST_TEST_CHECKPOINT("Complain about parallel edges");
    weight_map_t weights;
    insert( weights )(make_pair("a","b"),7.7);
    gs_t gs("diGraph { a -> b [weight = 7.7]  a -> b [weight = 7.7] }");
    try {
      test_graph<directedS,setS>(gs,2,mass_map_t(),weights);
      BOOST_ERROR("Failed to throw boost::bad_parallel_edge.");
    } catch (boost::bad_parallel_edge&) {}
  }

  // Handle parallel edges gracefully
  BOOST_AUTO_TEST_CASE (handle_parallel_edges_gracefully) {
    weight_map_t weights;
    insert( weights )(make_pair("a","b"),7.7);
    gs_t gs("digraph { a -> b [weight = 7.7]  a -> b [weight = 7.7] }");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,2,mass_map_t(),weights)));
  }

  // Graph Property Test 1
  BOOST_AUTO_TEST_CASE (graph_property_test_1) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",0.0f) ("e", 6.66f);
    gs_t gs("digraph { graph [name=\"foo \\\"escaped\\\"\"]  a  c e [mass = 6.66] }");
    std::string graph_name("foo \"escaped\"");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,3,masses,weight_map_t(),"",
                                            graph_name)));
  }

  // Graph Property Test 2
  BOOST_AUTO_TEST_CASE (graph_property_test_2) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",0.0f) ("e", 6.66f);
    gs_t gs("digraph { name=\"fo\"+ \"\\\no\"  a  c e [mass = 6.66] }");
    std::string graph_name("foo");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,3,masses,weight_map_t(),"",
                                            graph_name)));
  }

  // Graph Property Test 3 (HTML)
  BOOST_AUTO_TEST_CASE (graph_property_test_3) {
    mass_map_t masses;
    insert ( masses )  ("a",0.0f) ("c",0.0f) ("e", 6.66f);
    std::string graph_name = "<html title=\"x'\" title2='y\"'>foo<b><![CDATA[><bad tag&>]]>bar</b>\n<br/>\nbaz</html>";
    gs_t gs("digraph { name=" + graph_name + "  a  c e [mass = 6.66] }");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,3,masses,weight_map_t(),"",
                                            graph_name)));
  }

  // Comments embedded in strings
  BOOST_AUTO_TEST_CASE (comments_embedded_in_strings) { 
    gs_t gs( 
      "digraph { "
      "a0 [ label = \"//depot/path/to/file_14#4\" ];"
      "a1 [ label = \"//depot/path/to/file_29#9\" ];"
      "a0 -> a1 [ color=gray ];"
      "}");
    BOOST_CHECK((test_graph<directedS,vecS>(gs,2,mass_map_t(),weight_map_t())));
  }
// return 0;
// }
