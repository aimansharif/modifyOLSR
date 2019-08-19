package p2psipmanet.simulations.HierarchicalNetwork;

import inet.nodes.inet.AdhocHost;
import inet.networklayer.IManetRouting;

//
// Gateway (runs two instances of a routing protocol)
// Easiest way to implement it: a gateways is an AdhocHost 
// with a second routing protocol instance
//
//
// @author Thomas Kunz
//
module gateway extends AdhocHost
{
    parameters:
        // parameter to instantiate routing protocol, default is OLSR
        string backboneRoutingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman", " ") = default("inet.networklayer.manetrouting.OLSR");
        @display("bgb=613,521;i=device/cellphone");

    submodules:
        backbonemanetrouting: <backboneRoutingProtocol> like IManetRouting {
            @display("p=550,287");
        }

    connections allowunconnected:
        // the second routing protocol instance also is connected to the networkLayer
        // same as the routing protocol instance defined in AdhocHost
        backbonemanetrouting.to_ip --> networkLayer.transportIn++;
        backbonemanetrouting.from_ip <-- networkLayer.transportOut++;
}

