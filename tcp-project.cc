/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * DTA400 simulation project: TCP NewReno settings in three network scenarios.
 *
 * Based on examples/tutorial/fifth.cc (Lab 2). The way the TCP parameters and
 * the BulkSend application are set up follows examples/tcp/tcp-linux-reno.cc.
 *
 * Topology (same as fifth.cc):
 *
 *     node 0  ----------------------  node 1
 *     sender      point-to-point       receiver
 *              (dataRate, delay, loss)
 *
 * The sender transfers a fixed amount of data (default 1 MB) with TCP NewReno.
 * The program prints ONE line with the settings and the transfer time.
 *
 * Example:
 *   ./ns3 run "tcp-project --dataRate=63Mbps --delay=180ms --errorRate=0.00015
 *              --initCwnd=10 --segmentSize=1448 --delAckCount=1 --RngRun=1"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpProject");

// ---------------------------------------------------------------------------
// Global variables used by the callback functions below
// ---------------------------------------------------------------------------
static uint64_t g_fileSize = 1000000;   // bytes to transfer
static uint64_t g_rxBytes = 0;          // bytes received so far
static double g_startTime = 1.0;        // when the sender starts (s)
static double g_finishTime = -1.0;      // when the last byte arrived (s), -1 = not finished
static std::ofstream g_cwndFile;        // optional cwnd trace file

// Called every time the congestion window changes (same idea as fifth.cc).
// Writes "time  cwnd-in-bytes" to the cwnd file, if one was requested.
static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    if (g_cwndFile.is_open())
    {
        g_cwndFile << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
    }
}

// NEW: called every time the receiver gets a packet.
// When all bytes of the file have arrived, we note the time and stop.
static void
RxCallback(Ptr<const Packet> packet, const Address& from)
{
    g_rxBytes += packet->GetSize();
    if (g_finishTime < 0 && g_rxBytes >= g_fileSize)
    {
        g_finishTime = Simulator::Now().GetSeconds();
        Simulator::Stop(); // nothing more to measure
    }
}

// NEW: connect the cwnd trace. The BulkSend application creates its socket
// only when it starts, so we connect just after the start (as in tcp-linux-reno.cc).
static void
ConnectCwndTrace()
{
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                  MakeCallback(&CwndChange));
}

int
main(int argc, char* argv[])
{
    // -----------------------------------------------------------------------
    // CHANGE 1: command-line options.
    // Scenario = dataRate + delay + errorRate.  TCP settings = the 3 parameters.
    // The default values of the 3 parameters are the ns-3 defaults
    // (src/internet/model/tcp-socket.cc).
    // -----------------------------------------------------------------------
    std::string dataRate = "5Mbps"; // link speed
    std::string delay = "2ms";      // ONE-WAY delay (round-trip time = 2 x delay)
    double errorRate = 0.0;         // probability that a PACKET is lost
    uint32_t initCwnd = 10;         // InitialCwnd (segments)
    uint32_t segmentSize = 536;     // SegmentSize (bytes)
    uint32_t delAckCount = 2;       // DelAckCount (packets)
    double maxTime = 600.0;         // safety limit for one run (s)
    std::string cwndFileName = "";  // empty = do not write a cwnd trace
    bool header = false;            // print the column names first

    CommandLine cmd(__FILE__);
    cmd.AddValue("dataRate", "Link speed, e.g. 100Mbps", dataRate);
    cmd.AddValue("delay", "One-way link delay, e.g. 150ms", delay);
    cmd.AddValue("errorRate", "Packet loss probability on the link (0 = no random loss)", errorRate);
    cmd.AddValue("initCwnd", "TcpSocket::InitialCwnd (segments)", initCwnd);
    cmd.AddValue("segmentSize", "TcpSocket::SegmentSize (bytes)", segmentSize);
    cmd.AddValue("delAckCount", "TcpSocket::DelAckCount (packets)", delAckCount);
    cmd.AddValue("fileSize", "Bytes to transfer", g_fileSize);
    cmd.AddValue("maxTime", "Stop the run after this many seconds", maxTime);
    cmd.AddValue("cwndFile", "Write the cwnd trace to this file", cwndFileName);
    cmd.AddValue("header", "Print the column names before the result line", header);
    cmd.Parse(argc, argv);

    // -----------------------------------------------------------------------
    // TCP settings.  The first and third lines are the same as in fifth.cc.
    // -----------------------------------------------------------------------
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    Config::SetDefault("ns3::TcpL4Protocol::RecoveryType",
                       TypeIdValue(TypeId::LookupByName("ns3::TcpClassicRecovery")));

    // CHANGE 2: the three parameters come from the command line.
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(initCwnd));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segmentSize));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));

    // CHANGE 3: larger send and receive buffers (4 MB), the same for every run.
    // With the default 128 KB buffer the receiver window, not the three
    // parameters, would limit the long-distance scenario.
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(4 << 20));

    // -----------------------------------------------------------------------
    // Network: two nodes and one point-to-point link (as in fifth.cc).
    // -----------------------------------------------------------------------
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate)); // CHANGE 4
    pointToPoint.SetChannelAttribute("Delay", StringValue(delay));      // CHANGE 4

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Random loss on packets arriving at the receiver (as in fifth.cc).
    // CHANGE 5: the rate comes from the command line and the unit is PACKET,
    // so errorRate is a packet loss probability, the same measure that
    // measurement studies report. Every packet has the same loss probability,
    // whatever its size.
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(errorRate));
    em->SetAttribute("ErrorUnit", EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // -----------------------------------------------------------------------
    // Applications
    // -----------------------------------------------------------------------
    // Receiver: PacketSink, as in fifth.cc.
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(interfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.));
    // CHANGE 6: count received bytes, to know when the file is complete.
    sinkApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&RxCallback));

    // CHANGE 7: sender = BulkSendApplication (as in tcp-linux-reno.cc) instead
    // of fifth.cc's TutorialApp. It sends exactly fileSize bytes as fast as TCP
    // allows, and never loses data when the send buffer is full.
    BulkSendHelper source("ns3::TcpSocketFactory", sinkAddress);
    source.SetAttribute("MaxBytes", UintegerValue(g_fileSize));
    source.SetAttribute("SendSize", UintegerValue(segmentSize));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(g_startTime));

    if (!cwndFileName.empty())
    {
        g_cwndFile.open(cwndFileName);
        Simulator::Schedule(Seconds(g_startTime) + NanoSeconds(1), &ConnectCwndTrace);
    }

    // -----------------------------------------------------------------------
    // Run.  The run stops by itself when the file is complete (RxCallback),
    // or at maxTime as a safety limit.
    // -----------------------------------------------------------------------
    Simulator::Stop(Seconds(g_startTime + maxTime));
    Simulator::Run();
    Simulator::Destroy();

    // CHANGE 8: print one result line (comma separated).
    // transferTime = -1 means the file did not finish within maxTime.
    double transferTime = (g_finishTime < 0) ? -1.0 : g_finishTime - g_startTime;
    if (header)
    {
        std::cout << "dataRate,delay,errorRate,initCwnd,segmentSize,delAckCount,run,"
                     "receivedBytes,transferTime"
                  << std::endl;
    }
    std::cout << dataRate << "," << delay << "," << errorRate << "," << initCwnd << ","
              << segmentSize << "," << delAckCount << "," << RngSeedManager::GetRun() << ","
              << g_rxBytes << "," << transferTime << std::endl;

    return 0;
}
