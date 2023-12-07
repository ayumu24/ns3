/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This is an example script for AODV manet routing protocol. 
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include <iostream>
#include <cmath>
#include "ns3/ndaodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;

class NDAODV
{
public:
 NDAODV ();

 bool Configure(int argc, char **argv);

 void Run ();
 void Report (std::ostream & os);

private:
 uint32_t size; //ノード数
 double step; //ノード間距離
 double totaltime; //シミュレーション時間
 bool pcap; //pcapのオン・オフ
 bool printroutes; //printrouteのオン・オフ
 bool grid; //グリッドトポロジーのオン・オフ
 bool random; //ランダムモビリティのオン・オフ

 NodeContainer nodes;
 NetDeviceContainer devices;
 Ipv4InterfaceContainer interfaces; 
 
private:
 void CreateNodes (); //ノードの作成
 void CreateDevices (); //デバイスの作成
 void InstallInternetStack (); //デバイスにスタックをインストール
 void InstallApplications (); //アプリケーションをインストール
};

int main (int argc, char **argv)
{
    NDAODV ndaodv; //オブジェクト生成
    //Configureエラーのログ
    if(!ndaodv.Configure (argc, argv))
        NS_FATAL_ERROR("Configuration failed. Aborted.");
    
    ndaodv.Run ();
    ndaodv.Report(std::cout);
    return 0;
}

NDAODV::NDAODV () :
//値の初期設定
    size (10),
    step (100),
    totaltime (100),
    pcap (true),
    printroutes (true),
    grid (true),
    random (false)
{
};

bool
NDAODV::Configure (int argc, char **argv)
{
    //コマンドラインから値の受け取り
    SeedManager::SetSeed (12345);
    CommandLine cmd;

    cmd.AddValue ("pcap", "Write PCAP trace.", pcap);
    cmd.AddValue ("printroutes", "Print routing table dump.", printroutes);
    cmd.AddValue ("size", "Number of nodes.", size);
    cmd.AddValue ("time", "Simulation time, s.", totaltime);
    cmd.AddValue ("step", "Grid step, m", step);
    cmd.AddValue ("grid", "Grid mobility", grid);
    cmd.AddValue ("random", "Random mobility", random);

    cmd.Parse (argc, argv);
    return true;
}

void
NDAODV::Run ()
{
    CreateNodes ();
    CreateDevices ();
    InstallInternetStack ();
    InstallApplications ();

    std::cout << "Starting simulation for " << totaltime << " s ...\n";

    AnimationInterface anim ("ndaodv-animation.xml");

    Simulator::Stop (Seconds (totaltime));
    Simulator::Run ();
    Simulator::Destroy ();
}


void
NDAODV::Report (std::ostream &)
{

}

void
NDAODV::CreateNodes ()
{
    //ノードの生成
    nodes.Create (size);

    //ノードの名前付け
    for (uint32_t i = 0; i < size; ++i)
    {
        std::ostringstream os;
        os << "node-" << i;
        Names::Add (os.str (), nodes.Get (i));
    }

    //モビリティの設定
    MobilityHelper mobility;

    if(random)
    {
        grid = false;

        ObjectFactory pos;
        pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
        pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
        pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
        Ptr<PositionAllocator> posalloc = pos.Create()->GetObject<PositionAllocator>();
        mobility.SetPositionAllocator (posalloc);
        mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                    "Speed", StringValue ("ns3::UniformRandomVariable[Min=0|Max=60]"),
                                    "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"),
                                    "PositionAllocator", PointerValue(posalloc));
        mobility.Install (nodes);

    } 

    if (grid)
    {
        mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                        "MinX", DoubleValue (0.0),
                                        "MinY", DoubleValue (0.0),
                                        "DeltaX", DoubleValue (step),
                                        "DeltaY", DoubleValue (step),
                                        "GridWidth", UintegerValue (size/10),
                                        "LayoutType", StringValue ("RowFirst"));
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (nodes);
    }
}

void
NDAODV::CreateDevices ()
{
    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac");
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
    devices = wifi.Install (wifiPhy, wifiMac, nodes);
}

void
NDAODV::InstallInternetStack ()
{
    NDAodvHelper ndaodv;
    InternetStackHelper stack;
    stack.SetRoutingHelper (ndaodv);
    stack.Install (nodes);
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.0.0.0");
    interfaces = address.Assign (devices);

    if (printroutes)
    {
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("ndaodv.routes", std::ios::out);
        ndaodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
NDAODV::InstallApplications ()
{
    V4PingHelper ping (interfaces.GetAddress (size - 1));
    ping.SetAttribute ("Verbose", BooleanValue (true));

    ApplicationContainer p =ping.Install (nodes.Get (0));
    p.Start (Seconds (0));
    p.Stop (Seconds (totaltime) - Seconds (0.001));

    Ptr<Node> node = nodes.Get (size/2);
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
    Simulator::Schedule (Seconds (totaltime/3), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));
}