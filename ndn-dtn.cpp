/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple-with-link-failure.cpp
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp" // for LinkStatusControl::FailLinks and LinkStatusControl::UpLinks

// Added
#include "ns3/ndnSIM/apps/ndn-consumer-window.hpp"
#include "ns3/attribute-accessor-helper.h"


#define STOP_TIME 10.0 // seconds
#define FAIL_CHANCE 10 // Link failure percentage

namespace ns3 {
/*
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple-with-link-failure
 */

/*static void WindowChange(uint32_t oldCwnd, uint32_t newCwnd) {
 NS_LOG_UNCOND(Simulator::Now ().GetSeconds () << "\t" << newCwnd);
 }*/


// ==============================
// 		TRACE FUNCTION START
// ==============================
//
//void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd){
//	 *stream->GetStream () << Simulator::Now ().GetSeconds () << " " <<  newCwnd << std::endl;
//}
//
//static void TraceCwnd (){// Trace changes to the congestion window
//  AsciiTraceHelper ascii;
//  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("ndn-dtn.cwnd");
//  Config::ConnectWithoutContext ("/NodeList/*/$ns3::ndn::ConsumerWindow/m_window", MakeBoundCallback (&CwndChange,stream));
//}


// ==============================
// 		TRACE FUNCTION END
// ==============================

// ==============================
// 		TOPOLOGY FUNCTIONS START
// ==============================
void ScheduleLinkFailure(std::list<TopologyReader::Link> linksList){
	// define 1-100%
	double min = 1.0;
	double max = 100.0;

	// init RV
	Ptr<UniformRandomVariable> rv = CreateObject<UniformRandomVariable> ();
	rv->SetAttribute ("Min", DoubleValue (min));
	rv->SetAttribute ("Max", DoubleValue (max));

	// for all seconds and all links schedule failure
	for(int time_i = 0; time_i < max; time_i++){
		for (std::list<TopologyReader::Link>::const_iterator link = linksList.begin();
				link != linksList.end(); link++){
			if(rv->GetInteger() <= FAIL_CHANCE){
			    Ptr<Node> fromNode = link->GetFromNode();
				Ptr<Node> toNode = link->GetToNode();
				Simulator::Schedule(Seconds(time_i), ndn::LinkControlHelper::FailLink, fromNode, toNode);
				Simulator::Schedule(Seconds(time_i+1), ndn::LinkControlHelper::UpLink, fromNode, toNode);
			}
		}
	}
}

void InstallTopology() {
	std::list<TopologyReader::Link> linksList;
	AnnotatedTopologyReader topologyReader("", 10);
	topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-6-node.txt");
	topologyReader.Read();
	//ScheduleLinkFailure(topologyReader.GetLinks());
}
// ==============================
// 		TOPOLOGY FUNCTIONS END
// ==============================

// ==============================
// 			MAIN
// ==============================
int main(int argc, char* argv[]) {
	CommandLine cmd;
	cmd.Parse(argc, argv);

	InstallTopology();


	// ==============================
	// 		NETWORK START
	// ==============================
	// Install NDN stack on all nodes
	ndn::StackHelper ndnHelper;

	// Set Broadcast strategy
	ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/broadcast");

	// Installing global routing interface on all nodes
	ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll();

	// Getting containers for the consumer/producer
	NodeContainer producerNodes;
	NodeContainer consumerNodes;
	producerNodes.Add(Names::Find<Node>("Src1"));
	producerNodes.Add(Names::Find<Node>("Src2"));
	consumerNodes.Add(Names::Find<Node>("Dst1"));

	std::string prefix = "/prefix";
	// Add /prefix origins to ndn::GlobalRouter
	ndnGlobalRoutingHelper.AddOrigins(prefix, producerNodes);
	// Calculate and install FIBs
	ndn::GlobalRoutingHelper::CalculateRoutes();

	// ==============================
	// 		NETWORK END
	// ==============================


	// ==============================
	// 		APP START
	// ==============================
	//ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
	//consumerHelper.SetAttribute("Frequency", StringValue("100")); // 100 interests a second

	ndn::AppHelper consumerHelper("ns3::ndn::ConsumerWindow");
	consumerHelper.SetAttribute("Window", StringValue("1"));
	consumerHelper.SetAttribute("PayloadSize", StringValue("1024"));
	consumerHelper.SetAttribute("Size", StringValue("1"));

	consumerHelper.SetPrefix(prefix);
	consumerHelper.Install(consumerNodes);

	ndn::AppHelper producerHelper("ns3::ndn::Producer");
	producerHelper.SetPrefix(prefix);
	producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
	producerHelper.Install(producerNodes);
	// ==============================
	// 		APP END
	// ==============================


	// ==============================
	// 		TRACE START
	// ==============================
	ndn::L3RateTracer::InstallAll("src/ndnSIM/examples/dtn-trace/dtn-rate-trace.txt", Seconds(0.5));


	//ns3::AsciiTraceHelperForDevice::
	//PointToPointHelper R1R2;
	//R1R2.EnableAsciiAll(ascii.CreateFileStream("ndn-dtn-r1r2.tr"));
	//ascii.("/", consumerNodes);
	//Simulator::Schedule(Seconds(0.01),&TraceCwnd);

	// ==============================
	// 		TRACE END
	// ==============================


	// ==============================
	// 		SIM START
	// ==============================
	Simulator::Stop(Seconds(STOP_TIME));
	Simulator::Run();
	Simulator::Destroy();
	// ==============================
	// 		SIM END
	// ==============================

	return 0;
}

} // namespace ns3

int main(int argc, char* argv[]) {
	return ns3::main(argc, argv);
}
