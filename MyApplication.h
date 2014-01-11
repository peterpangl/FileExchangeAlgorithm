//
// Copyright (C) 2009 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

/**
 * @author Antonio Zea
 * @author Panagopoulos Petros
 */

#ifndef MYAPPLICATION_H
#define MYAPPLICATION_H

#include <omnetpp.h>

#include "BaseApp.h"

#include <IPvXAddress.h>


#include <cstdlib>
#include <list>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

struct DBase{
	IPvXAddress nodeAddr;	// keep the ip adress
	list<int> pktIds;	// keep the ids
};

struct idScore{
	int pktId;
	list<IPvXAddress> nodes;
	int score;
};

struct xored{
	int ids[2];
};

class SeederNode {

  public :
	IPvXAddress seederIp;					// ip adress of node
	unsigned int gotInfo;							// flag counts from how many nodes i have received info msg
	// the pkts of seeder are the numPktFile
    list<struct DBase> Common;				// stores the address of sender and a list of ids that sender has in common with receiver
    list<struct DBase> notHas;				// stores the address of sender node and a list of ids that sender hasn't in contrast with receiver
    list<struct idScore> notHasIds_Score;	// keeps ids that neighbors haven't, score counts these neighbors
    list<struct idScore> commonIds_Score;

};

class LeecherNode {
    
  public:
    IPvXAddress leecherIp;		// ip adress of node
    unsigned int gotInfo;		// flag counts from how many nodes i have received info msg

    int cntLPkt;				// counts packets in buffer
    list<int> lBuf;				// holds the ids,lBuf always sorted
    list<struct xored> encBuf;	// hold ids that node is unable to decode
    int flagLee;				// distinguish first time of sending from others
    list<struct DBase> Common;	// stores the address of sender and a list of ids that sender has in common with receiver
    list<struct DBase> notHas;  // stores the address of sender node and a list of ids that sender hasn't in contrast with receiver
    list<struct idScore> notHasIds_Score;	// keeps ids that neighbors haven't, score counts these neighbors
    list<struct idScore> commonIds_Score;

};


extern list<SeederNode> Seeders;      	// list of Seeders including their ip address
extern list<LeecherNode> Leechers;     	// list of Leechers
static int flag = 0;
static int flagSeed =0; 				// use to send to each leecher
extern int numNode;							// for statistics


class MyApplication : public BaseApp, public LeecherNode
{

    // module parameters
    simtime_t sendPeriod;     // we'll store the "sendPeriod" parameter here
    simtime_t sendInfoPeriod; // we'll store the "sendInfoPeriod" parameter here
    int numToSend;            // we'll store the "numToSend" parameter here

    // statistics
    int numSent;              //number of packets sent
    int numReceived;          //number of packets received
    int xorSend;
    int xorRcv;
    int broadSnd;
    int broadRcv;
    int reqSnd;
    int avlSnd;
    int avlRcv;
    int notAvlRcv;
    int putInEnc;
    int numDec;
    int hasBoth;			  // has both ids from the encoded combination
    int notAvlSnd;
    int XORfailedToSend;
    int ignored;

    // simulation parameters
    int numSeeders;			  // number of seeders, defined in omnetpp.ini
    int numPktFile;			  // number of file packets, defined in omnetpp.ini
    double metric;			  // percentage of the nodes that have common pktId among them, for a specific notHasId
    simsignal_t RecFile;	  // activates when a node receives its file

    // our timer
    cMessage *timerMsg;
    cMessage *timerInfoMsg;

    // application routines
    void initializeApp(int stage);                 // called when the module is being created
    void finishApp();                              // called when the module is about to be destroyed
    void handleTimerEvent(cMessage* msg);          // called when we received a timer message
    void deliver(OverlayKey& key, cMessage* msg);  // called when we receive a message from the overlay
    void handleUDPMessage(cMessage* msg);          // called when we receive a UDP message
    bool isSeeder();                               // finds if the node is seeder
    bool idExists(int pktId, list<LeecherNode>::iterator it);
    int getRandomSeeder();							// find a random seeder
    int seederFindDiffId(vector<int> tmp1Ids);
    void putItInCommons(list<LeecherNode>::iterator it, IPvXAddress addr, int pktId);
    void broadcastInfoMsg(list<LeecherNode>::iterator lit);			//at Functions.cc

    /*********** common functions for seeders and leechers, locate in CommonFuncInfoMsg.cc*****************/
    int checkIfExist(int s, list<struct idScore> X);	// checks if we have already compute the score of s id.
    void xorPktsANDSend(int newPkt, int commonPkt, list<IPvXAddress> nodes);

    /*********** Seeder's functions for network coding, locate in SeederInfoMsgMngmt.cc ***********/
    void constructBasesOfSeeder(MyMessage *myMsg, list<SeederNode>::iterator sit);

	int findPossibleNewId(list<SeederNode>::iterator sit);

	int commonInXorCombination(list<SeederNode>::iterator sit);
	int constructCommonIds_ScoreBase(list<SeederNode>::iterator sit, list<IPvXAddress> nodes, int scoreNotHas);

	void sortNotHasIds_ScoreBase(list<SeederNode>::iterator sit);
	void sortCommonIds_ScoreBase(list<SeederNode>::iterator sit);

	void printBases(list<SeederNode>::iterator sit);
	void printNotHasIds_Score(list<SeederNode>::iterator sit);
	void printCommonIds_ScoreBase(list<SeederNode>::iterator sit);
	void clear(list<SeederNode>::iterator sit);

	/******** Leecher's functions for network coding, locate in LeecherInfoMsgMngmt.cc ************/
	void constructBasesOfLeecher(MyMessage *myMsg, list<LeecherNode>::iterator lit);

	int findPossibleNewId(list<LeecherNode>::iterator lit);	// find the possible news ids

	int commonInXorCombination(list<LeecherNode>::iterator lit);
	int constructCommonIds_ScoreBase(list<LeecherNode>::iterator lit, list<IPvXAddress> nodes, int scoreNotHas);

	void sortNotHasIds_ScoreBase(list<LeecherNode>::iterator lit);
	void sortCommonIds_ScoreBase(list<LeecherNode>::iterator lit);

	void printBases(list<LeecherNode>::iterator lit);
	void printNotHasIds_Score(list<LeecherNode>::iterator lit);
	void printCommonIds_ScoreBase(list<LeecherNode>::iterator lit);
	void clear(list<LeecherNode>::iterator lit);

	/******** after receiving a message XOR or AVL, locate in Functions.cc *********/
	void handleXORmsg(MyMessage *myMsg, list<LeecherNode>::iterator lit);
	void afterInsertingInLBuf(list<LeecherNode>::iterator lit);
	int checkInEncBuf(int a, int b, list<LeecherNode>::iterator lit);
	int receivedFile(list<LeecherNode>::iterator lit);
	int canDecodeFromEncBuf(list<LeecherNode>::iterator lit);

public:
    MyApplication() {
    	timerMsg = NULL;
    	timerInfoMsg = NULL;
    };
    ~MyApplication() { 
        cancelAndDelete(timerMsg); 
        cancelAndDelete(timerInfoMsg);
        // clear and set to zero global vars. otherwise problem with rebuild in Tkenv
        flag = 0;
        flagSeed = 0;
        numNode = 0;
        Seeders.clear();
        Leechers.clear(); 

    };
};


#endif
