/** \file
 * \brief Tests for fileformat reading and writing using GraphIO,
 *   only graphs without attributes
 *
 * \author Stephan Beyer, Tilo Wiedera
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/

#include <algorithm>
#include <string>
#include <regex>
#include <tuple>
#include <unordered_map>
#include <bandit/bandit.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/EpsilonTest.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>
#include <resources.h>

using namespace ogdf;
using namespace bandit;

typedef bool (*ReaderFunc)(Graph&, istream&);
typedef bool (*WriterFunc)(const Graph&, ostream&);

bool seemsEqual(const Graph &G1, const Graph &G2)
{
	bool result = G1.numberOfNodes() == G2.numberOfNodes();
	result &= G1.numberOfEdges() == G2.numberOfEdges();

	if (result) {
		std::unordered_map<int, int> counters(0);

		for (node v : G1.nodes) {
			counters[v->degree()]++;
		}

		for (node v : G2.nodes) {
			counters[v->degree()]--;
		}

		for (auto value : counters) {
			result &= value.second == 0;
		}
	}

	return result;
}

template<typename T>
void describeSTP(const string &typeName) {
	describe(string("STP for " + typeName), [](){
		for(int i = 4; i < 1024; i *= 2) {
			it(string("stores and loads an instance of size " + to_string(i)), [&](){
				std::ostringstream writeStream;

				EdgeWeightedGraph<T> graph;
				List<node> terminals;
				NodeArray<bool> isTerminal(graph, false);

				randomGraph(graph, i, (i*(i-1))/2);
				for(node v : graph.nodes) {
					if(randomDouble(0, 1) > .5) {
						terminals.pushBack(v);
						isTerminal(v) = true;
					}
				}
				for (edge e : graph.edges) {
					graph.setWeight(e, (T) randomDouble(0, 1000));
				}

				string myComment = "";
				if (randomDouble(0, 1) > .5) {
					myComment += "Name \"MyRandomInstance\"\n";
					myComment += "Creator \"Tilo Wiedera\"\n";
				}
				GraphIO::writeSTP(graph, terminals, writeStream, myComment);

				EdgeWeightedGraph<T> readGraph;
				List<node> readTerminals;
				NodeArray<bool> readIsTerminal;

				std::istringstream readStream(writeStream.str());
				AssertThat(GraphIO::readSTP(readGraph, readTerminals, readIsTerminal, readStream), Equals(true));

				AssertThat(readGraph.numberOfNodes(), Equals(graph.numberOfNodes()));
				AssertThat(readGraph.numberOfEdges(), Equals(graph.numberOfEdges()));
				AssertThat(readTerminals.size(), Equals(terminals.size()));
				for(node v : readGraph.nodes) {
					AssertThat(readIsTerminal[v], Equals(readTerminals.search(v).valid()));
				}
			});
		}

		for_each_file("fileformats/stp/valid", [&](const string &filename){
			it(string("successfully parses " + filename), [&](){
				EdgeWeightedGraph<T> graph;
				List<node> terminals;
				NodeArray<bool> isTerminal;
				AssertThat(GraphIO::readSTP(graph, terminals, isTerminal, filename), IsTrue());

				AssertThat(graph.numberOfNodes(), IsGreaterThan(0));
				AssertThat(graph.numberOfEdges(), IsGreaterThan(0));
				AssertThat(terminals.size(), IsGreaterThan(0));

				int terminalCounter = 0;
				for(node v : graph.nodes) {
					terminalCounter += isTerminal[v];
				}

				AssertThat(terminalCounter, Equals(terminals.size()));
			});
		});

		for_each_file("fileformats/stp/invalid", [&](const string &filename){
			it(string("detects errors in " + filename), [&](){
				EdgeWeightedGraph<T> graph;
				List<node> terminals;
				NodeArray<bool> isTerminal;
				AssertThat(GraphIO::readSTP(graph, terminals, isTerminal, filename), IsFalse());
			});
		});
	});
}

template<typename T>
void describeDMF(const string &typeName) {
	describe(string("DMF for " + typeName), [](){
		const void* nullPointer = nullptr;

		for_each_file("fileformats/dmf/valid", [&](const string &filename){
			it(string("reads " + filename), [&](){
				EdgeWeightedGraph<T> graph;
				node source;
				node sink;

				AssertThat(GraphIO::readDMF(graph, source, sink, filename), IsTrue());
				AssertThat(graph.numberOfNodes(), IsGreaterThan(1));
				AssertThat(source, Is().Not().EqualTo(nullPointer));
				AssertThat(sink, Is().Not().EqualTo(nullPointer));
#ifdef OGDF_DEBUG
				AssertThat(source->graphOf(), Equals(&graph));
				AssertThat(sink->graphOf(), Equals(&graph));
#endif
				AssertThat(source, Is().Not().EqualTo(sink));

				for(edge e : graph.edges) {
					AssertThat(graph.weight(e) > 0, IsTrue());
				}
			});
		});

		for_each_file("fileformats/dmf/invalid", [&](const string &filename){
			it(string("reads " + filename), [&](){
				EdgeWeightedGraph<T> graph;
				node source;
				node sink;
				AssertThat(GraphIO::readDMF(graph, source, sink, filename), IsFalse());
			});
		});

		it("writes and reads a random graph", [&](){
			EdgeWeightedGraph<T> graph;
			node source;
			node sink;

			randomGraph(graph, 42, 189);
			sink = source = graph.chooseNode();
			while(sink == source) {
				sink = graph.chooseNode();
			}

			T sum = 0;
			for(edge e : graph.edges) {
				T cap = static_cast<T>(randomDoubleNormal(10, 5));
				if(cap < 0) {
					cap *= -1;
				}
				graph.setWeight(e, cap);
				sum += cap;
			}

			std::ostringstream writeStream;

			AssertThat(GraphIO::writeDMF(graph, source, sink, writeStream), IsTrue());

			EdgeWeightedGraph<T> readGraph;
			node readSource = nullptr;
			node readSink = nullptr;

			std::istringstream readStream(writeStream.str());
			AssertThat(GraphIO::readDMF(readGraph, readSource, readSink, readStream), IsTrue());

			AssertThat(readGraph.numberOfNodes(), Equals(graph.numberOfNodes()));
			AssertThat(readGraph.numberOfEdges(), Equals(graph.numberOfEdges()));
			AssertThat(readSource, Is().Not().EqualTo(nullPointer));
			AssertThat(readSink, Is().Not().EqualTo(nullPointer));
#ifdef OGDF_DEBUG
				AssertThat(readSource->graphOf(), Equals(&readGraph));
				AssertThat(readSink->graphOf(), Equals(&readGraph));
#endif
			AssertThat(readSource->degree(), Equals(source->degree()));
			AssertThat(readSink->degree(), Equals(sink->degree()));

			T readSum = 0;
			for(edge e : readGraph.edges) {
				readSum += readGraph.weight(e);
			}

			EpsilonTest eps(1.0e-3);
			AssertThat(eps.equal(sum, readSum), IsTrue());
		});
	});
}

/**
 * Used to describe a format parser and writer.
 *
 * \param name The name of the format.
 * \param reader The parse function to be tested.
 * \param writer The write function to be tested.
 * \param isXml Whether the format is based on XML.
 */
void describeFormat(const std::string name, ReaderFunc reader, WriterFunc writer, bool isXml)
{
	std::string lowerCaseName = name;
	std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);

	auto errorTest = [&](const string &filename){
		it(string("detects errors in " + filename), [&](){
			Graph graph;
			std::ifstream input(filename);
			AssertThat(reader(graph, input), IsFalse());
		});
	};

	auto resourceBasedTest = [&](){
		for_each_file("fileformats/" + lowerCaseName + "/valid", [&](const string &filename){
			it(string("successfully parses " + filename), [&](){
				Graph graph;
				std::ifstream input(filename);
				AssertThat(reader(graph, input), IsTrue());
				AssertThat(graph.numberOfNodes(), IsGreaterThan(0));
				AssertThat(graph.numberOfEdges(), IsGreaterThan(0));
			});
		});
		for_each_file("fileformats/" + lowerCaseName + "/valid/skip", [&](const string &filename) {
			it_skip(string("successfully parses " + filename), [&]() {});
		});

		for_each_file("fileformats/" + lowerCaseName + "/invalid", errorTest);
		for_each_file("fileformats/" + lowerCaseName + "/invalid/skip", [&](const string &filename) {
			it_skip(string("detects errors in " + filename), [&]() {});
		});

		it("returns false if the files does not exist", [&](){
			Graph graph;
			std::ifstream input;
			input.close();
			AssertThat(reader(graph, input), IsFalse());
		});
	};

	describe(name, [&](){
		if(isXml) {
			for_each_file("fileformats/xml/invalid", errorTest);
		}

		resourceBasedTest();

		it("writes and reads an empty graph", [&](){
			Graph G, Gtest;
			std::ostringstream write;
			AssertThat(writer(G, write), Equals(true));
			std::istringstream read(write.str());
			AssertThat(reader(Gtest, read), Equals(true));
			AssertThat(seemsEqual(G, Gtest), Equals(true));
		});

		it("writes and reads a graph of isolated nodes", [&](){
			Graph G, Gtest;
			G.newNode(); G.newNode();
			std::ostringstream write;
			AssertThat(writer(G, write), Equals(true));
			std::istringstream read(write.str());
			AssertThat(reader(Gtest, read), Equals(true));
			AssertThat(seemsEqual(G, Gtest), Equals(true));
		});

		it("writes and reads a Petersen graph", [&](){
			Graph G, Gtest;
			petersenGraph(G, 5, 2);
			std::ostringstream write;
			AssertThat(writer(G, write), Equals(true));
			std::istringstream read(write.str());
			AssertThat(reader(Gtest, read), Equals(true));
			AssertThat(seemsEqual(G, Gtest), Equals(true));
		});

		it("writes and reads a big complete graph", [&](){
			Graph G, Gtest;
			completeGraph(G, 243);
			std::ostringstream write;
			AssertThat(writer(G, write), Equals(true));
			std::istringstream read(write.str());
			AssertThat(reader(Gtest, read), Equals(true));
			AssertThat(seemsEqual(G, Gtest), Equals(true));
		});
	});
}

go_bandit([](){
describe("GraphIO", [](){
	describeSTP<int>("int");
	describeSTP<double>("double");

	describeDMF<int>("int");
	describeDMF<double>("double");

	describeFormat("GML", GraphIO::readGML, GraphIO::writeGML, false);
	describeFormat("OGML", GraphIO::readOGML, GraphIO::writeOGML, true);
	describeFormat("Rome", GraphIO::readRome, GraphIO::writeRome, false);
	describeFormat("LEDA", GraphIO::readLEDA, GraphIO::writeLEDA, false);
	describeFormat("Chaco", GraphIO::readChaco, GraphIO::writeChaco, false);
	describeFormat("PMDiss", GraphIO::readPMDissGraph, GraphIO::writePMDissGraph, false);
	describeFormat("GraphML", GraphIO::readGraphML, GraphIO::writeGraphML, true);
	describeFormat("DOT", GraphIO::readDOT, GraphIO::writeDOT, false);
	describeFormat("GEXF", GraphIO::readGEXF, GraphIO::writeGEXF, true);
	describeFormat("GDF", GraphIO::readGDF, GraphIO::writeGDF, false);
	describeFormat("TLP", GraphIO::readTLP, GraphIO::writeTLP, false);
	describeFormat("DL", GraphIO::readDL, GraphIO::writeDL, false);
	describeFormat("Graph6", GraphIO::readGraph6, GraphIO::writeGraph6, false);

	describe("generic reader", []() {
		std::function<void (const string&)> genericTest = [](const string &filename) {
			it(string("parses " + filename), [&]() {
				Graph graph;
				AssertThat(GraphIO::read(graph, filename), IsTrue());
			});
		};

		for_each_file("fileformats/gml/valid", genericTest);
		for_each_file("fileformats/chaco/valid", genericTest);
		for_each_file("fileformats/dl/valid", genericTest);
		for_each_file("fileformats/dot/valid", genericTest);
		for_each_file("fileformats/gdf/valid", genericTest);
		for_each_file("fileformats/gexf/valid", genericTest);
		for_each_file("fileformats/graphml/valid", genericTest);
		for_each_file("fileformats/leda/valid", genericTest);
		for_each_file("fileformats/ogml/valid", genericTest);
		for_each_file("fileformats/tlp/valid", genericTest);
	});
});
});
