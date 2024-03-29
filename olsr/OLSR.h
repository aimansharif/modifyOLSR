/***************************************************************************
 *   Copyright (C) 2004 by Francisco J. Ros                                *
 *   fjrm@dif.um.es                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

///
/// \file   OLSR.h
/// \brief  Header file for OLSR agent and related classes.
///
/// Here are defined all timers used by OLSR, including those for managing internal
/// state and those for sending messages. Class OLSR is also defined, therefore this
/// file has signatures for the most important methods. Lots of constants are also
/// defined.
///

#ifndef __OLSR_omnet_h__

#define __OLSR_omnet_h__
#include "ManetRoutingBase.h"

#include "OLSRpkt_m.h"
#include "OLSR_state.h"
#include "OLSR_rtable.h"
#include "OLSR_repositories.h"
#include "opp_utils.h"
#include "ManetAddress.h"
#include "IPv4Address.h"

#include <map>
#include <vector>

#include <assert.h>




/********** Useful macros **********/

/// Returns maximum of two numbers.
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/// Returns minimum of two numbers.
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/// Defines NULL as zero, used for pointers.
#ifndef NULL
#define NULL 0
#endif

#define IP_BROADCAST     ((uint32_t) 0xffffffff)

/// Gets current time from the scheduler.
#ifndef CURRENT_TIME
#define CURRENT_TIME    SIMTIME_DBL(simTime())
#endif

#ifndef CURRENT_TIME_T
#define CURRENT_TIME_T  SIMTIME_DBL(simTime())
#endif

#define debug  EV << OPP_Global::stringf



///
/// \brief Gets the delay between a given time and the current time.
///
/// If given time is previous to the current one, then this macro returns
/// a number close to 0. This is used for scheduling events at a certain moment.
///
#define DELAY(time) (((time) < (CURRENT_TIME)) ? (0.000001) : \
    (time - CURRENT_TIME + 0.000001))

#define DELAY_T(time) (((time) < (CURRENT_TIME_T)) ? (0.000001) : \
    (time - CURRENT_TIME_T + 0.000001))


/// Scaling factor used in RFC 3626.
#define OLSR_C      0.0625


    /********** Holding times **********/

    /// Neighbor holding time.
#define OLSR_NEIGHB_HOLD_TIME   3 * OLSR_REFRESH_INTERVAL
    /// Top holding time.
#define OLSR_TOP_HOLD_TIME  3 * tc_ival()
    /// Dup holding time.
#define OLSR_DUP_HOLD_TIME  30
    /// MID holding time.
#define OLSR_MID_HOLD_TIME  3 * mid_ival()
    /// HNA holding time.
#define OLSR_HNA_HOLD_TIME  3 * hna_ival()

/********** Link types **********/

/// Unspecified link type.
#define OLSR_UNSPEC_LINK    0
/// Asymmetric link type.
#define OLSR_ASYM_LINK      1
/// Symmetric link type.
#define OLSR_SYM_LINK       2
/// Lost link type.
#define OLSR_LOST_LINK      3

/********** Neighbor types **********/

/// Not neighbor type.
#define OLSR_NOT_NEIGH      0
/// Symmetric neighbor type.
#define OLSR_SYM_NEIGH      1
/// Asymmetric neighbor type.
#define OLSR_MPR_NEIGH      2


/********** Willingness **********/

/// Willingness for forwarding packets from other nodes: never.
#define OLSR_WILL_NEVER     0
/// Willingness for forwarding packets from other nodes: low.
#define OLSR_WILL_LOW       1
/// Willingness for forwarding packets from other nodes: medium.
#define OLSR_WILL_DEFAULT   3
/// Willingness for forwarding packets from other nodes: high.
#define OLSR_WILL_HIGH      6
/// Willingness for forwarding packets from other nodes: always.
#define OLSR_WILL_ALWAYS    7


/********** Miscellaneous constants **********/

/// Maximum allowed jitter.
#define OLSR_MAXJITTER      hello_ival()/4
/// Maximum allowed sequence number.
#define OLSR_MAX_SEQ_NUM    65535
/// Used to set status of an OLSR_nb_tuple as "not symmetric".
#define OLSR_STATUS_NOT_SYM 0
/// Used to set status of an OLSR_nb_tuple as "symmetric".
#define OLSR_STATUS_SYM     1
/// Random number between [0-OLSR_MAXJITTER] used to jitter OLSR packet transmission.
//#define JITTER            (Random::uniform()*OLSR_MAXJITTER)

class OLSRIf
{
  public:
    InterfaceEntry* dev;
    int32_t udp_send_sock;
    int32_t udp_recv_sock;
    int32_t udp_tunnel_sock;
    uint8_t if_num;
    uint8_t if_active;
    int32_t if_index;
    int8_t if_rp_filter_old;
    int8_t if_send_redirects_old;
    uint16_t seqno;
    bool wifi_if;
    ManetAddress address;
    ManetAddress broad;
};

class OLSR;         // forward declaratio



/********** Timers **********/

/// Basic timer class

class OLSR_Timer :  public cOwnedObject /*cMessage*/
{
  protected:
    OLSR*       agent_; ///< OLSR agent which created the timer.
    cObject* tuple_;
  public:

    virtual void removeTimer();
    OLSR_Timer(OLSR* agent);
    OLSR_Timer();
    ~OLSR_Timer();
    virtual void expire() = 0;
    virtual void removeQueueTimer();
    virtual void resched(double time);
    virtual void setTuple(cObject *tuple) {tuple_ = tuple;}
};


/// Timer for sending an enqued message.
class OLSR_MsgTimer : public OLSR_Timer
{
  public:
    OLSR_MsgTimer(OLSR* agent) : OLSR_Timer(agent) {}
    OLSR_MsgTimer():OLSR_Timer() {}
    void expire();
};

/// Timer for sending HELLO messages.
class OLSR_HelloTimer : public OLSR_Timer
{
  public:
    OLSR_HelloTimer(OLSR* agent) : OLSR_Timer(agent) {}
    OLSR_HelloTimer():OLSR_Timer() {}
    void expire();
};


/// Timer for sending TC messages.
class OLSR_TcTimer : public OLSR_Timer
{
  public:
    OLSR_TcTimer(OLSR* agent) : OLSR_Timer(agent) {}
    OLSR_TcTimer():OLSR_Timer() {}
    void expire();
};


/// Timer for sending MID messages.
class OLSR_MidTimer : public OLSR_Timer
{
  public:
    OLSR_MidTimer(OLSR* agent) : OLSR_Timer(agent) {}
    OLSR_MidTimer():OLSR_Timer() {}
    virtual void expire();
};


/// Timer for sending HNA messages.
class OLSR_HnaTimer : public OLSR_Timer
{
  public:
    OLSR_HnaTimer(OLSR* agent) : OLSR_Timer(agent) {}
    OLSR_HnaTimer():OLSR_Timer() {}
    void expire();
};


/// Timer for removing duplicate tuples: OLSR_dup_tuple.
class OLSR_DupTupleTimer : public OLSR_Timer
{
//  protected:
//  OLSR_dup_tuple* tuple_;
  public:
    OLSR_DupTupleTimer(OLSR* agent, OLSR_dup_tuple* tuple) : OLSR_Timer(agent)
    {
        tuple_ = tuple;
    }
    void setTuple(OLSR_dup_tuple* tuple) {tuple_ = tuple; tuple->asocTimer = this;}
    ~OLSR_DupTupleTimer();
    virtual void expire();
};

/// Timer for removing link tuples: OLSR_link_tuple.
class OLSR_LinkTupleTimer : public OLSR_Timer
{
  public:
    OLSR_LinkTupleTimer(OLSR* agent, OLSR_link_tuple* tuple);

    void setTuple(OLSR_link_tuple* tuple) {tuple_ = tuple; tuple->asocTimer = this;}
    ~OLSR_LinkTupleTimer();
    virtual void expire();
  protected:
    //OLSR_link_tuple*  tuple_; ///< OLSR_link_tuple which must be removed.
    bool            first_time_;

};


/// Timer for removing nb2hop tuples: OLSR_nb2hop_tuple.

class OLSR_Nb2hopTupleTimer : public OLSR_Timer
{
  public:
    OLSR_Nb2hopTupleTimer(OLSR* agent, OLSR_nb2hop_tuple* tuple) : OLSR_Timer(agent)
    {
        tuple_ = tuple;
    }

    void setTuple(OLSR_nb2hop_tuple* tuple) {tuple_ = tuple; tuple->asocTimer = this;}
    ~OLSR_Nb2hopTupleTimer();
    virtual void expire();
//  protected:
//  OLSR_nb2hop_tuple*  tuple_; ///< OLSR_link_tuple which must be removed.

};




/// Timer for removing MPR selector tuples: OLSR_mprsel_tuple.
class OLSR_MprSelTupleTimer : public OLSR_Timer
{
  public:
    OLSR_MprSelTupleTimer(OLSR* agent, OLSR_mprsel_tuple* tuple) : OLSR_Timer(agent)
    {
        tuple_ = tuple;
    }

    void setTuple(OLSR_mprsel_tuple* tuple) {tuple_ = tuple; tuple->asocTimer = this;}
    ~OLSR_MprSelTupleTimer();
    virtual void expire();

//  protected:
//  OLSR_mprsel_tuple*  tuple_; ///< OLSR_link_tuple which must be removed.

};


/// Timer for removing topology tuples: OLSR_topology_tuple.

class OLSR_TopologyTupleTimer : public OLSR_Timer
{
  public:
    OLSR_TopologyTupleTimer(OLSR* agent, OLSR_topology_tuple* tuple) : OLSR_Timer(agent)
    {
        tuple_ = tuple;
    }

    void setTuple(OLSR_topology_tuple* tuple) {tuple_ = tuple; tuple->asocTimer = this;}
    ~OLSR_TopologyTupleTimer();
    virtual void expire();
//  protected:
//  OLSR_topology_tuple*    tuple_; ///< OLSR_link_tuple which must be removed.

};

/// Timer for removing interface association tuples: OLSR_iface_assoc_tuple.
class OLSR_IfaceAssocTupleTimer : public OLSR_Timer
{
  public:
    OLSR_IfaceAssocTupleTimer(OLSR* agent, OLSR_iface_assoc_tuple* tuple) : OLSR_Timer(agent)
    {
        tuple_ = tuple;
    }

    void setTuple(OLSR_iface_assoc_tuple* tuple) {tuple_ = tuple; tuple->asocTimer = this;}
    ~OLSR_IfaceAssocTupleTimer();
    virtual void expire();
//  protected:
//  OLSR_iface_assoc_tuple* tuple_; ///< OLSR_link_tuple which must be removed.

};

/// Timer for removing association tuples: OLSR_association_tuple.
class OLSR_AssociationTupleTimer : public OLSR_Timer
{
  public:
    OLSR_AssociationTupleTimer(OLSR* agent, OLSR_association_tuple* tuple) : OLSR_Timer(agent)
    {
        tuple_ = tuple;
    }

    void setTuple(OLSR_association_tuple* tuple) {tuple_ = tuple; tuple->asocTimer = this;}
    ~OLSR_AssociationTupleTimer();
    virtual void expire();
//  protected:
//  OLSR_association_tuple* tuple_; ///< OLSR_association which must be removed.

};

//class cleanRoutingTable
//{
//    public:
//    double purgeRoutingTable_;
//    bool purged_;
//
//    inline double purgeRoutingTable() {return purgeRoutingTable_;}
//    inline bool purged() {return purged_;}
//    void setPurgeRoutingTable(double purgeRoutingTable) {purgeRoutingTable_ = purgeRoutingTable;}
//    void setPurged(double purged) {purged_ = purged;}
//
//};


class GwNode
{
  public:
    nsaddr_t orig_addr;
    uint16_t gw_port;
    uint16_t gw_failure;
    simtime_t last_failure;
    simtime_t deleted;
  public:
    GwNode() : orig_addr(), gw_port(0), gw_failure(0), last_failure(0), deleted(0) {}
};


/********** OLSR Agent **********/


///
/// \brief Routing agent which implements %OLSR protocol following RFC 3626.
///
/// Interacts with TCL interface through command() method. It implements all
/// functionalities related to sending and receiving packets and managing
/// internal state.
///

typedef std::set<OLSR_Timer *> TimerPendingList;
typedef std::multimap <simtime_t, OLSR_Timer *> TimerQueue;


class OLSR : public ManetRoutingBase
{
  protected:
    /********** Intervals **********/
    ///
    /// \brief Period at which a node must cite every link and every neighbor.
    ///
    /// We only use this value in order to define OLSR_NEIGHB_HOLD_TIME.
    ///
    double OLSR_REFRESH_INTERVAL;//   2

    double jitter() {return uniform(0,(double)OLSR_MAXJITTER);}
#define JITTER jitter()

  private:
    friend class OLSR_HelloTimer;
    friend class OLSR_TcTimer;
    friend class OLSR_MidTimer;
    friend class OLSR_HnaTimer;
    friend class OLSR_DupTupleTimer;
    friend class OLSR_LinkTupleTimer;
    friend class OLSR_Nb2hopTupleTimer;
    friend class OLSR_MprSelTupleTimer;
    friend class OLSR_TopologyTupleTimer;
    friend class OLSR_IfaceAssocTupleTimer;
    friend class OLSR_AssociationTupleTimer;
    friend class OLSR_MsgTimer;
    friend class OLSR_Timer;
    unsigned char get_gw_class(int down, int up);
    void get_gw_speeds(unsigned char gw_class, int *down, int *up);

    void choose_gw(void);
    OLSRIf * is_olsr_if(InterfaceEntry * dev);


  protected:

    //std::priority_queue<TimerQueueElem> *timerQueuePtr;
    bool topologyChange;
    virtual void setTopologyChanged(bool p) {topologyChange = p;}
    virtual bool getTopologyChanged() {return topologyChange;}
    TimerQueue *timerQueuePtr;

    cMessage *timerMessage;

// must be protected and used for dereved class OLSR_ETX
    /// A list of pending messages which are buffered awaiting for being sent.
    std::vector<OLSR_msg>   msgs_;
    /// Routing table.
    OLSR_rtable     rtable_;

    uint32_t prev_rtablesize; //used to keep track of routing table size changes

    typedef std::map<nsaddr_t,OLSR_rtable*> GlobalRtable;
    static GlobalRtable globalRtable;
    typedef std::map<nsaddr_t,std::vector<nsaddr_t> > DistributionPath;
    static DistributionPath distributionPath;
    bool computed;
    /// Internal state with all needed data structs.

    OLSR_state      *state_ptr;

    /// Packets sequence number counter.
    uint16_t    pkt_seq_;
    /// Messages sequence number counter.
    uint16_t    msg_seq_;
    /// Advertised Neighbor Set sequence number.
    uint16_t    ansn_;

    /// HELLO messages' emission interval.
    cPar     *hello_ival_;
    /// TC messages' emission interval.
    cPar     *tc_ival_;
    /// MID messages' emission interval.
    cPar     *mid_ival_;
    /// HNA messages' emission interval.
    cPar     *hna_ival_;
    /// Willingness for forwarding packets on behalf of other nodes.
    cPar     *willingness_;
    /// Determines if layer 2 notifications are enabled or not.
    int     use_mac_;
    bool useIndex;
    bool printOLSRmessages;


    /// Address of the routing agent.
    nsaddr_t ra_addr_;

    bool optimizedMid;

  protected:
// Omnet INET vaiables and functions
    char nodeName[50];


    virtual OLSR_pkt * check_packet(cPacket*, nsaddr_t &, int &);

    // PortClassifier*  dmux_;      ///< For passing packets up to agents.
    // Trace*       logtarget_; ///< For logging.

    OLSR_HelloTimer *helloTimer;    ///< Timer for sending HELLO messages.
    OLSR_TcTimer    *tcTimer;   ///< Timer for sending TC messages.
    OLSR_MidTimer   *midTimer;  ///< Timer for sending MID messages.
    OLSR_HnaTimer   *hnaTimer;   ///< Timer for sending HNA messages.

#define hello_timer_  (*helloTimer)
#define  tc_timer_  (*tcTimer)
#define mid_timer_  (*midTimer)
#define  hna_timer_  (*hnaTimer)

    /// Increments packet sequence number and returns the new value.
    inline uint16_t pkt_seq()
    {
        pkt_seq_ = (pkt_seq_ + 1)%(OLSR_MAX_SEQ_NUM + 1);
        return pkt_seq_;
    }
    /// Increments message sequence number and returns the new value.
    inline uint16_t msg_seq()
    {
        msg_seq_ = (msg_seq_ + 1)%(OLSR_MAX_SEQ_NUM + 1);
        return msg_seq_;
    }

    inline nsaddr_t    ra_addr()   { return ra_addr_;}

    inline double     hello_ival()    { return hello_ival_->doubleValue();}
    inline double     tc_ival()   { return tc_ival_->doubleValue();}
    inline double     mid_ival()  { return mid_ival_->doubleValue();}
    inline double     hna_ival()   { return hna_ival_->doubleValue();}
    inline int     willingness()   { return willingness_->longValue();}
    inline int     use_mac()   { return use_mac_;}

    inline linkset_t&   linkset()   { return state_ptr->linkset(); }
    inline mprset_t&    mprset()    { return state_ptr->mprset(); }
    inline mprselset_t& mprselset() { return state_ptr->mprselset(); }
    inline nbset_t&     nbset()     { return state_ptr->nbset(); }
    inline nb2hopset_t& nb2hopset() { return state_ptr->nb2hopset(); }
    inline topologyset_t&   topologyset()   { return state_ptr->topologyset(); }
    inline dupset_t&    dupset()    { return state_ptr->dupset(); }
    inline ifaceassocset_t& ifaceassocset() { return state_ptr->ifaceassocset(); }
    inline associationset_t& associationset() { return state_ptr->associationset();}
    inline associations_t& associations() { return state_ptr->associations();}


    virtual void        recv_olsr(cMessage*);

    virtual void        CoverTwoHopNeighbors(const nsaddr_t &neighborMainAddr, nb2hopset_t & N2);
    virtual void        mpr_computation();
    virtual void        rtable_computation();


    //my input ---- silas
    // this is to purge the routing tables at some point in the simulation.
//    virtual void        routingTableTimer(cMessage* msg);
    cMessage *purgeTables;

    // my input ''''' application specific .... for DHT update
    virtual void        cache_recent_table();

    virtual bool        process_hello(OLSR_msg&, const nsaddr_t &, const nsaddr_t &, const int &);
    virtual bool        process_tc(OLSR_msg&, const nsaddr_t &, const int &);
    virtual void        process_mid(OLSR_msg&, const nsaddr_t &, const int &);
    virtual bool        process_hna(OLSR_msg&, const nsaddr_t &, const int &);

    virtual void        forward_default(OLSR_msg&, OLSR_dup_tuple*, const nsaddr_t &, const nsaddr_t &);
    virtual void        forward_data(cMessage* p) {}

    virtual void        enque_msg(OLSR_msg&, double);
    virtual void        send_hello();
    virtual void        send_tc();
    virtual void        send_mid();
    virtual void        send_hna();
    virtual void        send_pkt();

    virtual bool        link_sensing(OLSR_msg&, const nsaddr_t &, const nsaddr_t &, const int &);
    virtual bool        populate_nbset(OLSR_msg&);
    virtual bool        populate_nb2hopset(OLSR_msg&);
    virtual void        populate_mprselset(OLSR_msg&);

    virtual void        set_hello_timer();
    virtual void        set_tc_timer();
    virtual void        set_mid_timer();
    virtual void        set_hna_timer();

    virtual void        nb_loss(OLSR_link_tuple*);
    virtual void        add_dup_tuple(OLSR_dup_tuple*);
    virtual void        rm_dup_tuple(OLSR_dup_tuple*);
    virtual void        add_link_tuple(OLSR_link_tuple*, uint8_t);
    virtual void        rm_link_tuple(OLSR_link_tuple*);
    virtual void        updated_link_tuple(OLSR_link_tuple*, uint8_t willingness);
    virtual void        add_nb_tuple(OLSR_nb_tuple*);
    virtual void        rm_nb_tuple(OLSR_nb_tuple*);
    virtual void        add_nb2hop_tuple(OLSR_nb2hop_tuple*);
    virtual void        rm_nb2hop_tuple(OLSR_nb2hop_tuple*);
    virtual void        add_mprsel_tuple(OLSR_mprsel_tuple*);
    virtual void        rm_mprsel_tuple(OLSR_mprsel_tuple*);
    virtual void        add_topology_tuple(OLSR_topology_tuple*);
    virtual void        rm_topology_tuple(OLSR_topology_tuple*);
    virtual void        add_ifaceassoc_tuple(OLSR_iface_assoc_tuple*);
    virtual void        rm_ifaceassoc_tuple(OLSR_iface_assoc_tuple*);
    virtual void        add_association_tuple(OLSR_association_tuple*);
    virtual void        rm_association_tuple(OLSR_association_tuple*);
    virtual OLSR_nb_tuple*    find_or_add_nb(OLSR_link_tuple*, uint8_t willingness);

    const nsaddr_t  & get_main_addr(const nsaddr_t&) const;
    virtual int     degree(OLSR_nb_tuple*);

    static bool seq_num_bigger_than(uint16_t, uint16_t);
    virtual int numInitStages() const  {return 5;}
    virtual void initialize(int stage);
    virtual void    mac_failed(IPv4Datagram*);
    virtual void    recv(cMessage *p) {}

    virtual void handleMessage(cMessage *msg);
    virtual void finish();
    //virtual void processPromiscuous(const cObject *details){};
    virtual void processLinkBreak(const cObject *details);
    virtual void scheduleNextEvent();

    ManetAddress getIfaceAddressFromIndex(int index);

    const char * getNodeId(const nsaddr_t &addr);
    short unsigned int getNodeId1(const uint16_t &addr);

    void computeDistributionPath(const nsaddr_t &initNode);

  protected:
    GwNode *curr_gateway;

  public:
    OLSR() {}
    virtual ~OLSR();

    virtual void add_association(OLSR_association*);
    virtual void rm_association(OLSR_association*);

    static double       emf_to_seconds(uint8_t);
    static uint8_t      seconds_to_emf(double);
    static int      node_id(const nsaddr_t&);


    // Routing information access
    virtual bool supportGetRoute() {return true;}
    virtual uint32_t getRoute(const ManetAddress &, std::vector<ManetAddress> &);
    virtual bool getNextHop(const ManetAddress &, ManetAddress &add, int &iface, double &);
    virtual bool isProactive();
    virtual void setRefreshRoute(const ManetAddress &destination, const ManetAddress & nextHop,bool isReverse) {}
    virtual bool isOurType(cPacket *);
    virtual bool getDestAddress(cPacket *, ManetAddress &);
    virtual int getRouteGroup(const AddressGroup &gr, std::vector<ManetAddress>&);
    virtual bool getNextHopGroup(const AddressGroup &gr, ManetAddress &add, int &iface, ManetAddress&);
    virtual int  getRouteGroup(const ManetAddress&, std::vector<ManetAddress> &, ManetAddress&, bool &, int group = 0);
    virtual bool getNextHopGroup(const ManetAddress&, ManetAddress &add, int &iface, ManetAddress&, bool &, int group = 0);

    //
    virtual void getDistributionPath(const ManetAddress&, std::vector<ManetAddress> &path);

    virtual bool isNodeCandidate(const ManetAddress&);

    virtual bool handleNodeStart(IDoneCallback *doneCallback);
    virtual bool handleNodeShutdown(IDoneCallback *doneCallback);
    virtual void handleNodeCrash();
    bool debugOutput;           /**< debug output ? */

    //STATISTICS
    long numHelloMsgs; /**<number of hello messages>*/
//    long numTimesHelloTimerIsFired;
    long numTcMsgs; /**<number of tc messages>*/
    long numHnaMsgs; /**<number of hna messages>*/
    long numForwTc; /**<number of forwarded tc>*/
    long numForwHna; /**<number of forwarded hna>*/
    double TotalMessagesOLSRVector;

    IRoutingTable *inet_rt;
    NotificationBoard *nb;



    typedef struct rTable_Cache: public cObject
    {
        //destination of this packet
        IPv4Address dest_addr_;
        //next hop to this destination
        IPv4Address gw_addr_;
        IPv4Address netmask_;
        int hopCount_;
        int interface_;

        inline IPv4Address& dest_addr() {return dest_addr_;}
        inline IPv4Address& gw_addr() {return gw_addr_;}
        inline IPv4Address& netmask() {return netmask_;}
        inline int& hopCount() {return hopCount_;}
        inline int& interface() {return interface_;}
        void setDest_addr(const IPv4Address &dest_addr) {dest_addr_ = dest_addr;}
        void setGw_addr(const IPv4Address &gw_addr) {gw_addr_ = gw_addr;}
        void setNetmask(const IPv4Address &netmask)  {netmask_ = netmask;}
        void setHopCount(int hopCount) {hopCount_ = hopCount;}
        void setInterface(int interface) {interface_ = interface;}

        rTable_Cache(){}
        rTable_Cache(rTable_Cache * e) {
            dest_addr_ = e->dest_addr_;
            gw_addr_ = e->gw_addr_;
            netmask_ = e->netmask_;
            interface_ = e->interface_;
        }

        bool operator==(const rTable_Cache& other) const {return this->dest_addr_ == other.dest_addr_
                && this->gw_addr_ == other.gw_addr_ && this->netmask_ == other.netmask_
                && this->hopCount_ == other.hopCount_;
        }

    }rTable_Cache;

    typedef std::vector<rTable_Cache> rTable_CacheVector; ///<DHT entry Set type.
    rTable_CacheVector item_;
    inline rTable_CacheVector& item() {return item_;}

//    virtual void insert_rTable_Cache(rTable_Cache);
    //void erase_DHT_rTable_Cache(DHT_rTable_Cache);

//    struct deleteCtrl : public cObject
//    {
//        IPv4Address gateway_;
//        IPv4Address host_;
//    };

};

#endif
