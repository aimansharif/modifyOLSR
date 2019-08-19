package p2psipmanet.simulations.HierarchicalNetwork;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.nodes.inet.AdhocHost; // base node model, useful for nodes in cluster
import inet.world.radio.ChannelControl; // to use different channels in different subnets
import p2psipmanet.simulations.HierarchicalNetwork.gateway; // defines gateway with 2nd radio and routing protocol

//
// Models a network with several hosts and gateways. Each host may contain one or
// more radios. Nodes are using adhoc routing to pass information. A gateway node
// will also have (potentially) multiple radios, but also a second routing protocol
// instance, which we can bind in the *.ini file to a different radios
//
network OLSR
{
    parameters:

        @display("bgb=1023,595");
        double hosts;
        double gw;

    submodules:
        // allows us to assign dedicated channels to various radios
        channelControl: ChannelControl {
            @display("p=42,37");
        }
        // useful for assigning initial IPv4 addresses
        configurator: IPv4NetworkConfigurator {
            @display("p=47,98");
        }
        // the network has a number of gateways, defined by parameter GW
        gw[gw]: gateway {
            @display("p=160,38");
        }
        // the network has a number of hosts, defined by parameter HOSTS
        host[hosts]: AdhocHost {
            @display("p=251,38");
        }
    connections allowunconnected:
}

